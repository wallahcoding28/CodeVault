#pragma once

#include "models/enums.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace codevault::models {

/**
 * @brief Breakdown counts and percentages for an individual difficulty level.
 */
struct DifficultyCount {
    Difficulty difficulty{Difficulty::Unknown};
    std::string difficultyName;
    size_t count{0};
    double percentage{0.0}; // 0.0 to 100.0
};

/**
 * @brief Aggregated difficulty distribution across Easy, Medium, and Hard questions.
 */
struct DifficultyStatistics {
    size_t easyCount{0};
    size_t mediumCount{0};
    size_t hardCount{0};

    double easyPercentage{0.0};
    double mediumPercentage{0.0};
    double hardPercentage{0.0};

    size_t totalQuestions{0};
};

/**
 * @brief Breakdown counts and percentages for an individual topic.
 */
struct TopicCount {
    Topic topic{Topic::Other};
    std::string topicName;
    size_t count{0};
    double percentage{0.0}; // 0.0 to 100.0
};

/**
 * @brief Aggregated topic distribution across all defined curriculum categories.
 */
struct TopicStatistics {
    std::vector<TopicCount> topicCounts; // Enumerated or sorted distribution
    size_t totalQuestions{0};
    size_t distinctTopicsCount{0};       // Topics with at least 1 question
};

/**
 * @brief Distribution across all lifecycle statuses: Unsolved, InProgress, Solved, Mastered.
 */
struct StatusStatistics {
    size_t unsolvedCount{0};
    size_t inProgressCount{0};
    size_t solvedCount{0};
    size_t masteredCount{0};

    double unsolvedPercentage{0.0};
    double inProgressPercentage{0.0};
    double solvedPercentage{0.0};
    double masteredPercentage{0.0};

    size_t totalQuestions{0};
};

/**
 * @brief Revision metrics derived from next_revision_at, priorities, and revision intervals.
 */
struct RevisionStatistics {
    size_t dueCount{0};         // next_revision_at > 0 && next_revision_at <= currentTime
    size_t upcomingCount{0};    // next_revision_at > currentTime
    size_t scheduledCount{0};   // dueCount + upcomingCount
    size_t unscheduledCount{0}; // next_revision_at <= 0

    // Priority counts: index 1 to 5 (1 = highest urgency / urgent, 5 = lowest urgency)
    size_t priorityCounts[6]{0, 0, 0, 0, 0, 0};

    // Revision Level counts: index 1 to 5 (Level 1: 1d, Level 2: 3d, Level 3: 7d, Level 4: 14d, Level 5: 30d+)
    size_t levelCounts[6]{0, 0, 0, 0, 0, 0};

    double duePercentageOfScheduled{0.0};
    double scheduledPercentageOfTotal{0.0};
};

/**
 * @brief Practice attempt statistics derived from last_practiced_at timestamps.
 */
struct PracticeStatistics {
    size_t practicedCount{0};          // last_practiced_at > 0
    size_t unpracticedCount{0};        // last_practiced_at == 0
    double practicedPercentage{0.0};   // practicedCount / totalQuestions * 100.0
    int64_t lastPracticedTimestamp{0}; // Most recent practice attempt across all questions
};

/**
 * @brief High-level summary metrics for overall curriculum progress.
 */
struct OverallStatistics {
    size_t totalQuestions{0};
    size_t solvedCount{0};
    size_t inProgressCount{0};
    size_t unsolvedCount{0};
    size_t masteredCount{0};
    size_t favoriteCount{0};

    size_t dueForRevisionCount{0};
    size_t upcomingRevisionsCount{0};
    size_t totalScheduledCount{0};

    // Progress metrics (guaranteed zero-division safe)
    // Completion formula: (solvedCount + masteredCount) / totalQuestions * 100.0
    double completionPercentage{0.0};
    double solvedPercentage{0.0};
    double masteredPercentage{0.0};
    double inProgressPercentage{0.0};
    double unsolvedPercentage{0.0};
    double favoritePercentage{0.0};
};

/**
 * @brief Complete immutable dashboard snapshot containing all calculated categories.
 *
 * This object is designed to be cleanly serializable and consumable by:
 * - Terminal CLI presentation
 * - Future desktop GUIs (Qt / ImGui)
 * - Future Web/REST API backends
 */
struct DashboardSnapshot {
    int64_t generatedAt{0};
    OverallStatistics overall;
    DifficultyStatistics difficulty;
    TopicStatistics topic;
    StatusStatistics status;
    RevisionStatistics revision;
    PracticeStatistics practice;
};

} // namespace codevault::models
