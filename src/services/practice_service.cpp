#include "services/practice_service.hpp"
#include "services/question_service.hpp"

#include <algorithm>
#include <ctime>

namespace codevault::services {

PracticeService::PracticeService(
    std::shared_ptr<QuestionService> questionService,
    std::shared_ptr<RevisionService> revisionService)
    : questionService_(std::move(questionService)),
      revisionService_(std::move(revisionService)) {}

void PracticeService::startSession(const std::vector<std::string>& questionIds) {
    clearSession();
    for (const auto& id : questionIds) {
        if (!id.empty()) {
            if (questionService_ && !questionService_->questionExists(id)) {
                continue;
            }
            sessionQueue_.enqueue(id);
            ++totalSessionQuestions_;
        }
    }
}

void PracticeService::startSessionWithFilter(const models::QuestionFilter& filter, bool dueOnly) {
    clearSession();
    if (!questionService_) return;

    std::vector<std::string> targetIds;
    if (dueOnly && revisionService_) {
        auto dueItems = revisionService_->getDueQuestions();
        for (const auto& item : dueItems) {
            auto qOpt = questionService_->getQuestionById(item.questionId);
            if (qOpt.has_value() && filter.matches(qOpt.value())) {
                targetIds.push_back(item.questionId);
            }
        }
    } else {
        auto all = questionService_->getAllQuestions();
        for (const auto& q : all) {
            if (filter.matches(q)) {
                targetIds.push_back(q.getId());
            }
        }
    }
    startSession(targetIds);
}

void PracticeService::enqueueQuestion(const std::string& questionId) {
    if (!questionId.empty()) {
        if (questionService_ && !questionService_->questionExists(questionId)) {
            return;
        }
        sessionQueue_.enqueue(questionId);
        ++totalSessionQuestions_;
    }
}

std::vector<std::string> PracticeService::getQueueQuestionIds() const {
    return sessionQueue_.toVector();
}

std::vector<models::Question> PracticeService::getQueueQuestions() const {
    std::vector<models::Question> questions;
    if (!questionService_) return questions;

    auto ids = sessionQueue_.toVector();
    questions.reserve(ids.size());
    for (const auto& id : ids) {
        auto qOpt = questionService_->getQuestionById(id);
        if (qOpt.has_value()) {
            questions.push_back(qOpt.value());
        }
    }
    return questions;
}

bool PracticeService::removeQuestionFromQueue(const std::string& questionId) {
    if (sessionQueue_.remove(questionId)) {
        if (totalSessionQuestions_ > 0) {
            --totalSessionQuestions_;
        }
        return true;
    }
    return false;
}

bool PracticeService::isQuestionQueued(const std::string& questionId) const {
    return sessionQueue_.contains(questionId);
}

std::optional<std::string> PracticeService::getCurrentQuestion() const {
    if (sessionQueue_.empty()) {
        return std::nullopt;
    }
    return sessionQueue_.front();
}

std::optional<std::string> PracticeService::completeCurrentQuestion() {
    if (sessionQueue_.empty()) {
        return std::nullopt;
    }
    std::string current = sessionQueue_.front();
    sessionQueue_.dequeue();
    ++completedCount_;
    return current;
}

bool PracticeService::skipCurrentQuestion() {
    if (sessionQueue_.size() <= 1) {
        return false;
    }
    std::string current = sessionQueue_.front();
    sessionQueue_.dequeue();
    sessionQueue_.enqueue(std::move(current));
    ++skippedCount_;
    return true;
}

std::optional<std::string> PracticeService::advanceToNextQuestion() {
    return completeCurrentQuestion();
}

std::optional<RevisionScheduleResult> PracticeService::recordPracticeAttempt(
    models::PracticeVerdict verdict,
    int64_t currentTime) {
    if (sessionQueue_.empty()) {
        return std::nullopt;
    }
    return recordPracticeAttemptForQuestion(sessionQueue_.front(), verdict, currentTime);
}

std::optional<RevisionScheduleResult> PracticeService::recordPracticeAttemptForQuestion(
    const std::string& questionId,
    models::PracticeVerdict verdict,
    int64_t currentTime) {
    if (questionId.empty() || !questionService_ || !revisionService_) {
        return std::nullopt;
    }

    auto qOpt = questionService_->getQuestionById(questionId);
    if (!qOpt.has_value()) {
        return std::nullopt;
    }

    models::Question q = qOpt.value();
    RevisionScheduleResult sched = revisionService_->markRevisionResult(q, verdict, currentTime);
    questionService_->updateQuestion(q);

    // If the question is currently in the session queue, update session state
    if (sessionQueue_.contains(questionId)) {
        if (!sessionQueue_.empty() && sessionQueue_.front() == questionId) {
            if (verdict == models::PracticeVerdict::Solved || verdict == models::PracticeVerdict::NeedsReview) {
                completeCurrentQuestion();
            } else if (verdict == models::PracticeVerdict::Skipped) {
                if (sessionQueue_.size() > 1) {
                    skipCurrentQuestion();
                } else {
                    completeCurrentQuestion();
                    ++skippedCount_;
                }
            }
        } else {
            // Present in queue but not at the front
            if (verdict == models::PracticeVerdict::Solved || verdict == models::PracticeVerdict::NeedsReview) {
                removeQuestionFromQueue(questionId);
                ++completedCount_;
            }
        }
    }

    return sched;
}

models::PracticeNextResult PracticeService::getPracticeNext(
    const models::PracticeNextCriteria& criteria,
    int64_t asOfTime) const {
    models::PracticeNextResult res;
    res.recommendationReason = "No eligible problem found matching criteria";
    if (!questionService_) return res;

    int64_t now = asOfTime > 0 ? asOfTime : static_cast<int64_t>(std::time(nullptr));

    // Priority 1: Head of active practice session queue (if matching criteria)
    if (!sessionQueue_.empty()) {
        auto qIds = sessionQueue_.toVector();
        for (const auto& id : qIds) {
            auto qOpt = questionService_->getQuestionById(id);
            if (qOpt.has_value()) {
                const auto& q = qOpt.value();
                if ((!criteria.topic.has_value() || q.getTopic() == *criteria.topic) &&
                    (!criteria.difficulty.has_value() || q.getDifficulty() == *criteria.difficulty)) {
                    res.hasQuestion = true;
                    res.question = q;
                    res.recommendationReason = "Queued in your active practice queue";
                    return res;
                }
            }
        }
    }

    // Priority 2: Overdue Spaced Revision from MinHeap
    if (criteria.includeDueRevisions && revisionService_) {
        auto dueItems = revisionService_->getDueQuestions(now);
        for (const auto& item : dueItems) {
            auto qOpt = questionService_->getQuestionById(item.questionId);
            if (qOpt.has_value()) {
                const auto& q = qOpt.value();
                if ((!criteria.topic.has_value() || q.getTopic() == *criteria.topic) &&
                    (!criteria.difficulty.has_value() || q.getDifficulty() == *criteria.difficulty)) {
                    res.hasQuestion = true;
                    res.question = q;
                    res.recommendationReason = "Due for spaced revision (Priority " + std::to_string(item.priority) + ")";
                    return res;
                }
            }
        }
    }

    // Retrieve user's catalog
    auto allQuestions = questionService_->getAllQuestions();
    std::vector<models::Question> candidates;
    for (const auto& q : allQuestions) {
        if (criteria.topic.has_value() && q.getTopic() != *criteria.topic) continue;
        if (criteria.difficulty.has_value() && q.getDifficulty() != *criteria.difficulty) continue;
        candidates.push_back(q);
    }

    if (candidates.empty()) {
        return res;
    }

    // Priority 3: Unsolved / Never practiced problems
    if (criteria.preferUnsolved) {
        // Group 3A: Unsolved and never practiced (last_practiced_at == 0)
        std::vector<models::Question> neverPracticed;
        for (const auto& q : candidates) {
            if (q.getStatus() == models::Status::Unsolved && q.getLastPracticedAt() == 0) {
                neverPracticed.push_back(q);
            }
        }
        if (!neverPracticed.empty()) {
            std::sort(neverPracticed.begin(), neverPracticed.end(), [](const models::Question& a, const models::Question& b) {
                if (a.getRevisionPriority() != b.getRevisionPriority()) {
                    return a.getRevisionPriority() < b.getRevisionPriority();
                }
                return a.getId() < b.getId();
            });
            res.hasQuestion = true;
            res.question = neverPracticed.front();
            res.recommendationReason = "Unsolved problem ready for initial practice";
            return res;
        }

        // Group 3B: In-Progress problems (or Unsolved practiced earlier)
        std::vector<models::Question> inProgress;
        for (const auto& q : candidates) {
            if (q.getStatus() == models::Status::InProgress || q.getStatus() == models::Status::Unsolved) {
                inProgress.push_back(q);
            }
        }
        if (!inProgress.empty()) {
            std::sort(inProgress.begin(), inProgress.end(), [](const models::Question& a, const models::Question& b) {
                if (a.getLastPracticedAt() != b.getLastPracticedAt()) {
                    return a.getLastPracticedAt() < b.getLastPracticedAt();
                }
                if (a.getRevisionPriority() != b.getRevisionPriority()) {
                    return a.getRevisionPriority() < b.getRevisionPriority();
                }
                return a.getId() < b.getId();
            });
            res.hasQuestion = true;
            res.question = inProgress.front();
            res.recommendationReason = "In-progress problem needing practice";
            return res;
        }
    }

    // Priority 4: Stale Solved/Mastered questions (Oldest practiced timestamp first)
    std::sort(candidates.begin(), candidates.end(), [](const models::Question& a, const models::Question& b) {
        if (a.getLastPracticedAt() != b.getLastPracticedAt()) {
            return a.getLastPracticedAt() < b.getLastPracticedAt();
        }
        if (a.getRevisionPriority() != b.getRevisionPriority()) {
            return a.getRevisionPriority() < b.getRevisionPriority();
        }
        return a.getId() < b.getId();
    });

    res.hasQuestion = true;
    res.question = candidates.front();
    res.recommendationReason = "Strengthen mastery (Oldest practiced problem)";
    return res;
}

SessionProgress PracticeService::getProgress() const {
    SessionProgress p;
    p.total = totalSessionQuestions_;
    p.completed = completedCount_;
    p.remaining = sessionQueue_.size();
    p.skipped = skippedCount_;
    return p;
}

size_t PracticeService::getRemainingCount() const noexcept {
    return sessionQueue_.size();
}

size_t PracticeService::getCompletedCount() const noexcept {
    return completedCount_;
}

size_t PracticeService::getTotalCount() const noexcept {
    return totalSessionQuestions_;
}

bool PracticeService::hasQuestions() const noexcept {
    return !sessionQueue_.empty();
}

void PracticeService::clearSession() noexcept {
    sessionQueue_.clear();
    totalSessionQuestions_ = 0;
    completedCount_ = 0;
    skippedCount_ = 0;
}

} // namespace codevault::services
