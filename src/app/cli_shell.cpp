#include "app/cli_shell.hpp"
#include "models/enums.hpp"
#include "models/practice_result.hpp"
#include "models/question.hpp"
#include "models/question_filter.hpp"
#include "models/sort_options.hpp"
#include "services/practice_service.hpp"
#include "services/question_service.hpp"
#include "services/revision_service.hpp"
#include "services/statistics_service.hpp"
#include "utils/datetime.hpp"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace codevault::app {

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

static int64_t currentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

CliShell::CliShell(
    std::shared_ptr<services::QuestionService> questionService,
    std::shared_ptr<services::RecentHistoryService> recentHistoryService,
    std::shared_ptr<services::SearchService> searchService,
    std::shared_ptr<services::RevisionService> revisionService,
    std::shared_ptr<services::PracticeService> practiceService,
    std::shared_ptr<services::StatisticsService> statisticsService)
    : questionService_(std::move(questionService)),
      recentHistoryService_(std::move(recentHistoryService)),
      searchService_(std::move(searchService)),
      revisionService_(std::move(revisionService)),
      practiceService_(std::move(practiceService)),
      statisticsService_(std::move(statisticsService)) {
    if (!recentHistoryService_) {
        recentHistoryService_ = std::make_shared<services::RecentHistoryService>();
    }
    if (!searchService_ && questionService_) {
        searchService_ = questionService_->getSearchService();
    }
    if (!searchService_) {
        searchService_ = std::make_shared<services::SearchService>();
    }
    if (!revisionService_ && questionService_) {
        revisionService_ = questionService_->getRevisionService();
    }
    if (!revisionService_) {
        revisionService_ = std::make_shared<services::RevisionService>();
        if (questionService_) {
            revisionService_->loadFromQuestions(questionService_->getAllQuestions());
        }
    }
    if (!practiceService_) {
        practiceService_ = std::make_shared<services::PracticeService>(questionService_, revisionService_);
    }
    if (!statisticsService_) {
        statisticsService_ = std::make_shared<services::StatisticsService>(questionService_, revisionService_);
    }
}

void CliShell::printHeader() {
    std::cout << "\n================================\n";
    std::cout << "          CODEVAULT\n";
    std::cout << "================================\n";
}

void CliShell::printMainMenu() {
    std::cout << "1. Questions\n";
    std::cout << "2. Search, Filter & Sort\n";
    std::cout << "3. Practice\n";
    std::cout << "4. Revision\n";
    std::cout << "5. Dashboard & Statistics\n";
    std::cout << "6. Settings\n";
    std::cout << "0. Exit\n";
    std::cout << "--------------------------------\n";
    std::cout << "Enter choice: ";
}

void CliShell::printQuestionsMenu() {
    std::cout << "\n--------------------------------\n";
    std::cout << "Questions\n";
    std::cout << "--------------------------------\n";
    std::cout << "1. Add Question\n";
    std::cout << "2. View All Questions\n";
    std::cout << "3. View Question by ID\n";
    std::cout << "4. Edit Question\n";
    std::cout << "5. Delete Question\n";
    std::cout << "0. Back\n";
    std::cout << "--------------------------------\n";
    std::cout << "Enter choice: ";
}

void CliShell::printSearchMenu() {
    std::cout << "\n================================\n";
    std::cout << "        SEARCH & FILTER         \n";
    std::cout << "================================\n";
    std::cout << "1. Search by title prefix (Trie)\n";
    std::cout << "2. Search by keyword (All fields)\n";
    std::cout << "3. Filter questions\n";
    std::cout << "4. Sort questions (Merge / Quick Sort)\n";
    std::cout << "5. Search + Filter + Sort\n";
    std::cout << "6. Reset / Show all\n";
    std::cout << "0. Back\n";
    std::cout << "--------------------------------\n";
    std::cout << "Enter choice: ";
}

void CliShell::printPracticeMenu() {
    std::cout << "\n================================\n";
    std::cout << "        PRACTICE SESSION        \n";
    std::cout << "================================\n";
    std::cout << "1. Practice all unsolved\n";
    std::cout << "2. Practice by topic\n";
    std::cout << "3. Practice by difficulty\n";
    std::cout << "4. Practice favorites\n";
    std::cout << "5. Practice due revisions\n";
    std::cout << "0. Back\n";
    std::cout << "--------------------------------\n";
    std::cout << "Enter choice: ";
}

void CliShell::printRevisionMenu() {
    std::cout << "\n================================\n";
    std::cout << "            REVISION            \n";
    std::cout << "================================\n";
    std::cout << "1. Show Due Questions\n";
    std::cout << "2. Show Upcoming Revisions\n";
    std::cout << "3. Show Next Revision\n";
    std::cout << "4. Start Due Revision Session\n";
    std::cout << "5. Schedule Question\n";
    std::cout << "0. Back\n";
    std::cout << "--------------------------------\n";
    std::cout << "Enter choice: ";
}

void CliShell::printRevisionTable(const std::vector<models::RevisionItem>& items, bool showDueStatus) const {
    if (items.empty()) {
        std::cout << "\nNo revision items found.\n";
        return;
    }

    std::cout << "\n"
              << std::left
              << std::setw(10) << "ID"
              << std::setw(30) << "Title"
              << std::setw(16) << "Topic"
              << std::setw(12) << "Priority"
              << std::setw(20) << "Next Revision"
              << (showDueStatus ? "Status" : "") << "\n";
    std::cout << std::string(showDueStatus ? 96 : 88, '-') << "\n";

    int64_t curTime = revisionService_ ? revisionService_->now() : currentTimestamp();

    for (const auto& item : items) {
        std::string title = item.questionId;
        std::string topicStr = "-";
        if (questionService_) {
            auto qOpt = questionService_->getQuestionById(item.questionId);
            if (qOpt.has_value()) {
                title = qOpt->getTitle();
                topicStr = models::topicToString(qOpt->getTopic());
            }
        }
        if (title.length() > 28) {
            title = title.substr(0, 25) + "...";
        }

        std::string dateStr = utils::formatTimestamp(item.nextRevisionAt);
        std::string dueStr = "";
        if (showDueStatus) {
            dueStr = (item.nextRevisionAt <= curTime) ? "[DUE NOW]" : "Scheduled";
        }

        std::string prioLabel = std::to_string(item.priority);
        if (item.priority == 1) prioLabel += " (Urgent)";

        std::cout << std::left
                  << std::setw(10) << item.questionId
                  << std::setw(30) << title
                  << std::setw(16) << topicStr
                  << std::setw(12) << prioLabel
                  << std::setw(20) << dateStr
                  << dueStr << "\n";
    }
    std::cout << std::string(showDueStatus ? 96 : 88, '-') << "\n";
    std::cout << "Total Items: " << items.size() << "\n";
}

