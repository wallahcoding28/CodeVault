#pragma once

#include "models/question.hpp"
#include "models/revision_item.hpp"
#include "services/practice_service.hpp"
#include "services/question_service.hpp"
#include "services/recent_history_service.hpp"
#include "services/revision_service.hpp"
#include "services/search_service.hpp"
#include "services/statistics_service.hpp"

#include <memory>
#include <string>
#include <vector>

namespace codevault::app {

/**
 * @brief Terminal user interface shell driving interactive navigation and commands.
 */
class CliShell {
public:
    explicit CliShell(
        std::shared_ptr<services::QuestionService> questionService,
        std::shared_ptr<services::RecentHistoryService> recentHistoryService = nullptr,
        std::shared_ptr<services::SearchService> searchService = nullptr,
        std::shared_ptr<services::RevisionService> revisionService = nullptr,
        std::shared_ptr<services::PracticeService> practiceService = nullptr,
        std::shared_ptr<services::StatisticsService> statisticsService = nullptr);

    /**
     * @brief Run the interactive REPL menu loop.
     */
    void run();

    /**
     * @brief Render the main application header banner.
     */
    static void printHeader();

    /**
     * @brief Render the primary command menu options.
     */
    static void printMainMenu();

    /**
     * @brief Render the questions sub-menu options.
     */
    static void printQuestionsMenu();

    /**
     * @brief Render the search, filter, and sort sub-menu options.
     */
    static void printSearchMenu();

    /**
     * @brief Render the practice sub-menu options.
     */
    static void printPracticeMenu();

    /**
     * @brief Render the revision sub-menu options.
     */
    static void printRevisionMenu();

    /**
     * @brief Reusable formatted table display for question collections.
     */
    static void printQuestionsTable(const std::vector<models::Question>& questions);

    /**
     * @brief Reusable formatted table display for revision items.
     */
    void printRevisionTable(const std::vector<models::RevisionItem>& items, bool showDueStatus = false) const;

    /**
     * @brief Render text-based progress bar (e.g. [##########----------] 50.0%).
     */
    static std::string renderProgressBar(double percentage, int width = 20);

private:
    std::shared_ptr<services::QuestionService> questionService_;
    std::shared_ptr<services::RecentHistoryService> recentHistoryService_;
    std::shared_ptr<services::SearchService> searchService_;
    std::shared_ptr<services::RevisionService> revisionService_;
    std::shared_ptr<services::PracticeService> practiceService_;
    std::shared_ptr<services::StatisticsService> statisticsService_;
    bool isRunning_{true};

    // Submenu routers
    void handleQuestionsSubmenu();
    void handleSearchSubmenu();
    void handlePracticeSubmenu();
    void handleRevisionSubmenu();

    // Question operations
    void handleAddQuestion();
    void handleViewAllQuestions();
    void handleViewQuestionById();
    void handleEditQuestion();
    void handleDeleteQuestion();

    // Search, filter, and sort operations
    void handleSearchByTitlePrefix();
    void handleSearchByKeyword();
    void handleFilterQuestions();
    void handleSortQuestions();
    void handleCombinedSearchFilterSort();

    // Practice operations
    void handlePracticeAllUnsolved();
    void handlePracticeByTopic();
    void handlePracticeByDifficulty();
    void handlePracticeFavorites();
    void handlePracticeDueRevisions();
    void runPracticeSession();

    // Revision operations
    void handleShowDueQuestions();
    void handleShowUpcomingRevisions();
    void handleShowNextRevision();
    void handleStartDueRevisionSession();
    void handleScheduleQuestion();

    // General handlers
    void handleStatistics();
    void handleSettings();

    // Dashboard presentation helpers
    void printDashboard(const models::DashboardSnapshot& snapshot) const;

    // Input helpers
    static std::string readLine(const std::string& prompt);
    static std::string readLineWithDefault(const std::string& prompt, const std::string& defaultVal);
    static bool readConfirmation(const std::string& prompt, bool defaultYes = false);
};

} // namespace codevault::app
