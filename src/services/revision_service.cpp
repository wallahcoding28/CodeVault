#include "services/revision_service.hpp"

#include <algorithm>

namespace codevault::services {

RevisionService::RevisionService(std::shared_ptr<utils::IClock> clock)
    : clock_(std::move(clock)) {
    if (!clock_) {
        clock_ = std::make_shared<utils::SystemClock>();
    }
}

int64_t RevisionService::now() const {
    return clock_ ? clock_->now() : 0;
}

void RevisionService::setClock(std::shared_ptr<utils::IClock> clock) {
    clock_ = std::move(clock);
}

std::shared_ptr<utils::IClock> RevisionService::getClock() const noexcept {
    return clock_;
}

void RevisionService::scheduleQuestion(const std::string& questionId, int64_t nextRevisionAt, int32_t priority) {
    if (questionId.empty()) return;
    heap_.push(models::RevisionItem(questionId, nextRevisionAt, priority));
}

void RevisionService::rescheduleQuestion(const std::string& questionId, int64_t nextRevisionAt, int32_t priority) {
    if (questionId.empty()) return;

    std::vector<models::RevisionItem> items;
    items.reserve(heap_.size() + 1);

    while (!heap_.empty()) {
        auto item = heap_.extractMin();
        if (item.questionId != questionId) {
            items.push_back(std::move(item));
        }
    }

    items.push_back(models::RevisionItem(questionId, nextRevisionAt, priority));
    heap_ = dsa::MinHeap<models::RevisionItem, models::RevisionItemComparator>(items.begin(), items.end());
}

void RevisionService::removeQuestion(const std::string& questionId) {
    if (questionId.empty() || heap_.empty()) return;

    std::vector<models::RevisionItem> items;
    items.reserve(heap_.size());

    while (!heap_.empty()) {
        auto item = heap_.extractMin();
        if (item.questionId != questionId) {
            items.push_back(std::move(item));
        }
    }

    heap_ = dsa::MinHeap<models::RevisionItem, models::RevisionItemComparator>(items.begin(), items.end());
}

void RevisionService::scheduleRevision(const models::RevisionItem& item) {
    if (item.questionId.empty()) return;
    heap_.push(item);
}

void RevisionService::loadFromQuestions(const std::vector<models::Question>& questions) {
    clear();
    for (const auto& q : questions) {
        if (q.getNextRevisionAt() > 0) {
            heap_.push(models::RevisionItem(
                q.getId(),
                q.getNextRevisionAt(),
                q.getRevisionPriority()
            ));
        }
    }
}

RevisionScheduleResult RevisionService::calculateNextSchedule(
    const models::Question& question,
    models::PracticeVerdict verdict,
    int64_t currentTime) const {
    if (currentTime <= 0) {
        currentTime = now();
    }

    RevisionScheduleResult result;
    int currentLevel = calculateLevelFromQuestion(question);

    switch (verdict) {
        case models::PracticeVerdict::Solved: {
            result.nextLevel = (currentLevel == 0) ? 1 : std::min(currentLevel + 1, 5);
            result.intervalSeconds = getIntervalForLevel(result.nextLevel);
            result.lastPracticedAt = currentTime;
            result.nextRevisionAt = currentTime + result.intervalSeconds;

            // Revision priority: 1 = highest urgency, 5 = lowest urgency.
            // As question gets solved repeatedly, urgency decreases towards lower urgency.
            switch (result.nextLevel) {
                case 1: result.revisionPriority = 3; break;
                case 2: result.revisionPriority = 3; break;
                case 3: result.revisionPriority = 4; break;
                case 4: result.revisionPriority = 4; break;
                default: result.revisionPriority = 5; break;
            }

            if (result.nextLevel >= 5) {
                result.newStatus = models::Status::Mastered;
            } else {
                result.newStatus = (question.getStatus() == models::Status::Mastered)
                                       ? models::Status::Mastered
                                       : models::Status::Solved;
            }
            break;
        }

        case models::PracticeVerdict::NeedsReview: {
            // Needs review: reset interval to 1 day (level 1), highest urgency (priority 1)
            result.nextLevel = 1;
            result.intervalSeconds = getIntervalForLevel(1);
            result.lastPracticedAt = currentTime;
            result.nextRevisionAt = currentTime + result.intervalSeconds;
            result.revisionPriority = 1; // Highest urgency

            // Question remains a revision candidate
            result.newStatus = models::Status::InProgress;
            break;
        }

        case models::PracticeVerdict::Skipped: {
            // Skipped: question is not treated as completed; preserve existing state.
            result.nextLevel = currentLevel;
            result.intervalSeconds = (currentLevel > 0) ? getIntervalForLevel(currentLevel) : 0;
            result.lastPracticedAt = question.getLastPracticedAt();
            result.nextRevisionAt = question.getNextRevisionAt();
            result.revisionPriority = question.getRevisionPriority();
            result.newStatus = question.getStatus();
            break;
        }
    }

    return result;
}

RevisionScheduleResult RevisionService::markRevisionResult(
    models::Question& question,
    models::PracticeVerdict verdict,
    int64_t currentTime) {
    if (currentTime <= 0) {
        currentTime = now();
    }

    auto sched = calculateNextSchedule(question, verdict, currentTime);

    if (verdict != models::PracticeVerdict::Skipped) {
        question.setLastPracticedAt(sched.lastPracticedAt);
        question.setNextRevisionAt(sched.nextRevisionAt);
        question.setRevisionPriority(sched.revisionPriority);
        question.setStatus(sched.newStatus);
        question.setUpdatedAt(currentTime);

        rescheduleQuestion(question.getId(), sched.nextRevisionAt, sched.revisionPriority);
    }

    return sched;
}

std::vector<models::RevisionItem> RevisionService::getDueQuestions(int64_t asOfTime) const {
    if (asOfTime < 0) {
        asOfTime = now();
    }

    std::vector<models::RevisionItem> due;
    auto tempHeap = heap_;
    while (!tempHeap.empty()) {
        auto item = tempHeap.extractMin();
        if (item.nextRevisionAt <= asOfTime) {
            due.push_back(item);
        }
    }
    return due;
}

std::vector<models::RevisionItem> RevisionService::getUpcomingRevisions(size_t limit, int64_t asOfTime) const {
    if (asOfTime < 0) {
        asOfTime = now();
    }

    std::vector<models::RevisionItem> upcoming;
    auto tempHeap = heap_;
    while (!tempHeap.empty() && upcoming.size() < limit) {
        auto item = tempHeap.extractMin();
        if (item.nextRevisionAt > asOfTime) {
            upcoming.push_back(item);
        }
    }
    return upcoming;
}

std::optional<models::RevisionItem> RevisionService::getNextDueRevision() const {
    if (heap_.empty()) {
        return std::nullopt;
    }
    return heap_.top();
}

std::optional<models::RevisionItem> RevisionService::getNextDueRevision(int64_t asOfTime) const {
    if (heap_.empty()) {
        return std::nullopt;
    }
    const auto& top = heap_.top();
    if (top.nextRevisionAt <= asOfTime) {
        return top;
    }
    return std::nullopt;
}

std::optional<models::RevisionItem> RevisionService::nextRevision() {
    return popNextDueRevision();
}

std::optional<models::RevisionItem> RevisionService::popNextDueRevision() {
    if (heap_.empty()) {
        return std::nullopt;
    }
    return heap_.extractMin();
}

size_t RevisionService::getDueRevisionsCount() const noexcept {
    return heap_.size();
}

size_t RevisionService::getDueCount(int64_t asOfTime) const {
    if (asOfTime < 0) {
        asOfTime = now();
    }
    size_t count = 0;
    auto tempHeap = heap_;
    while (!tempHeap.empty()) {
        auto item = tempHeap.extractMin();
        if (item.nextRevisionAt <= asOfTime) {
            ++count;
        }
    }
    return count;
}

size_t RevisionService::getTotalScheduledCount() const noexcept {
    return heap_.size();
}

bool RevisionService::hasDueRevisions() const noexcept {
    return !heap_.empty();
}

bool RevisionService::hasDueRevisions(int64_t asOfTime) const {
    return getDueCount(asOfTime) > 0;
}

void RevisionService::clear() noexcept {
    heap_.clear();
}

} // namespace codevault::services