void CliShell::printQuestionsTable(const std::vector<models::Question>& questions) {
    if (questions.empty()) {
        std::cout << "\nNo questions matched the selected criteria.\n";
        return;
    }

    std::cout << "\n"
              << std::left
              << std::setw(10) << "ID"
              << std::setw(30) << "Title"
              << std::setw(18) << "Topic"
              << std::setw(12) << "Difficulty"
              << std::setw(14) << "Company"
              << std::setw(13) << "Status"
              << "Fav\n";
    std::cout << std::string(102, '-') << "\n";

    for (const auto& q : questions) {
        std::string displayTitle = q.getTitle();
        if (displayTitle.length() > 28) {
            displayTitle = displayTitle.substr(0, 25) + "...";
        }

        std::string displayCompany = q.getCompany();
        if (displayCompany.length() > 12) {
            displayCompany = displayCompany.substr(0, 9) + "...";
        }
        if (displayCompany.empty()) {
            displayCompany = "-";
        }

        std::cout << std::left
                  << std::setw(10) << q.getId()
                  << std::setw(30) << displayTitle
                  << std::setw(18) << models::topicToString(q.getTopic())
                  << std::setw(12) << models::difficultyToString(q.getDifficulty())
                  << std::setw(14) << displayCompany
                  << std::setw(13) << models::statusToString(q.getStatus())
                  << (q.isFavorite() ? "[*]" : " - ") << "\n";
    }
    std::cout << std::string(102, '-') << "\n";
    std::cout << "Total Questions: " << questions.size() << "\n";
}

std::string CliShell::readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return "";
    }
    return trim(line);
}

std::string CliShell::readLineWithDefault(const std::string& prompt, const std::string& defaultVal) {
    std::cout << prompt << " [" << defaultVal << "]: ";
    std::string line;
    if (!std::getline(std::cin, line)) {
        return defaultVal;
    }
    std::string trimmed = trim(line);
    return trimmed.empty() ? defaultVal : trimmed;
}

bool CliShell::readConfirmation(const std::string& prompt, bool defaultYes) {
    std::cout << prompt << (defaultYes ? " [Y/n]: " : " [y/N]: ");
    std::string line;
    if (!std::getline(std::cin, line)) {
        return defaultYes;
    }
    std::string trimmed = trim(line);
    if (trimmed.empty()) {
        return defaultYes;
    }
    return (trimmed[0] == 'y' || trimmed[0] == 'Y');
}

void CliShell::run() {
    printHeader();
    std::cout << "Welcome to CodeVault\n";
    std::cout << "Loaded Questions: " << (questionService_ ? questionService_->getQuestionCount() : 0) << "\n";

    std::string input;
    while (isRunning_) {
        printHeader();
        printMainMenu();

        if (!std::getline(std::cin, input)) {
            break;
        }

        std::string choice = trim(input);
        if (choice.empty()) {
            continue;
        }

        switch (choice[0]) {
            case '1':
                handleQuestionsSubmenu();
                break;
            case '2':
                handleSearchSubmenu();
                break;
            case '3':
                handlePracticeSubmenu();
                break;
            case '4':
                handleRevisionSubmenu();
                break;
            case '5':
                handleStatistics();
                break;
            case '6':
                handleSettings();
                break;
            case '0':
                std::cout << "\nExiting CodeVault. Happy Coding!\n";
                isRunning_ = false;
                break;
            default:
                std::cout << "\n[!] Invalid option. Please select 0-6.\n";
                break;
        }
    }
}

void CliShell::handleQuestionsSubmenu() {
    bool inSubmenu = true;
    std::string input;

    while (inSubmenu && isRunning_) {
        printQuestionsMenu();

        if (!std::getline(std::cin, input)) {
            break;
        }

        std::string choice = trim(input);
        if (choice.empty()) {
            continue;
        }

        switch (choice[0]) {
            case '1':
                handleAddQuestion();
                break;
            case '2':
                handleViewAllQuestions();
                break;
            case '3':
                handleViewQuestionById();
                break;
            case '4':
                handleEditQuestion();
                break;
            case '5':
                handleDeleteQuestion();
                break;
            case '0':
                inSubmenu = false;
                break;
            default:
                std::cout << "\n[!] Invalid option. Please select 0-5.\n";
                break;
        }
    }
}

void CliShell::handleSearchSubmenu() {
    bool inSubmenu = true;
    std::string input;

    while (inSubmenu && isRunning_) {
        printSearchMenu();

        if (!std::getline(std::cin, input)) {
            break;
        }

        std::string choice = trim(input);
        if (choice.empty()) {
            continue;
        }

        switch (choice[0]) {
            case '1':
                handleSearchByTitlePrefix();
                break;
            case '2':
                handleSearchByKeyword();
                break;
            case '3':
                handleFilterQuestions();
                break;
            case '4':
                handleSortQuestions();
                break;
            case '5':
                handleCombinedSearchFilterSort();
                break;
            case '6':
                handleViewAllQuestions();
                break;
            case '0':
                inSubmenu = false;
                break;
            default:
                std::cout << "\n[!] Invalid option. Please select 0-6.\n";
                break;
        }
    }
}

