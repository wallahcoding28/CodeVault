#pragma once

#include "dsa/min_heap.hpp"
#include "models/enums.hpp"
#include "models/practice_result.hpp"
#include "models/question.hpp"
#include "models/revision_item.hpp"
#include "utils/clock.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace codevault::services {

/**
 * @brief Seconds per standard day (86,400s).
 */
constexpr int64_t SECONDS_PER_DAY = 86400;

/**
 * @brief Deterministic revision interval calculation for CodeVault's initial schedule.
 *
 * Schedule:
 * - Level 1: 1 day (86,400s)
 * - Level 2: 3 days (259,200s)
 * - Level 3: 7 days (604,800s)
 * - Level 4: 14 days (1,209,600s)
 * - Level 5+: 30 days (2,592,000s)
 */
inline int64_t getIntervalForLevel(int level) noexcept {
    if (level <= 1) return 1 * SECONDS_PER_DAY;
    if (level == 2) return 3 * SECONDS_PER_DAY;
    if (level == 3) return 7 * SECONDS_PER_DAY;
    if (level == 4) return 14 * SECONDS_PER_DAY;
    return 30 * SECONDS_PER_DAY;
}

/**
 * @brief Determine approximate revision level from last practice and scheduled next revision.
 */
inline int calculateLevelFromQuestion(const models::Question& question) noexcept {
    if (question.getLastPracticedAt() <= 0 || question.getNextRevisionAt() <= question.getLastPracticedAt()) {
        return 0;
    }
    int64_t delta = question.getNextRevisionAt() - question.getLastPracticedAt();
    if (delta <= 1 * SECONDS_PER_DAY + 3600) return 1;
    if (delta <= 3 * SECONDS_PER_DAY + 3600) return 2;
    if (delta <= 7 * SECONDS_PER_DAY + 3600) return 3;
    if (delta <= 14 * SECONDS_PER_DAY + 3600) return 4;
    return 5;
}

/**
 * @brief Detailed outcome of calculating next revision schedule.
 */
struct RevisionScheduleResult {
    int nextLevel{1};
    int64_t nextRevisionAt{0};
    int32_t revisionPriority{2};
    int64_t lastPracticedAt{0};
    models::Status newStatus{models::Status::Unsolved};
    int64_t intervalSeconds{86400};
};

/**
 * @brief Manages spaced repetition and review prioritization using a custom MinHeap.
 *
 * MinHeap ordering:
 * 1. Earliest scheduled timestamp (nextRevisionAt)
 * 2. Revision priority (1 = Highest urgency, 5 = Lowest urgency)
 * 3. Question ID deterministic tie-breaker
 *
 * Complexities:
 * - Insert / Schedule: O(log N)
 * - Remove / Extract min: O(log N)
 * - Inspect next due: O(1)
 */
class RevisionService {
public:
    explicit RevisionService(std::shared_ptr<utils::IClock> clock = nullptr);

    /**
     * @brief Schedule a question for revision with a timestamp and priority.
     */
    void scheduleQuestion(const std::string& questionId, int64_t nextRevisionAt, int32_t priority = 2);

    /**
     * @brief Reschedule an existing question (updates or inserts).
     */
    void rescheduleQuestion(const std::string& questionId, int64_t nextRevisionAt, int32_t priority = 2);

    /**
     * @brief Remove a question from the revision queue.
     */
    void removeQuestion(const std::string& questionId);

    /**
     * @brief Schedule a new question revision item into the priority heap (Stage 3 compat).
     */
    void scheduleRevision(const models::RevisionItem& item);

    /**
     * @brief Populate the revision heap from existing questions that have next_revision_at > 0.
     */
    void loadFromQuestions(const std::vector<models::Question>& questions);

    /**
     * @brief Calculate the next revision state given the question and verdict.
     */
    RevisionScheduleResult calculateNextSchedule(
        const models::Question& question,
        models::PracticeVerdict verdict,
        int64_t currentTime = -1) const;

    /**
     * @brief Apply a practice result: updates question fields and reschedules in MinHeap.
     */
    RevisionScheduleResult markRevisionResult(
        models::Question& question,
        models::PracticeVerdict verdict,
        int64_t currentTime = -1);

    /**
     * @brief Retrieve all questions currently due (nextRevisionAt <= asOfTime), ordered by priority.
     */
    std::vector<models::RevisionItem> getDueQuestions(int64_t asOfTime = -1) const;

    /**
     * @brief Retrieve upcoming scheduled revisions in priority order.
     * @param limit Maximum number of items to return.
     * @param asOfTime Filter for future revisions (nextRevisionAt > asOfTime). If -1, uses now().
     */
    std::vector<models::RevisionItem> getUpcomingRevisions(size_t limit = 10, int64_t asOfTime = -1) const;

    /**
     * @brief Peek at the earliest scheduled revision item without removing it.
     */
    std::optional<models::RevisionItem> getNextDueRevision() const;

    /**
     * @brief Peek at the earliest scheduled revision item if it is due (nextRevisionAt <= asOfTime).
     */
    std::optional<models::RevisionItem> getNextDueRevision(int64_t asOfTime) const;

    /**
     * @brief Extract and remove the earliest scheduled revision item (synonym for popNextDueRevision).
     */
    std::optional<models::RevisionItem> nextRevision();

    /**
     * @brief Extract and remove the earliest scheduled revision item.
     */
    std::optional<models::RevisionItem> popNextDueRevision();

    /**
     * @brief Total count of scheduled items in the revision heap.
     */
    size_t getDueRevisionsCount() const noexcept;

    /**
     * @brief Count of items that are strictly due (nextRevisionAt <= asOfTime).
     */
    size_t getDueCount(int64_t asOfTime = -1) const;

    /**
     * @brief Total scheduled revisions count.
     */
    size_t getTotalScheduledCount() const noexcept;

    /**
     * @brief Check whether any revisions are scheduled in the heap.
     */
    bool hasDueRevisions() const noexcept;

    /**
     * @brief Check whether any revisions are due as of the given timestamp.
     */
    bool hasDueRevisions(int64_t asOfTime) const;

    /**
     * @brief Clear all scheduled revision items.
     */
    void clear() noexcept;

    /**
     * @brief Current time from the configured clock.
     */
    int64_t now() const;

    /**
     * @brief Inject a mock or custom clock for testing.
     */
    void setClock(std::shared_ptr<utils::IClock> clock);

    /**
     * @brief Get the configured clock.
     */
    std::shared_ptr<utils::IClock> getClock() const noexcept;

private:
    dsa::MinHeap<models::RevisionItem, models::RevisionItemComparator> heap_;
    std::shared_ptr<utils::IClock> clock_;
};

} // namespace codevault::services
