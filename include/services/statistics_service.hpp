#pragma once

#include "models/question.hpp"
#include "models/statistics_models.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace codevault::services {

class QuestionService;
class RevisionService;

/**
 * @brief Application service responsible for computing deterministic progress and dashboard metrics.
 *
 * Designed according to Clean Architecture:
 * - Pure computation over Question collections and revision states.
 * - Zero terminal/CLI presentation coupling.
 * - Safe for consumption by CLI, future GUI, and Web frontends.
 */
class StatisticsService {
public:
    /**
     * @brief Construct StatisticsService with optional service dependencies.
     */
    explicit StatisticsService(
        std::shared_ptr<QuestionService> questionService = nullptr,
        std::shared_ptr<RevisionService> revisionService = nullptr);

    /**
     * @brief Retrieve complete snapshot using configured QuestionService and RevisionService.
     * @param currentTime Reference epoch timestamp (seconds). If <= 0, uses RevisionService/system clock.
     */
    models::DashboardSnapshot getDashboardSnapshot(int64_t currentTime = 0) const;

    /**
     * @brief Compute complete snapshot from an explicit question collection.
     */
    models::DashboardSnapshot computeDashboardSnapshot(
        const std::vector<models::Question>& questions,
        int64_t currentTime = 0) const;

    /**
     * @brief Calculate overall progress metrics across the collection.
     */
    models::OverallStatistics computeOverallStatistics(
        const std::vector<models::Question>& questions,
        int64_t currentTime = 0) const;

    /**
     * @brief Calculate difficulty distribution (counts and percentages for Easy, Medium, Hard).
     */
    models::DifficultyStatistics computeDifficultyStatistics(
        const std::vector<models::Question>& questions) const;

    /**
     * @brief Calculate topic breakdown across all defined curriculum categories.
     * @param sortByCountDescending If true, orders topic breakdown from most frequent to least frequent.
     */
    models::TopicStatistics computeTopicStatistics(
        const std::vector<models::Question>& questions,
        bool sortByCountDescending = false) const;

    /**
     * @brief Calculate question counts and percentages for each lifecycle status.
     */
    models::StatusStatistics computeStatusStatistics(
        const std::vector<models::Question>& questions) const;

    /**
     * @brief Calculate revision scheduling metrics (due, upcoming, unscheduled, priorities, levels).
     */
    models::RevisionStatistics computeRevisionStatistics(
        const std::vector<models::Question>& questions,
        int64_t currentTime = 0) const;

    /**
     * @brief Calculate practice attempt metrics (practiced count, unpracticed count, latest timestamp).
     */
    models::PracticeStatistics computePracticeStatistics(
        const std::vector<models::Question>& questions) const;

    /**
     * @brief Dependency injection setters for application integration.
     */
    void setQuestionService(std::shared_ptr<QuestionService> questionService) noexcept {
        questionService_ = std::move(questionService);
    }

    void setRevisionService(std::shared_ptr<RevisionService> revisionService) noexcept {
        revisionService_ = std::move(revisionService);
    }

    /**
     * @brief Safe percentage calculation helper: (numerator / denominator) * 100.0 with 0-division guard.
     */
    static double calculatePercentage(size_t numerator, size_t denominator) noexcept;

private:
    int64_t resolveCurrentTime(int64_t explicitTime) const;

    std::shared_ptr<QuestionService> questionService_;
    std::shared_ptr<RevisionService> revisionService_;
};

} // namespace codevault::services