void CliShell::handleAddQuestion() {
    std::cout << "\n--- Add New Question ---\n";
    if (!questionService_) {
        std::cout << "[!] Question service is not initialized.\n";
        return;
    }

    std::string autoId = questionService_->generateNextId();
    std::string id = readLineWithDefault("ID (press Enter for auto-generated)", autoId);

    std::string title = readLine("Title: ");
    if (title.empty()) {
        std::cout << "[!] Question creation cancelled: Title cannot be empty.\n";
        return;
    }

    std::string description = readLine("Description: ");
    std::string topicStr = readLineWithDefault("Topic [Arrays/Strings/LinkedLists/Trees/Graphs/DynamicProgramming/etc.]", "Arrays");
    std::string diffStr = readLineWithDefault("Difficulty [Easy/Medium/Hard]", "Medium");
    std::string company = readLine("Company: ");
    std::string platformStr = readLineWithDefault("Platform [LeetCode/HackerRank/Codeforces/GeeksforGeeks/Custom]", "LeetCode");
    std::string sourceUrl = readLine("Source URL: ");
    std::string statusStr = readLineWithDefault("Status [Unsolved/InProgress/Solved/Mastered]", "Unsolved");
    bool isFav = readConfirmation("Favorite", false);
    std::string notes = readLine("Notes: ");
    std::string tagsStr = readLine("Tags (comma or semicolon separated): ");

    auto topic = models::stringToTopic(topicStr);
    auto diff = models::stringToDifficulty(diffStr);
    auto platform = models::stringToPlatform(platformStr);
    auto status = models::stringToStatus(statusStr);

    std::vector<std::string> tags;
    std::string tagToken;
    for (char ch : tagsStr) {
        if (ch == ',' || ch == ';') {
            std::string t = trim(tagToken);
            if (!t.empty()) tags.push_back(t);
            tagToken.clear();
        } else {
            tagToken += ch;
        }
    }
    std::string lastTag = trim(tagToken);
    if (!lastTag.empty()) tags.push_back(lastTag);

    int64_t now = currentTimestamp();
    int64_t nextRevision = now + (3 * 86400);

    models::Question question(
        id, title, description, topic, diff, company, platform,
        sourceUrl, status, isFav, notes,
        now, now, 0, nextRevision, 2, tags, "local_user"
    );

    auto result = questionService_->createQuestion(question);
    if (result.isValid) {
        std::cout << "\nQuestion created successfully.\n";
        std::cout << "Question ID: " << id << "\n";
    } else {
        std::cout << "\n[!] Error creating question: " << result.errorMessage << "\n";
    }
}

void CliShell::handleViewAllQuestions() {
    std::cout << "\n--- Tracked Questions ---\n";
    if (!questionService_) {
        std::cout << "[!] Question service is not initialized.\n";
        return;
    }

    const auto questions = questionService_->getAllQuestions();
    printQuestionsTable(questions);
}

void CliShell::handleViewQuestionById() {
    std::string id = readLine("\nEnter Question ID: ");
    if (id.empty()) {
        return;
    }

    auto qOpt = questionService_->getQuestionById(id);
    if (!qOpt.has_value()) {
        std::cout << "[!] Question not found with ID: " << id << "\n";
        return;
    }

    const auto& q = qOpt.value();
    std::cout << "\n========================================\n";
    std::cout << " [" << q.getId() << "] " << q.getTitle() << "\n";
    std::cout << "========================================\n";
    std::cout << "Topic        : " << models::topicToString(q.getTopic()) << "\n";
    std::cout << "Difficulty   : " << models::difficultyToString(q.getDifficulty()) << "\n";
    std::cout << "Status       : " << models::statusToString(q.getStatus()) << "\n";
    std::cout << "Company      : " << (q.getCompany().empty() ? "None specified" : q.getCompany()) << "\n";
    std::cout << "Platform     : " << models::platformToString(q.getPlatform()) << "\n";
    std::cout << "Source URL   : " << (q.getSourceUrl().empty() ? "None" : q.getSourceUrl()) << "\n";
    std::cout << "Favorite     : " << (q.isFavorite() ? "Yes [*]" : "No") << "\n";
    std::cout << "Description  : " << (q.getDescription().empty() ? "No description provided." : q.getDescription()) << "\n";

    if (!q.getTags().empty()) {
        std::cout << "Tags         : ";
        for (size_t i = 0; i < q.getTags().size(); ++i) {
            std::cout << q.getTags()[i] << (i + 1 < q.getTags().size() ? ", " : "");
        }
        std::cout << "\n";
    }
    if (!q.getNotes().empty()) {
        std::cout << "Notes        : " << q.getNotes() << "\n";
    }

    std::cout << "Last Practiced: " << utils::formatTimestamp(q.getLastPracticedAt()) << "\n";
    std::cout << "Next Revision : " << utils::formatTimestamp(q.getNextRevisionAt());
    if (q.getNextRevisionAt() > 0) {
        int64_t cur = revisionService_ ? revisionService_->now() : currentTimestamp();
        if (q.getNextRevisionAt() <= cur) {
            std::cout << " [DUE NOW]";
        } else {
            int64_t daysLeft = (q.getNextRevisionAt() - cur + 86399) / 86400;
            std::cout << " (in " << daysLeft << " day" << (daysLeft == 1 ? "" : "s") << ")";
        }
    }
    std::cout << "\n";
    std::cout << "Revision Prio : " << q.getRevisionPriority() << " (1=High, 5=Low)\n";

    if (recentHistoryService_) {
        recentHistoryService_->recordView(q.getId());
    }
}

void CliShell::handleEditQuestion() {
    std::cout << "\n--- Edit Question ---\n";
    std::string id = readLine("Enter Question ID to edit: ");
    if (id.empty()) return;

    auto qOpt = questionService_->getQuestionById(id);
    if (!qOpt.has_value()) {
        std::cout << "[!] Question not found with ID: " << id << "\n";
        return;
    }

    auto q = qOpt.value();
    std::cout << "Leave blank to keep existing values in brackets.\n";

    std::string title = readLineWithDefault("Title", q.getTitle());
    std::string desc = readLineWithDefault("Description", q.getDescription());
    std::string topicStr = readLineWithDefault("Topic", models::topicToString(q.getTopic()));
    std::string diffStr = readLineWithDefault("Difficulty", models::difficultyToString(q.getDifficulty()));
    std::string company = readLineWithDefault("Company", q.getCompany());
    std::string statusStr = readLineWithDefault("Status", models::statusToString(q.getStatus()));
    bool isFav = readConfirmation("Favorite", q.isFavorite());
    std::string notes = readLineWithDefault("Notes", q.getNotes());

    q.setTitle(title);
    q.setDescription(desc);
    q.setTopic(models::stringToTopic(topicStr));
    q.setDifficulty(models::stringToDifficulty(diffStr));
    q.setCompany(company);
    q.setStatus(models::stringToStatus(statusStr));
    q.setFavorite(isFav);
    q.setNotes(notes);
    q.setUpdatedAt(currentTimestamp());

    auto result = questionService_->updateQuestion(q);
    if (result.isValid) {
        std::cout << "\nQuestion updated successfully.\n";
    } else {
        std::cout << "\n[!] Error updating question: " << result.errorMessage << "\n";
    }
}

