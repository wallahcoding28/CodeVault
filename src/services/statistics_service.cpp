#include "services/statistics_service.hpp"
#include "services/question_service.hpp"
#include "services/revision_service.hpp"

#include <algorithm>
#include <chrono>

namespace codevault::services {

StatisticsService::StatisticsService(
    std::shared_ptr<QuestionService> questionService,
    std::shared_ptr<RevisionService> revisionService)
    : questionService_(std::move(questionService)),
      revisionService_(std::move(revisionService)) {}

int64_t StatisticsService::resolveCurrentTime(int64_t explicitTime) const {
    if (explicitTime > 0) {
        return explicitTime;
    }
    if (revisionService_) {
        return revisionService_->now();
    }
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

double StatisticsService::calculatePercentage(size_t numerator, size_t denominator) noexcept {
    if (denominator == 0) {
        return 0.0;
    }
    return (static_cast<double>(numerator) * 100.0) / static_cast<double>(denominator);
}

models::DashboardSnapshot StatisticsService::getDashboardSnapshot(int64_t currentTime) const {
    std::vector<models::Question> questions;
    if (questionService_) {
        questions = questionService_->getAllQuestions();
    }
    return computeDashboardSnapshot(questions, currentTime);
}

models::DashboardSnapshot StatisticsService::computeDashboardSnapshot(
    const std::vector<models::Question>& questions,
    int64_t currentTime) const {
    int64_t resolvedTime = resolveCurrentTime(currentTime);

    models::DashboardSnapshot snapshot;
    snapshot.generatedAt = resolvedTime;
    snapshot.overall = computeOverallStatistics(questions, resolvedTime);
    snapshot.difficulty = computeDifficultyStatistics(questions);
    snapshot.topic = computeTopicStatistics(questions, false);
    snapshot.status = computeStatusStatistics(questions);
    snapshot.revision = computeRevisionStatistics(questions, resolvedTime);
    snapshot.practice = computePracticeStatistics(questions);

    return snapshot;
}

models::OverallStatistics StatisticsService::computeOverallStatistics(
    const std::vector<models::Question>& questions,
    int64_t currentTime) const {
    int64_t resolvedTime = resolveCurrentTime(currentTime);

    models::OverallStatistics stats;
    stats.totalQuestions = questions.size();

    for (const auto& q : questions) {
        switch (q.getStatus()) {
            case models::Status::Solved:
                ++stats.solvedCount;
                break;
            case models::Status::Mastered:
                ++stats.masteredCount;
                break;
            case models::Status::InProgress:
                ++stats.inProgressCount;
                break;
            case models::Status::Unsolved:
            default:
                ++stats.unsolvedCount;
                break;
        }

        if (q.isFavorite()) {
            ++stats.favoriteCount;
        }

        int64_t revAt = q.getNextRevisionAt();
        if (revAt > 0) {
            ++stats.totalScheduledCount;
            if (revAt <= resolvedTime) {
                ++stats.dueForRevisionCount;
            } else {
                ++stats.upcomingRevisionsCount;
            }
        }
    }

    // Formulas:
    // completionPercentage = (solved + mastered) / total * 100.0
    stats.completionPercentage = calculatePercentage(stats.solvedCount + stats.masteredCount, stats.totalQuestions);
    stats.solvedPercentage = calculatePercentage(stats.solvedCount, stats.totalQuestions);
    stats.masteredPercentage = calculatePercentage(stats.masteredCount, stats.totalQuestions);
    stats.inProgressPercentage = calculatePercentage(stats.inProgressCount, stats.totalQuestions);
    stats.unsolvedPercentage = calculatePercentage(stats.unsolvedCount, stats.totalQuestions);
    stats.favoritePercentage = calculatePercentage(stats.favoriteCount, stats.totalQuestions);

    return stats;
}

models::DifficultyStatistics StatisticsService::computeDifficultyStatistics(
    const std::vector<models::Question>& questions) const {
    models::DifficultyStatistics stats;
    stats.totalQuestions = questions.size();

    for (const auto& q : questions) {
        switch (q.getDifficulty()) {
            case models::Difficulty::Easy:
                ++stats.easyCount;
                break;
            case models::Difficulty::Medium:
                ++stats.mediumCount;
                break;
            case models::Difficulty::Hard:
                ++stats.hardCount;
                break;
            default:
                break;
        }
    }

    stats.easyPercentage = calculatePercentage(stats.easyCount, stats.totalQuestions);
    stats.mediumPercentage = calculatePercentage(stats.mediumCount, stats.totalQuestions);
    stats.hardPercentage = calculatePercentage(stats.hardCount, stats.totalQuestions);

    return stats;
}

models::TopicStatistics StatisticsService::computeTopicStatistics(
    const std::vector<models::Question>& questions,
    bool sortByCountDescending) const {
    models::TopicStatistics stats;
    stats.totalQuestions = questions.size();

    const std::vector<models::Topic> allTopics = {
        models::Topic::Arrays,
        models::Topic::Strings,
        models::Topic::LinkedLists,
        models::Topic::StacksQueues,
        models::Topic::Trees,
        models::Topic::Graphs,
        models::Topic::DynamicProgramming,
        models::Topic::BinarySearch,
        models::Topic::RecursionBacktracking,
        models::Topic::Greedy,
        models::Topic::Heaps,
        models::Topic::BitManipulation,
        models::Topic::MathGeometry,
        models::Topic::Other
    };

    stats.topicCounts.reserve(allTopics.size());
    for (const auto& t : allTopics) {
        models::TopicCount tc;
        tc.topic = t;
        tc.topicName = models::topicToString(t);
        tc.count = 0;
        tc.percentage = 0.0;
        stats.topicCounts.push_back(tc);
    }

    for (const auto& q : questions) {
        auto t = q.getTopic();
        for (auto& tc : stats.topicCounts) {
            if (tc.topic == t) {
                ++tc.count;
                break;
            }
        }
    }

    for (auto& tc : stats.topicCounts) {
        tc.percentage = calculatePercentage(tc.count, stats.totalQuestions);
        if (tc.count > 0) {
            ++stats.distinctTopicsCount;
        }
    }

    if (sortByCountDescending) {
        std::stable_sort(stats.topicCounts.begin(), stats.topicCounts.end(),
            [](const models::TopicCount& a, const models::TopicCount& b) {
                if (a.count != b.count) {
                    return a.count > b.count;
                }
                return static_cast<int>(a.topic) < static_cast<int>(b.topic);
            });
    }

    return stats;
}

models::StatusStatistics StatisticsService::computeStatusStatistics(
    const std::vector<models::Question>& questions) const {
    models::StatusStatistics stats;
    stats.totalQuestions = questions.size();

    for (const auto& q : questions) {
        switch (q.getStatus()) {
            case models::Status::Solved:
                ++stats.solvedCount;
                break;
            case models::Status::Mastered:
                ++stats.masteredCount;
                break;
            case models::Status::InProgress:
                ++stats.inProgressCount;
                break;
            case models::Status::Unsolved:
            default:
                ++stats.unsolvedCount;
                break;
        }
    }

    stats.unsolvedPercentage = calculatePercentage(stats.unsolvedCount, stats.totalQuestions);
    stats.inProgressPercentage = calculatePercentage(stats.inProgressCount, stats.totalQuestions);
    stats.solvedPercentage = calculatePercentage(stats.solvedCount, stats.totalQuestions);
    stats.masteredPercentage = calculatePercentage(stats.masteredCount, stats.totalQuestions);

    return stats;
}

models::RevisionStatistics StatisticsService::computeRevisionStatistics(
    const std::vector<models::Question>& questions,
    int64_t currentTime) const {
    int64_t resolvedTime = resolveCurrentTime(currentTime);

    models::RevisionStatistics stats;

    for (const auto& q : questions) {
        int64_t revAt = q.getNextRevisionAt();
        if (revAt > 0) {
            ++stats.scheduledCount;
            if (revAt <= resolvedTime) {
                ++stats.dueCount;
            } else {
                ++stats.upcomingCount;
            }

            int32_t prio = q.getRevisionPriority();
            if (prio >= 1 && prio <= 5) {
                ++stats.priorityCounts[prio];
            }

            int lvl = calculateLevelFromQuestion(q);
            if (lvl >= 1 && lvl <= 5) {
                ++stats.levelCounts[lvl];
            } else if (lvl == 0) {
                // If question has next_revision_at but last_practiced_at is 0,
                // treat as Level 1 for initial schedule
                ++stats.levelCounts[1];
            }
        } else {
            ++stats.unscheduledCount;
        }
    }

    stats.duePercentageOfScheduled = calculatePercentage(stats.dueCount, stats.scheduledCount);
    stats.scheduledPercentageOfTotal = calculatePercentage(stats.scheduledCount, questions.size());

    return stats;
}

models::PracticeStatistics StatisticsService::computePracticeStatistics(
    const std::vector<models::Question>& questions) const {
    models::PracticeStatistics stats;

    for (const auto& q : questions) {
        int64_t practicedAt = q.getLastPracticedAt();
        if (practicedAt > 0) {
            ++stats.practicedCount;
            if (practicedAt > stats.lastPracticedTimestamp) {
                stats.lastPracticedTimestamp = practicedAt;
            }
        } else {
            ++stats.unpracticedCount;
        }
    }

    stats.practicedPercentage = calculatePercentage(stats.practicedCount, questions.size());

    return stats;
}

} // namespace codevault::services
