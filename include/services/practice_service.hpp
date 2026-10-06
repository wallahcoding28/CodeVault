#pragma once

#include "dsa/queue.hpp"
#include "models/practice_next.hpp"
#include "models/practice_result.hpp"
#include "models/question_filter.hpp"
#include "services/revision_service.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace codevault::services {

class QuestionService;

/**
 * @brief Tracks current progress statistics for an active practice session.
 */
struct SessionProgress {
    size_t total{0};
    size_t completed{0};
    size_t remaining{0};
    size_t skipped{0};
};

/**
 * @brief Manages daily/targeted practice sessions using a custom FIFO Queue.
 *
 * Guaranteed FIFO ordering:
 * - Questions practiced in the order they were scheduled / selected.
 * - Skipped questions cycle to the back of the queue.
 * - Completed/reviewed questions are dequeued from the front.
 */
class PracticeService {
public:
    explicit PracticeService(
        std::shared_ptr<QuestionService> questionService = nullptr,
        std::shared_ptr<RevisionService> revisionService = nullptr);

    /**
     * @brief Start a practice session by loading question IDs in FIFO order.
     */
    void startSession(const std::vector<std::string>& questionIds);

    /**
     * @brief Start a targeted practice session filtered by criteria (topic, difficulty, status, etc.).
     * @param filter Criteria specifications to match questions against.
     * @param dueOnly If true, limits session strictly to questions currently due for spaced revision.
     */
    void startSessionWithFilter(const models::QuestionFilter& filter, bool dueOnly = false);

    /**
     * @brief Enqueue a single question to the end of the current practice session.
     */
    void enqueueQuestion(const std::string& questionId);

    /**
     * @brief Retrieve all question IDs currently in the practice queue in FIFO order.
     */
    std::vector<std::string> getQueueQuestionIds() const;

    /**
     * @brief Retrieve all Question models currently in the practice queue in FIFO order.
     */
    std::vector<models::Question> getQueueQuestions() const;

    /**
     * @brief Remove a specific question from the practice queue if present.
     * @return true if removed, false if question was not in the queue.
     */
    bool removeQuestionFromQueue(const std::string& questionId);

    /**
     * @brief Check whether a specific question is currently in the practice queue.
     */
    bool isQuestionQueued(const std::string& questionId) const;

    /**
     * @brief View the next question to be practiced without removing it.
     */
    std::optional<std::string> getCurrentQuestion() const;

    /**
     * @brief Complete/process the current question (dequeues front).
     * @return The completed question ID, or nullopt if session was empty.
     */
    std::optional<std::string> completeCurrentQuestion();

    /**
     * @brief Skip the current question: removes it from the front and enqueues to the back.
     * @return true if skipped, false if session has 0 or 1 question.
     */
    bool skipCurrentQuestion();

    /**
     * @brief Advance to next question (synonym for completeCurrentQuestion).
     */
    std::optional<std::string> advanceToNextQuestion();

    /**
     * @brief Submit a verdict (Solved, NeedsReview, Skipped) for the current question at queue head.
     *
     * Coordinates with RevisionService and QuestionService to update domain state,
     * persist to storage, and advance the queue.
     */
    std::optional<RevisionScheduleResult> recordPracticeAttempt(
        models::PracticeVerdict verdict,
        int64_t currentTime = -1);

    /**
     * @brief Submit a verdict directly for any specific question (e.g. from Problem Detail workspace).
     *
     * Updates status, timestamps, and revision schedule via RevisionService & QuestionService.
     * If the question is currently present in the practice queue, removes or advances it cleanly.
     */
    std::optional<RevisionScheduleResult> recordPracticeAttemptForQuestion(
        const std::string& questionId,
        models::PracticeVerdict verdict,
        int64_t currentTime = -1);

    /**
     * @brief Deterministically recommend the next best problem to practice.
     *
     * Prioritization hierarchy:
     * 1. Active practice session queue head (if matching criteria)
     * 2. Overdue spaced revision items from MinHeap (earliest due + highest urgency priority)
     * 3. Unsolved / never-practiced problems matching criteria
     * 4. In-progress problems matching criteria
     * 5. Oldest-practiced solved/mastered problems for retention reinforcement
     *
     * @param criteria Optional topic and difficulty filters, and preference flags.
     * @param asOfTime Evaluation timestamp (defaults to current system time).
     * @return PracticeNextResult containing the question and an explainable recommendation reason.
     */
    models::PracticeNextResult getPracticeNext(
        const models::PracticeNextCriteria& criteria = {},
        int64_t asOfTime = -1) const;

    /**
     * @brief Query current progress metrics.
     */
    SessionProgress getProgress() const;

    /**
     * @brief Number of questions remaining in the practice session.
     */
    size_t getRemainingCount() const noexcept;

    /**
     * @brief Number of questions completed in this session.
     */
    size_t getCompletedCount() const noexcept;

    /**
     * @brief Total number of questions initially in or added to this session.
     */
    size_t getTotalCount() const noexcept;

    /**
     * @brief Check whether any questions remain in the session.
     */
    bool hasQuestions() const noexcept;

    /**
     * @brief Clear and reset the practice session.
     */
    void clearSession() noexcept;

    void setQuestionService(std::shared_ptr<QuestionService> qs) {
        questionService_ = std::move(qs);
    }

    void setRevisionService(std::shared_ptr<RevisionService> rs) {
        revisionService_ = std::move(rs);
    }

private:
    dsa::Queue<std::string> sessionQueue_;
    size_t totalSessionQuestions_{0};
    size_t completedCount_{0};
    size_t skippedCount_{0};

    std::shared_ptr<QuestionService> questionService_;
    std::shared_ptr<RevisionService> revisionService_;
};

} // namespace codevault::services