void CliShell::handleDeleteQuestion() {
    std::cout << "\n--- Delete Question ---\n";
    std::string id = readLine("Enter Question ID to delete: ");
    if (id.empty()) return;

    auto qOpt = questionService_->getQuestionById(id);
    if (!qOpt.has_value()) {
        std::cout << "[!] Question not found with ID: " << id << "\n";
        return;
    }

    std::cout << "Question: [" << id << "] " << qOpt.value().getTitle() << "\n";
    if (readConfirmation("Are you sure you want to permanently delete this question?", false)) {
        if (questionService_->deleteQuestion(id)) {
            std::cout << "\nQuestion deleted successfully.\n";
        } else {
            std::cout << "\n[!] Failed to delete question.\n";
        }
    } else {
        std::cout << "Deletion cancelled.\n";
    }
}

void CliShell::handleSearchByTitlePrefix() {
    std::cout << "\n--- Title Prefix Search (Custom PrefixTrie) ---\n";
    std::string prefix = readLine("Enter title prefix: ");
    if (prefix.empty()) {
        std::cout << "No prefix entered.\n";
        return;
    }

    if (!questionService_ || !searchService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    const auto allQuestions = questionService_->getAllQuestions();
    auto matches = searchService_->searchByTitlePrefix(allQuestions, prefix);

    auto suggestions = searchService_->searchTitlesByPrefix(prefix);
    if (!suggestions.empty()) {
        std::cout << "Trie Autocomplete Suggestions: ";
        for (size_t i = 0; i < suggestions.size(); ++i) {
            std::cout << "\"" << suggestions[i] << "\"" << (i + 1 < suggestions.size() ? ", " : "\n");
        }
    }

    printQuestionsTable(matches);
}

void CliShell::handleSearchByKeyword() {
    std::cout << "\n--- Keyword Search (All Fields) ---\n";
    std::string keyword = readLine("Enter keyword (matches title, description, tags, company): ");
    if (keyword.empty()) {
        std::cout << "No keyword entered.\n";
        return;
    }

    if (!questionService_ || !searchService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    const auto allQuestions = questionService_->getAllQuestions();
    auto matches = searchService_->searchByKeyword(allQuestions, keyword);
    printQuestionsTable(matches);
}

void CliShell::handleFilterQuestions() {
    std::cout << "\n--- Filter Questions (Press Enter to skip any criteria) ---\n";
    std::string topicStr = readLine("Topic [e.g. Arrays, Trees, DynamicProgramming]: ");
    std::string diffStr = readLine("Difficulty [Easy, Medium, Hard]: ");
    std::string statusStr = readLine("Status [Unsolved, InProgress, Solved, Mastered]: ");
    std::string company = readLine("Company: ");
    std::string favStr = readLine("Favorite [1=Favorites only, 2=Non-favorites, Enter=All]: ");

    models::QuestionFilter filter;
    if (!topicStr.empty()) {
        filter.topic = models::stringToTopic(topicStr);
    }
    if (!diffStr.empty() && models::isValidDifficulty(diffStr)) {
        filter.difficulty = models::stringToDifficulty(diffStr);
    }
    if (!statusStr.empty() && models::isValidStatus(statusStr)) {
        filter.status = models::stringToStatus(statusStr);
    }
    if (!company.empty()) {
        filter.company = company;
    }
    if (favStr == "1") {
        filter.isFavorite = true;
    } else if (favStr == "2") {
        filter.isFavorite = false;
    }

    if (!questionService_ || !searchService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    const auto allQuestions = questionService_->getAllQuestions();
    auto results = searchService_->filter(allQuestions, filter);
    printQuestionsTable(results);
}

void CliShell::handleSortQuestions() {
    std::cout << "\n--- Sort Questions ---\n";
    std::cout << "Available Sort Fields:\n";
    std::cout << "  1. Title\n";
    std::cout << "  2. Difficulty\n";
    std::cout << "  3. Topic\n";
    std::cout << "  4. Status\n";
    std::cout << "  5. Company\n";
    std::cout << "  6. Created Date\n";
    std::cout << "  7. Updated Date\n";
    std::cout << "  8. Revision Priority\n";
    std::string fieldChoice = readLineWithDefault("Select Field [1-8]", "1");

    models::SortField field = models::SortField::Title;
    switch (fieldChoice[0]) {
        case '1': field = models::SortField::Title; break;
        case '2': field = models::SortField::Difficulty; break;
        case '3': field = models::SortField::Topic; break;
        case '4': field = models::SortField::Status; break;
        case '5': field = models::SortField::Company; break;
        case '6': field = models::SortField::CreatedAt; break;
        case '7': field = models::SortField::UpdatedAt; break;
        case '8': field = models::SortField::RevisionPriority; break;
        default:  field = models::SortField::Title; break;
    }

    std::string dirChoice = readLineWithDefault("Direction [1=Ascending, 2=Descending]", "1");
    models::SortDirection dir = (dirChoice == "2") ? models::SortDirection::Descending : models::SortDirection::Ascending;

    std::string algoChoice = readLineWithDefault("Algorithm [1=Merge Sort (Stable), 2=Quick Sort (In-Place)]", "1");
    models::SortAlgorithm algo = (algoChoice == "2") ? models::SortAlgorithm::QuickSort : models::SortAlgorithm::MergeSort;

    models::SortOptions options{field, dir, algo};

    if (!questionService_ || !searchService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    auto questions = questionService_->getAllQuestions();
    searchService_->sort(questions, options);

    std::cout << "\n[Sorted by " << models::sortFieldToString(options.field)
              << " (" << models::sortDirectionToString(options.direction) << ")"
              << " using " << models::sortAlgorithmToString(options.algorithm) << "]\n";
    printQuestionsTable(questions);
}

void CliShell::handleCombinedSearchFilterSort() {
    std::cout << "\n--- Combined Search + Filter + Sort ---\n";
    std::string keyword = readLine("Search Term / Keyword (Enter to skip): ");
    std::string topicStr = readLine("Topic [e.g. Arrays, Trees] (Enter to skip): ");
    std::string diffStr = readLine("Difficulty [Easy, Medium, Hard] (Enter to skip): ");
    std::string statusStr = readLine("Status [Unsolved, Solved] (Enter to skip): ");
    std::string company = readLine("Company (Enter to skip): ");
    std::string favStr = readLine("Favorite [1=Favs, 2=Non-favs, Enter=All]: ");

    models::QuestionFilter filter;
    if (!keyword.empty()) filter.keyword = keyword;
    if (!topicStr.empty()) filter.topic = models::stringToTopic(topicStr);
    if (!diffStr.empty() && models::isValidDifficulty(diffStr)) filter.difficulty = models::stringToDifficulty(diffStr);
    if (!statusStr.empty() && models::isValidStatus(statusStr)) filter.status = models::stringToStatus(statusStr);
    if (!company.empty()) filter.company = company;
    if (favStr == "1") filter.isFavorite = true;
    else if (favStr == "2") filter.isFavorite = false;

    std::cout << "\nSorting Setup:\n";
    std::cout << "1. Title  2. Difficulty  3. Topic  4. Status  5. Company  6. Created Date\n";
    std::string fieldChoice = readLineWithDefault("Sort Field [1-6]", "1");
    models::SortField field = models::SortField::Title;
    switch (fieldChoice[0]) {
        case '1': field = models::SortField::Title; break;
        case '2': field = models::SortField::Difficulty; break;
        case '3': field = models::SortField::Topic; break;
        case '4': field = models::SortField::Status; break;
        case '5': field = models::SortField::Company; break;
        case '6': field = models::SortField::CreatedAt; break;
        default:  field = models::SortField::Title; break;
    }

    std::string dirChoice = readLineWithDefault("Direction [1=Ascending, 2=Descending]", "1");
    models::SortDirection dir = (dirChoice == "2") ? models::SortDirection::Descending : models::SortDirection::Ascending;

    std::string algoChoice = readLineWithDefault("Algorithm [1=Merge Sort, 2=Quick Sort]", "1");
    models::SortAlgorithm algo = (algoChoice == "2") ? models::SortAlgorithm::QuickSort : models::SortAlgorithm::MergeSort;

    models::SortOptions sortOptions{field, dir, algo};

    if (!questionService_ || !searchService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    const auto allQuestions = questionService_->getAllQuestions();
    auto results = searchService_->searchAndFilter(allQuestions, filter, sortOptions);

    std::cout << "\n[Results sorted by " << models::sortFieldToString(sortOptions.field)
              << " (" << models::sortDirectionToString(sortOptions.direction) << ")"
              << " using " << models::sortAlgorithmToString(sortOptions.algorithm) << "]\n";
    printQuestionsTable(results);
}

void CliShell::handlePracticeSubmenu() {
    bool inSubmenu = true;
    std::string input;

    while (inSubmenu && isRunning_) {
        printPracticeMenu();

        if (!std::getline(std::cin, input)) {
            break;
        }

        std::string choice = trim(input);
        if (choice.empty()) {
            continue;
        }

        switch (choice[0]) {
            case '1':
                handlePracticeAllUnsolved();
                break;
            case '2':
                handlePracticeByTopic();
                break;
            case '3':
                handlePracticeByDifficulty();
                break;
            case '4':
                handlePracticeFavorites();
                break;
            case '5':
                handlePracticeDueRevisions();
                break;
            case '0':
                inSubmenu = false;
                break;
            default:
                std::cout << "\n[!] Invalid option. Please select 0-5.\n";
                break;
        }
    }
}

void CliShell::handlePracticeAllUnsolved() {
    if (!questionService_ || !practiceService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    std::vector<std::string> ids;
    for (const auto& q : questionService_->getAllQuestions()) {
        if (q.getStatus() == models::Status::Unsolved || q.getStatus() == models::Status::InProgress) {
            ids.push_back(q.getId());
        }
    }

    if (ids.empty()) {
        std::cout << "\n[!] No unsolved questions found to practice!\n";
        return;
    }

    practiceService_->startSession(ids);
    runPracticeSession();
}

void CliShell::handlePracticeByTopic() {
    if (!questionService_ || !practiceService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    std::cout << "\nSelect Topic for Practice:\n";
    std::cout << "1. Arrays               2. Strings\n";
    std::cout << "3. LinkedLists          4. StacksQueues\n";
    std::cout << "5. Trees                6. Graphs\n";
    std::cout << "7. DynamicProgramming   8. BinarySearch\n";
    std::cout << "9. RecursionBacktrack   10. Heaps\n";
    std::cout << "Enter Topic name or number: ";

    std::string topicStr = readLine("");
    if (topicStr.empty()) return;

    auto topic = models::stringToTopic(topicStr);
    std::vector<std::string> ids;
    for (const auto& q : questionService_->getAllQuestions()) {
        if (q.getTopic() == topic) {
            ids.push_back(q.getId());
        }
    }

    if (ids.empty()) {
        std::cout << "\n[!] No questions found for topic: " << models::topicToString(topic) << "\n";
        return;
    }

    practiceService_->startSession(ids);
    runPracticeSession();
}

void CliShell::handlePracticeByDifficulty() {
    if (!questionService_ || !practiceService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    std::cout << "\nSelect Difficulty:\n";
    std::cout << "1. Easy\n2. Medium\n3. Hard\n";
    std::string diffStr = readLine("Enter Difficulty: ");
    if (diffStr.empty()) return;

    auto diff = models::stringToDifficulty(diffStr);
    if (diff == models::Difficulty::Unknown) {
        std::cout << "[!] Invalid difficulty selection.\n";
        return;
    }

    std::vector<std::string> ids;
    for (const auto& q : questionService_->getAllQuestions()) {
        if (q.getDifficulty() == diff) {
            ids.push_back(q.getId());
        }
    }

    if (ids.empty()) {
        std::cout << "\n[!] No questions found for difficulty: " << models::difficultyToString(diff) << "\n";
        return;
    }

    practiceService_->startSession(ids);
    runPracticeSession();
}

void CliShell::handlePracticeFavorites() {
    if (!questionService_ || !practiceService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    std::vector<std::string> ids;
    for (const auto& q : questionService_->getAllQuestions()) {
        if (q.isFavorite()) {
            ids.push_back(q.getId());
        }
    }

    if (ids.empty()) {
        std::cout << "\n[!] No favorite questions marked yet.\n";
        return;
    }

    practiceService_->startSession(ids);
    runPracticeSession();
}

void CliShell::handlePracticeDueRevisions() {
    if (!practiceService_ || !revisionService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    int64_t curTime = revisionService_->now();
    auto dueItems = revisionService_->getDueQuestions(curTime);
    if (dueItems.empty()) {
        std::cout << "\n[+] No questions currently due for revision! Excellent work.\n";
        return;
    }

    std::vector<std::string> ids;
    ids.reserve(dueItems.size());
    for (const auto& item : dueItems) {
        ids.push_back(item.questionId);
    }

    practiceService_->startSession(ids);
    runPracticeSession();
}

void CliShell::runPracticeSession() {
    if (!practiceService_ || !practiceService_->hasQuestions()) {
        std::cout << "\n[!] No active practice session.\n";
        return;
    }

    std::cout << "\n[Practice session started with " << practiceService_->getRemainingCount() << " question(s)]\n";

    while (practiceService_->hasQuestions() && isRunning_) {
        auto currentIdOpt = practiceService_->getCurrentQuestion();
        if (!currentIdOpt.has_value()) {
            break;
        }

        std::string id = currentIdOpt.value();
        auto qOpt = questionService_ ? questionService_->getQuestionById(id) : std::nullopt;
        if (!qOpt.has_value()) {
            practiceService_->completeCurrentQuestion();
            continue;
        }

        const auto& q = qOpt.value();
        auto progress = practiceService_->getProgress();

        std::cout << "\n--------------------------------\n";
        std::cout << "Practice Question (" << (progress.completed + 1) << " of " << progress.total << ", " << progress.remaining << " remaining)\n";
        std::cout << "--------------------------------\n";
        std::cout << "ID: " << q.getId() << "\n";
        std::cout << "Title: " << q.getTitle() << "\n";
        std::cout << "Topic: " << models::topicToString(q.getTopic()) << "\n";
        std::cout << "Difficulty: " << models::difficultyToString(q.getDifficulty()) << "\n";
        std::cout << "Company: " << (q.getCompany().empty() ? "-" : q.getCompany()) << "\n";
        std::cout << "Platform: " << models::platformToString(q.getPlatform()) << "\n";
        std::cout << "Status: " << models::statusToString(q.getStatus()) << "\n";
        if (!q.getNotes().empty()) {
            std::cout << "Notes: " << q.getNotes() << "\n";
        }
        std::cout << "\nDescription:\n" << (q.getDescription().empty() ? "(No description provided)" : q.getDescription()) << "\n";

        std::cout << "\n--------------------------------\n";
        std::cout << "1. Mark Solved\n";
        std::cout << "2. Needs Review\n";
        std::cout << "3. Skip\n";
        std::cout << "4. Exit Session\n";
        std::cout << "--------------------------------\n";
        std::cout << "Enter choice: ";

        std::string choiceLine = readLine("");
        if (!std::cin.good()) {
            practiceService_->clearSession();
            isRunning_ = false;
            break;
        }
        if (choiceLine.empty()) continue;

        char c = choiceLine[0];
        if (c == '1') {
            auto schedOpt = practiceService_->recordPracticeAttempt(models::PracticeVerdict::Solved);
            if (schedOpt.has_value()) {
                std::cout << "\n[+] Marked Solved! Next revision scheduled in "
                          << (schedOpt->intervalSeconds / 86400) << " day(s) (Priority "
                          << schedOpt->revisionPriority << ", Status: "
                          << models::statusToString(schedOpt->newStatus) << ").\n";
            } else {
                std::cout << "\n[+] Marked Solved!\n";
            }
        } else if (c == '2') {
            auto schedOpt = practiceService_->recordPracticeAttempt(models::PracticeVerdict::NeedsReview);
            if (schedOpt.has_value()) {
                std::cout << "\n[*] Marked Needs Review. Rescheduled for review in "
                          << (schedOpt->intervalSeconds / 86400) << " day with High Urgency (Priority "
                          << schedOpt->revisionPriority << ").\n";
            } else {
                std::cout << "\n[*] Marked Needs Review.\n";
            }
        } else if (c == '3') {
            practiceService_->recordPracticeAttempt(models::PracticeVerdict::Skipped);
            std::cout << "\n[-] Question skipped. Cycled to back of FIFO session queue.\n";
        } else if (c == '4') {
            std::cout << "\n[!] Practice session ended early.\n";
            break;
        } else {
            std::cout << "\n[!] Invalid selection. Please choose 1-4.\n";
        }
    }

    auto progress = practiceService_->getProgress();
    std::cout << "\n================================\n";
    std::cout << "   Practice Session Summary     \n";
    std::cout << "================================\n";
    std::cout << "Total Reviewed : " << progress.completed << "\n";
    std::cout << "Total Skipped  : " << progress.skipped << "\n";
    std::cout << "================================\n";

    practiceService_->clearSession();
}

void CliShell::handleRevisionSubmenu() {
    bool inSubmenu = true;
    std::string input;

    while (inSubmenu && isRunning_) {
        printRevisionMenu();

        if (!std::getline(std::cin, input)) {
            break;
        }

        std::string choice = trim(input);
        if (choice.empty()) {
            continue;
        }

        switch (choice[0]) {
            case '1':
                handleShowDueQuestions();
                break;
            case '2':
                handleShowUpcomingRevisions();
                break;
            case '3':
                handleShowNextRevision();
                break;
            case '4':
                handleStartDueRevisionSession();
                break;
            case '5':
                handleScheduleQuestion();
                break;
            case '0':
                inSubmenu = false;
                break;
            default:
                std::cout << "\n[!] Invalid option. Please select 0-5.\n";
                break;
        }
    }
}

void CliShell::handleShowDueQuestions() {
    if (!revisionService_) {
        std::cout << "[!] Revision service is not available.\n";
        return;
    }

    int64_t curTime = revisionService_->now();
    auto dueItems = revisionService_->getDueQuestions(curTime);

    if (dueItems.empty()) {
        std::cout << "\n[+] No questions currently due for revision.\n";
        return;
    }

    std::cout << "\n--- Questions Due for Revision ---\n";
    printRevisionTable(dueItems, true);
    std::cout << "Total Due: " << dueItems.size() << "\n";
}

void CliShell::handleShowUpcomingRevisions() {
    if (!revisionService_) {
        std::cout << "[!] Revision service is not available.\n";
        return;
    }

    int64_t curTime = revisionService_->now();
    auto upcoming = revisionService_->getUpcomingRevisions(20, curTime);

    if (upcoming.empty()) {
        std::cout << "\nNo upcoming revisions scheduled in the future.\n";
        return;
    }

    std::cout << "\n--- Upcoming Scheduled Revisions ---\n";
    printRevisionTable(upcoming, false);
}

void CliShell::handleShowNextRevision() {
    if (!revisionService_) {
        std::cout << "[!] Revision service is not available.\n";
        return;
    }

    auto nextOpt = revisionService_->getNextDueRevision();
    if (!nextOpt.has_value()) {
        std::cout << "\nNo scheduled revisions found.\n";
        return;
    }

    const auto& item = nextOpt.value();
    std::cout << "\n========================================\n";
    std::cout << "         Next Scheduled Revision        \n";
    std::cout << "========================================\n";
    std::cout << "Question ID   : " << item.questionId << "\n";

    if (questionService_) {
        auto qOpt = questionService_->getQuestionById(item.questionId);
        if (qOpt.has_value()) {
            std::cout << "Title         : " << qOpt->getTitle() << "\n";
            std::cout << "Topic         : " << models::topicToString(qOpt->getTopic()) << "\n";
            std::cout << "Difficulty    : " << models::difficultyToString(qOpt->getDifficulty()) << "\n";
            std::cout << "Status        : " << models::statusToString(qOpt->getStatus()) << "\n";
        }
    }

    int64_t curTime = revisionService_->now();
    std::cout << "Priority      : " << item.priority << (item.priority == 1 ? " (Urgent)" : "") << "\n";
    std::cout << "Scheduled For : " << utils::formatTimestamp(item.nextRevisionAt) << "\n";

    if (item.nextRevisionAt <= curTime) {
        std::cout << "Status        : [DUE NOW]\n";
    } else {
        int64_t daysLeft = (item.nextRevisionAt - curTime + 86399) / 86400;
        std::cout << "Status        : Upcoming in " << daysLeft << " day" << (daysLeft == 1 ? "" : "s") << "\n";
    }
    std::cout << "========================================\n";
}

void CliShell::handleStartDueRevisionSession() {
    handlePracticeDueRevisions();
}

void CliShell::handleScheduleQuestion() {
    if (!questionService_ || !revisionService_) {
        std::cout << "[!] Services unavailable.\n";
        return;
    }

    std::string id = readLine("\nEnter Question ID to schedule: ");
    if (id.empty()) return;

    auto qOpt = questionService_->getQuestionById(id);
    if (!qOpt.has_value()) {
        std::cout << "[!] Question not found with ID: " << id << "\n";
        return;
    }

    auto q = qOpt.value();
    std::cout << "Scheduling revision for: [" << q.getId() << "] " << q.getTitle() << "\n";

    std::string daysStr = readLineWithDefault("Revision interval in days (e.g. 1, 3, 7, 14, 30)", "1");
    int days = 1;
    try {
        days = std::max(1, std::stoi(daysStr));
    } catch (...) {
        days = 1;
    }

    std::string prioStr = readLineWithDefault("Priority (1=High, 5=Low)", "2");
    int prio = 2;
    try {
        prio = std::stoi(prioStr);
        if (prio < 1) prio = 1;
        if (prio > 5) prio = 5;
    } catch (...) {
        prio = 2;
    }

    int64_t curTime = revisionService_->now();
    int64_t nextRevisionAt = curTime + static_cast<int64_t>(days) * 86400;

    q.setNextRevisionAt(nextRevisionAt);
    q.setRevisionPriority(prio);
    q.setUpdatedAt(curTime);

    auto val = questionService_->updateQuestion(q);
    if (val.isValid) {
        std::cout << "\n[+] Successfully scheduled " << q.getId()
                  << " for revision on " << utils::formatTimestamp(nextRevisionAt)
                  << " with Priority " << prio << ".\n";
    } else {
        std::cout << "\n[!] Scheduling failed: " << val.errorMessage << "\n";
    }
}

std::string CliShell::renderProgressBar(double percentage, int width) {
    if (width <= 0) width = 20;
    double clamped = std::max(0.0, std::min(100.0, percentage));
    int filled = static_cast<int>(std::round((clamped / 100.0) * width));
    filled = std::max(0, std::min(width, filled));
    int unfilled = width - filled;

    return "[" + std::string(filled, '#') + std::string(unfilled, '-') + "]";
}

void CliShell::printDashboard(const models::DashboardSnapshot& snapshot) const {
    std::cout << "\n========================================\n";
    std::cout << "          CODEVAULT DASHBOARD\n";
    std::cout << "========================================\n";
    std::cout << "Generated: " << utils::formatTimestamp(snapshot.generatedAt) << "\n\n";

    if (snapshot.overall.totalQuestions == 0) {
        std::cout << "Total Questions Tracked : 0\n";
        std::cout << "\n[!] No questions found in catalog.\n";
        std::cout << "    Add questions to populate curriculum progress & statistics.\n";
        std::cout << "========================================\n";
        return;
    }

    std::cout << "----------------------------------------\n";
    std::cout << "OVERALL CURRICULUM PROGRESS\n";
    std::cout << "----------------------------------------\n";
    std::cout << std::left
              << std::setw(24) << "Total Questions" << ": " << snapshot.overall.totalQuestions << "\n"
              << std::setw(24) << "Solved" << ": " << snapshot.overall.solvedCount << "\n"
              << std::setw(24) << "Mastered" << ": " << snapshot.overall.masteredCount << "\n"
              << std::setw(24) << "In Progress" << ": " << snapshot.overall.inProgressCount << "\n"
              << std::setw(24) << "Unsolved" << ": " << snapshot.overall.unsolvedCount << "\n"
              << std::setw(24) << "Favorites" << ": " << snapshot.overall.favoriteCount << "\n\n";

    std::cout << "Completion Rate (Solved + Mastered):\n";
    std::cout << renderProgressBar(snapshot.overall.completionPercentage, 24) << " "
              << std::fixed << std::setprecision(1) << snapshot.overall.completionPercentage << "% ("
              << (snapshot.overall.solvedCount + snapshot.overall.masteredCount) << " of "
              << snapshot.overall.totalQuestions << " questions)\n\n";

    std::cout << "----------------------------------------\n";
    std::cout << "DIFFICULTY DISTRIBUTION\n";
    std::cout << "----------------------------------------\n";
    std::cout << std::left
              << std::setw(10) << "Easy" << ": " << std::setw(4) << snapshot.difficulty.easyCount
              << " (" << std::fixed << std::setprecision(1) << std::setw(5) << snapshot.difficulty.easyPercentage << "%) "
              << renderProgressBar(snapshot.difficulty.easyPercentage, 16) << "\n"
              << std::setw(10) << "Medium" << ": " << std::setw(4) << snapshot.difficulty.mediumCount
              << " (" << std::fixed << std::setprecision(1) << std::setw(5) << snapshot.difficulty.mediumPercentage << "%) "
              << renderProgressBar(snapshot.difficulty.mediumPercentage, 16) << "\n"
              << std::setw(10) << "Hard" << ": " << std::setw(4) << snapshot.difficulty.hardCount
              << " (" << std::fixed << std::setprecision(1) << std::setw(5) << snapshot.difficulty.hardPercentage << "%) "
              << renderProgressBar(snapshot.difficulty.hardPercentage, 16) << "\n\n";

    std::cout << "----------------------------------------\n";
    std::cout << "STATUS BREAKDOWN\n";
    std::cout << "----------------------------------------\n";
    std::cout << std::left
              << std::setw(14) << "Unsolved" << ": " << std::setw(4) << snapshot.status.unsolvedCount
              << " (" << std::fixed << std::setprecision(1) << std::setw(5) << snapshot.status.unsolvedPercentage << "%) "
              << renderProgressBar(snapshot.status.unsolvedPercentage, 16) << "\n"
              << std::setw(14) << "In Progress" << ": " << std::setw(4) << snapshot.status.inProgressCount
              << " (" << std::fixed << std::setprecision(1) << std::setw(5) << snapshot.status.inProgressPercentage << "%) "
              << renderProgressBar(snapshot.status.inProgressPercentage, 16) << "\n"
              << std::setw(14) << "Solved" << ": " << std::setw(4) << snapshot.status.solvedCount
              << " (" << std::fixed << std::setprecision(1) << std::setw(5) << snapshot.status.solvedPercentage << "%) "
              << renderProgressBar(snapshot.status.solvedPercentage, 16) << "\n"
              << std::setw(14) << "Mastered" << ": " << std::setw(4) << snapshot.status.masteredCount
              << " (" << std::fixed << std::setprecision(1) << std::setw(5) << snapshot.status.masteredPercentage << "%) "
              << renderProgressBar(snapshot.status.masteredPercentage, 16) << "\n\n";

    std::cout << "----------------------------------------\n";
    std::cout << "TOPIC COVERAGE (" << snapshot.topic.distinctTopicsCount << " of "
              << snapshot.topic.topicCounts.size() << " Topics Tracked)\n";
    std::cout << "----------------------------------------\n";
    size_t zeroTopicCount = 0;
    for (const auto& tc : snapshot.topic.topicCounts) {
        if (tc.count > 0) {
            std::cout << std::left << std::setw(22) << tc.topicName << ": "
                      << std::setw(4) << tc.count << " ("
                      << std::fixed << std::setprecision(1) << std::setw(5) << tc.percentage << "%) "
                      << renderProgressBar(tc.percentage, 14) << "\n";
        } else {
            ++zeroTopicCount;
        }
    }
    if (zeroTopicCount > 0) {
        std::cout << "  (" << zeroTopicCount << " other defined topic"
                  << (zeroTopicCount == 1 ? "" : "s") << " currently have 0 questions)\n";
    }

    std::cout << "\n----------------------------------------\n";
    std::cout << "SPACED REVISION & RETENTION\n";
    std::cout << "----------------------------------------\n";
    std::cout << std::left
              << std::setw(24) << "Due for Revision" << ": " << snapshot.revision.dueCount;
    if (snapshot.revision.dueCount > 0) {
        std::cout << "  [DUE NOW]";
    }
    std::cout << "\n"
              << std::setw(24) << "Upcoming Scheduled" << ": " << snapshot.revision.upcomingCount << "\n"
              << std::setw(24) << "Unscheduled" << ": " << snapshot.revision.unscheduledCount << "\n"
              << std::setw(24) << "Total Scheduled" << ": " << snapshot.revision.scheduledCount
              << " (" << std::fixed << std::setprecision(1) << snapshot.revision.scheduledPercentageOfTotal << "% of catalog)\n\n";

    std::cout << "Urgency Priority Breakdown (Scheduled):\n";
    std::cout << "  Priority 1 (Urgent) : " << snapshot.revision.priorityCounts[1] << "\n"
              << "  Priority 2          : " << snapshot.revision.priorityCounts[2] << "\n"
              << "  Priority 3          : " << snapshot.revision.priorityCounts[3] << "\n"
              << "  Priority 4          : " << snapshot.revision.priorityCounts[4] << "\n"
              << "  Priority 5 (Lowest) : " << snapshot.revision.priorityCounts[5] << "\n\n";

    std::cout << "Revision Level Progression (Scheduled):\n";
    std::cout << "  Level 1 (1 Day)     : " << snapshot.revision.levelCounts[1] << "\n"
              << "  Level 2 (3 Days)    : " << snapshot.revision.levelCounts[2] << "\n"
              << "  Level 3 (7 Days)    : " << snapshot.revision.levelCounts[3] << "\n"
              << "  Level 4 (14 Days)   : " << snapshot.revision.levelCounts[4] << "\n"
              << "  Level 5 (30 Days+)  : " << snapshot.revision.levelCounts[5] << "\n\n";

    std::cout << "----------------------------------------\n";
    std::cout << "PRACTICE ACTIVITY\n";
    std::cout << "----------------------------------------\n";
    std::cout << std::left
              << std::setw(24) << "Questions Practiced" << ": " << snapshot.practice.practicedCount
              << " (" << std::fixed << std::setprecision(1) << snapshot.practice.practicedPercentage << "%)\n"
              << std::setw(24) << "Unpracticed" << ": " << snapshot.practice.unpracticedCount << "\n";
    if (snapshot.practice.lastPracticedTimestamp > 0) {
        std::cout << std::setw(24) << "Last Practice Attempt" << ": "
                  << utils::formatTimestamp(snapshot.practice.lastPracticedTimestamp) << "\n";
    } else {
        std::cout << std::setw(24) << "Last Practice Attempt" << ": Never\n";
    }
    std::cout << "========================================\n";
}

void CliShell::handleStatistics() {
    if (!statisticsService_) {
        statisticsService_ = std::make_shared<services::StatisticsService>(questionService_, revisionService_);
    }
    auto snapshot = statisticsService_->getDashboardSnapshot();
    printDashboard(snapshot);
}

void CliShell::handleSettings() {
    std::cout << "\n--- Settings ---\n";
    std::cout << "Active User Profile : local_user\n";
    std::cout << "Storage Engine      : Local CSV Flat-File\n";
    std::cout << "Architecture Tier   : Stage 6 Dashboard & Progress Statistics\n";
}

} // namespace codevault::app
