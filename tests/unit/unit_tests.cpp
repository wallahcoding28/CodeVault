#include "models/enums.hpp"
#include "models/practice_result.hpp"
#include "models/question.hpp"
#include "models/revision_item.hpp"
#include "utils/clock.hpp"
#include "utils/datetime.hpp"
#include "models/user.hpp"
#include "persistence/file_question_repository.hpp"
#include "persistence/migration_service.hpp"
#include "persistence/sqlite_question_repository.hpp"
#include "persistence/sqlite_user_repository.hpp"
#include "persistence/sqlite_session_repository.hpp"
#include "services/auth_service.hpp"
#include "utils/crypto.hpp"
#include "services/current_user_provider.hpp"
#include "services/question_service.hpp"
#include <sqlite3.h>
#include "services/revision_service.hpp"
#include "models/practice_next.hpp"
#include "services/practice_service.hpp"
#include "services/recent_history_service.hpp"
#include "services/problem_playlist_service.hpp"
#include "dsa/trie.hpp"
#include "dsa/min_heap.hpp"
#include "dsa/queue.hpp"
#include "dsa/stack.hpp"
#include "dsa/doubly_linked_list.hpp"
#include "dsa/sorting.hpp"
#include "models/question_filter.hpp"
#include "models/sort_options.hpp"
#include "models/statistics_models.hpp"
#include "services/search_service.hpp"
#include "services/statistics_service.hpp"
#include "app/cli_shell.hpp"
#include "app/http_server.hpp"
#include "utils/json.hpp"
#include "models/import_result.hpp"
#include "services/import_service.hpp"
#include "services/export_service.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_passedTests = 0;
int g_totalTests = 0;

template <typename F>
void runTest(const std::string& name, F&& testFunc) {
    ++g_totalTests;
    std::cout << "  [TEST " << g_totalTests << "] " << name << " ... " << std::flush;
    try {
        if (testFunc()) {
            std::cout << "PASSED\n" << std::flush;
            ++g_passedTests;
        } else {
            std::cout << "FAILED\n" << std::flush;
        }
    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")\n" << std::flush;
    } catch (...) {
        std::cout << "FAILED (Unknown exception)\n" << std::flush;
    }
}

} // namespace

int main() {
    std::cout << "==================================================\n";
    std::cout << "        CodeVault Stage 2 Unit Test Suite         \n";
    std::cout << "==================================================\n\n";

    // =========================================================================
    // 1. Question Model & Enum Tests
    // =========================================================================
    std::cout << "--- 1. Domain Model & Enum Conversions ---\n";

    runTest("Question Model Constructor & Getters", []() {
        codevault::models::Question q(
            "Q-TEST1", "Two Sum Variant", "Find pair with target sum",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Amazon",
            codevault::models::Platform::LeetCode,
            "https://leetcode.com/problems/two-sum/",
            codevault::models::Status::Solved,
            true, "Hash map approach",
            1000, 2000, 1500, 3000, 1,
            {"array", "hash-map"},
            "test_user"
        );

        return q.getId() == "Q-TEST1" &&
               q.getTitle() == "Two Sum Variant" &&
               q.getDescription() == "Find pair with target sum" &&
               q.getTopic() == codevault::models::Topic::Arrays &&
               q.getDifficulty() == codevault::models::Difficulty::Easy &&
               q.getCompany() == "Amazon" &&
               q.getPlatform() == codevault::models::Platform::LeetCode &&
               q.getSourceUrl() == "https://leetcode.com/problems/two-sum/" &&
               q.getStatus() == codevault::models::Status::Solved &&
               q.isFavorite() == true &&
               q.getNotes() == "Hash map approach" &&
               q.getCreatedAt() == 1000 &&
               q.getUpdatedAt() == 2000 &&
               q.getLastPracticedAt() == 1500 &&
               q.getNextRevisionAt() == 3000 &&
               q.getRevisionPriority() == 1 &&
               q.getTags().size() == 2 &&
               q.getOwnerId() == "test_user";
    });

    runTest("Question Model Setters & Mutability", []() {
        codevault::models::Question q;
        q.setTitle("Initial Title");
        q.setDifficulty(codevault::models::Difficulty::Hard);
        q.setStatus(codevault::models::Status::InProgress);
        q.setFavorite(true);
        q.setRevisionPriority(3);
        q.setTags({"tagA", "tagB"});

        return q.getTitle() == "Initial Title" &&
               q.getDifficulty() == codevault::models::Difficulty::Hard &&
               q.getStatus() == codevault::models::Status::InProgress &&
               q.isFavorite() == true &&
               q.getRevisionPriority() == 3 &&
               q.getTags().size() == 2;
    });

    runTest("Difficulty Enum Conversions & Case-Insensitivity", []() {
        using namespace codevault::models;
        bool b1 = difficultyToString(Difficulty::Easy) == "Easy";
        bool b2 = difficultyToString(Difficulty::Medium) == "Medium";
        bool b3 = difficultyToString(Difficulty::Hard) == "Hard";
        bool b4 = difficultyToString(Difficulty::Unknown) == "Unknown";

        bool b5 = stringToDifficulty("Easy") == Difficulty::Easy;
        bool b6 = stringToDifficulty("easy") == Difficulty::Easy;
        bool b7 = stringToDifficulty("EASY") == Difficulty::Easy;
        bool b8 = stringToDifficulty("medium") == Difficulty::Medium;
        bool b9 = stringToDifficulty("HARD") == Difficulty::Hard;
        bool b10 = stringToDifficulty("invalid") == Difficulty::Unknown;

        bool b11 = isValidDifficulty("Easy") && isValidDifficulty("medium") && !isValidDifficulty("SuperHard");

        return b1 && b2 && b3 && b4 && b5 && b6 && b7 && b8 && b9 && b10 && b11;
    });

    runTest("Status Enum Conversions & Aliases", []() {
        using namespace codevault::models;
        bool b1 = statusToString(Status::Unsolved) == "Unsolved";
        bool b2 = statusToString(Status::InProgress) == "InProgress";
        bool b3 = statusToString(Status::Solved) == "Solved";

        bool b4 = stringToStatus("Unsolved") == Status::Unsolved;
        bool b5 = stringToStatus("unsolved") == Status::Unsolved;
        bool b6 = stringToStatus("Todo") == Status::Unsolved;
        bool b7 = stringToStatus("InProgress") == Status::InProgress;
        bool b8 = stringToStatus("In Progress") == Status::InProgress;
        bool b9 = stringToStatus("Attempted") == Status::InProgress;
        bool b10 = stringToStatus("solved") == Status::Solved;

        bool b11 = isValidStatus("Unsolved") && isValidStatus("InProgress") && isValidStatus("Solved");
        bool b12 = !isValidStatus("RandomStatus");

        return b1 && b2 && b3 && b4 && b5 && b6 && b7 && b8 && b9 && b10 && b11 && b12;
    });

    runTest("Topic & Platform Enum Conversions", []() {
        using namespace codevault::models;
        bool b1 = topicToString(Topic::DynamicProgramming) == "DynamicProgramming";
        bool b2 = stringToTopic("DP") == Topic::DynamicProgramming;
        bool b3 = stringToTopic("BinarySearch") == Topic::BinarySearch;
        bool b4 = stringToTopic("Trees") == Topic::Trees;
        bool b5 = stringToTopic("UnknownCategory") == Topic::Other;

        bool b6 = platformToString(Platform::LeetCode) == "LeetCode";
        bool b7 = stringToPlatform("leetcode") == Platform::LeetCode;
        bool b8 = stringToPlatform("Codeforces") == Platform::Codeforces;
        bool b9 = stringToPlatform("UnknownJudge") == Platform::Custom;

        return b1 && b2 && b3 && b4 && b5 && b6 && b7 && b8 && b9;
    });

    // =========================================================================
    // 2. Validation Business Rules Tests
    // =========================================================================
    std::cout << "\n--- 2. Validation Rules & Guard Rails ---\n";

    // Set up an in-memory repository for validation tests
    std::string tempDummyPath = "build/temp_dummy_validation.csv";
    auto dummyRepo = std::make_shared<codevault::persistence::FileQuestionRepository>(tempDummyPath);
    codevault::services::QuestionService valService(dummyRepo);

    runTest("Valid Question Passes Validation", [&]() {
        codevault::models::Question q(
            "Q-VALID", "Valid Title", "Description",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Meta", codevault::models::Platform::LeetCode,
            "https://leetcode.com/problems/valid",
            codevault::models::Status::Unsolved,
            false, "Notes", 100, 100, 0, 0, 2, {"tag"}
        );
        auto res = valService.validateQuestion(q, false);
        return res.isValid && res.errorMessage.empty();
    });

    runTest("Empty Title Rejected", [&]() {
        codevault::models::Question q;
        q.setId("Q-EMPTY-TITLE");
        q.setTitle("");
        q.setDifficulty(codevault::models::Difficulty::Easy);
        auto res = valService.validateQuestion(q, false);
        return !res.isValid && res.errorMessage.find("title cannot be empty") != std::string::npos;
    });

    runTest("Whitespace-Only Title Rejected", [&]() {
        codevault::models::Question q;
        q.setId("Q-WS-TITLE");
        q.setTitle("    \t  \n  ");
        q.setDifficulty(codevault::models::Difficulty::Easy);
        auto res = valService.validateQuestion(q, false);
        return !res.isValid && res.errorMessage.find("title cannot be empty") != std::string::npos;
    });

    runTest("Empty ID Rejected", [&]() {
        codevault::models::Question q;
        q.setId("");
        q.setTitle("Has Title");
        q.setDifficulty(codevault::models::Difficulty::Easy);
        auto res = valService.validateQuestion(q, false);
        return !res.isValid && res.errorMessage.find("ID cannot be empty") != std::string::npos;
    });

    runTest("Invalid ID Characters Rejected", [&]() {
        codevault::models::Question q;
        q.setId("Q 1001 with spaces!");
        q.setTitle("Has Title");
        q.setDifficulty(codevault::models::Difficulty::Easy);
        auto res = valService.validateQuestion(q, false);
        return !res.isValid && res.errorMessage.find("invalid characters") != std::string::npos;
    });

    runTest("Unknown Difficulty Rejected", [&]() {
        codevault::models::Question q;
        q.setId("Q-DIFF");
        q.setTitle("Has Title");
        q.setDifficulty(codevault::models::Difficulty::Unknown);
        auto res = valService.validateQuestion(q, false);
        return !res.isValid && res.errorMessage.find("Difficulty must be specified") != std::string::npos;
    });

    runTest("Malformed Source URL Rejected", [&]() {
        codevault::models::Question q;
        q.setId("Q-URL");
        q.setTitle("Has Title");
        q.setDifficulty(codevault::models::Difficulty::Medium);
        q.setSourceUrl("ftp://invalid-protocol.com");
        auto res = valService.validateQuestion(q, false);
        return !res.isValid && res.errorMessage.find("must start with http:// or https://") != std::string::npos;
    });

    runTest("URL with Embedded Spaces Rejected", [&]() {
        codevault::models::Question q;
        q.setId("Q-URL2");
        q.setTitle("Has Title");
        q.setDifficulty(codevault::models::Difficulty::Medium);
        q.setSourceUrl("https://leetcode.com/problem with spaces/");
        auto res = valService.validateQuestion(q, false);
        return !res.isValid && res.errorMessage.find("must not contain spaces") != std::string::npos;
    });

    runTest("Revision Priority Out of Bounds Rejected", [&]() {
        codevault::models::Question q;
        q.setId("Q-PRIO");
        q.setTitle("Has Title");
        q.setDifficulty(codevault::models::Difficulty::Medium);
        q.setRevisionPriority(10); // Valid is [1, 5]
        auto res = valService.validateQuestion(q, false);
        return !res.isValid && res.errorMessage.find("between 1") != std::string::npos;
    });

    // Cleanup dummy validation file if created
    std::filesystem::remove(tempDummyPath);

    // =========================================================================
    // 3. Repository Layer & In-Memory Operations
    // =========================================================================
    std::cout << "\n--- 3. Repository CRUD Operations ---\n";

    std::string testRepoPath = "build/test_repository.csv";
    std::filesystem::remove(testRepoPath);
    auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testRepoPath);

    runTest("Repository starts empty for new file", [&]() {
        return repo->count() == 0 && repo->findAll().empty();
    });

    runTest("Repository save and exists", [&]() {
        codevault::models::Question q(
            "Q-1", "Problem One", "Desc 1",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Google", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved,
            false, "", 100, 100, 0, 0, 2, {"arr"}
        );

        bool saved = repo->save(q);
        return saved && repo->count() == 1 && repo->exists("Q-1");
    });

    runTest("Repository findById success and not found", [&]() {
        auto found = repo->findById("Q-1");
        auto notFound = repo->findById("Q-NONEXISTENT");

        return found.has_value() && found->getTitle() == "Problem One" &&
               !notFound.has_value() && !repo->exists("Q-NONEXISTENT");
    });

    runTest("Repository update existing record", [&]() {
        auto qOpt = repo->findById("Q-1");
        if (!qOpt.has_value()) return false;

        auto q = qOpt.value();
        q.setTitle("Problem One Updated");
        q.setStatus(codevault::models::Status::Solved);
        q.setFavorite(true);

        bool updated = repo->save(q);
        auto reloaded = repo->findById("Q-1");

        return updated && repo->count() == 1 &&
               reloaded.has_value() &&
               reloaded->getTitle() == "Problem One Updated" &&
               reloaded->getStatus() == codevault::models::Status::Solved &&
               reloaded->isFavorite() == true;
    });

    runTest("Repository remove existing and nonexistent", [&]() {
        bool delNonExistent = repo->remove("Q-DOES-NOT-EXIST");
        bool delExisting = repo->remove("Q-1");

        return !delNonExistent && delExisting && repo->count() == 0 && !repo->exists("Q-1");
    });

    std::filesystem::remove(testRepoPath);

    // =========================================================================
    // 4. Persistence Roundtrip & Edge Cases
    // =========================================================================
    std::cout << "\n--- 4. Local Persistence & Disk Roundtrip ---\n";

    std::string testPersistPath = "build/test_persistence_roundtrip.csv";
    std::filesystem::remove(testPersistPath);

    runTest("Persistence Roundtrip: Save, Reload, Special Characters Escaping", [&]() {
        {
            codevault::persistence::FileQuestionRepository writerRepo(testPersistPath);

            codevault::models::Question q1(
                "Q-101", "Two Sum, Sorted", "Find two numbers, target = sum.",
                codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy,
                "Google", codevault::models::Platform::LeetCode,
                "https://leetcode.com/problems/two-sum/",
                codevault::models::Status::Solved,
                true, "Notes with \"quotes\" and commas, here.",
                1000, 2000, 1500, 3000, 1,
                {"hash-table", "two-pointers"},
                "local_user"
            );

            codevault::models::Question q2(
                "Q-102", "Median of Two Sorted Arrays", "Hard problem description.",
                codevault::models::Topic::BinarySearch,
                codevault::models::Difficulty::Hard,
                "Apple", codevault::models::Platform::LeetCode,
                "https://leetcode.com/problems/median/",
                codevault::models::Status::Unsolved,
                false, "Binary search partition.",
                1100, 2100, 0, 3100, 1,
                {"binary-search", "divide-and-conquer"},
                "local_user"
            );

            writerRepo.save(q1);
            writerRepo.save(q2);
        } // writerRepo closes, file written to disk

        // Now open fresh repository from disk
        codevault::persistence::FileQuestionRepository readerRepo(testPersistPath);
        if (readerRepo.count() != 2) return false;

        auto q1Opt = readerRepo.findById("Q-101");
        if (!q1Opt.has_value()) return false;
        const auto& q1 = q1Opt.value();

        bool q1Match = q1.getTitle() == "Two Sum, Sorted" &&
                       q1.getDescription() == "Find two numbers, target = sum." &&
                       q1.getDifficulty() == codevault::models::Difficulty::Easy &&
                       q1.getStatus() == codevault::models::Status::Solved &&
                       q1.isFavorite() == true &&
                       q1.getNotes() == "Notes with \"quotes\" and commas, here." &&
                       q1.getTags().size() == 2 &&
                       q1.getTags()[0] == "hash-table";

        auto q2Opt = readerRepo.findById("Q-102");
        if (!q2Opt.has_value()) return false;
        const auto& q2 = q2Opt.value();

        bool q2Match = q2.getTitle() == "Median of Two Sorted Arrays" &&
                       q2.getDifficulty() == codevault::models::Difficulty::Hard &&
                       q2.getStatus() == codevault::models::Status::Unsolved &&
                       q2.isFavorite() == false;

        return q1Match && q2Match;
    });

    runTest("Persistence Update and Delete Survive Disk Reload", [&]() {
        // Update Q-101 and delete Q-102
        {
            codevault::persistence::FileQuestionRepository modRepo(testPersistPath);
            auto q1Opt = modRepo.findById("Q-101");
            if (!q1Opt.has_value()) return false;

            auto q1 = q1Opt.value();
            q1.setTitle("Two Sum Renamed");
            modRepo.save(q1);
            modRepo.remove("Q-102");
        }

        // Reload fresh from disk
        codevault::persistence::FileQuestionRepository verifyRepo(testPersistPath);
        if (verifyRepo.count() != 1) return false;

        auto q1 = verifyRepo.findById("Q-101");
        bool q2Deleted = !verifyRepo.exists("Q-102");

        return q1.has_value() && q1->getTitle() == "Two Sum Renamed" && q2Deleted;
    });

    std::filesystem::remove(testPersistPath);

    runTest("Graceful Handling of Empty Persistence File", []() {
        std::string emptyPath = "build/test_empty.csv";
        {
            std::ofstream f(emptyPath);
            // Empty file
        }

        codevault::persistence::FileQuestionRepository repo(emptyPath);
        bool ok = repo.count() == 0 && repo.findAll().empty();
        std::filesystem::remove(emptyPath);
        return ok;
    });

    runTest("Graceful Handling of Missing Persistence File", []() {
        std::string nonExistentPath = "build/non_existent_folder/missing.csv";
        std::filesystem::remove(nonExistentPath);

        codevault::persistence::FileQuestionRepository repo(nonExistentPath);
        return repo.count() == 0 && repo.findAll().empty();
    });

    runTest("Graceful Handling of Corrupted Lines in CSV", []() {
        std::string corruptPath = "build/test_corrupt.csv";
        {
            std::ofstream f(corruptPath);
            f << "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\n";
            f << "Q-CORRUPT-1,Too,Few,Tokens\n"; // Only 4 tokens -> should be skipped
            f << "\n"; // Empty line -> skipped
            f << "# Comment line -> skipped\n";
            f << "Q-GOOD,Good Question,A fine question,Arrays,Easy,Google,LeetCode,,Solved,true,Notes,100,100,0,0,1,tag1,local_user\n";
            f << ",EmptyID,Desc,Arrays,Easy,Google,LeetCode,,Solved,true,Notes,100,100,0,0,1,tag1,local_user\n"; // Empty ID -> skipped
        }

        codevault::persistence::FileQuestionRepository repo(corruptPath);
        bool ok = repo.count() == 1 && repo.exists("Q-GOOD") && !repo.exists("Q-CORRUPT-1");
        std::filesystem::remove(corruptPath);
        return ok;
    });

    // =========================================================================
    // 5. QuestionService Application Logic Tests
    // =========================================================================
    std::cout << "\n--- 5. QuestionService Business Logic & Lifecycle ---\n";

    std::string testServicePath = "build/test_service.csv";
    std::filesystem::remove(testServicePath);
    auto sRepo = std::make_shared<codevault::persistence::FileQuestionRepository>(testServicePath);
    codevault::services::QuestionService service(sRepo);

    runTest("Service createQuestion: Valid Question Successfully Added", [&]() {
        codevault::models::Question q(
            "Q-2001", "Binary Search Problem", "Implement standard binary search.",
            codevault::models::Topic::BinarySearch,
            codevault::models::Difficulty::Easy,
            "Amazon", codevault::models::Platform::LeetCode,
            "https://leetcode.com/problems/binary-search/",
            codevault::models::Status::Solved,
            true, "mid = low + (high - low)/2",
            100, 100, 0, 0, 2, {"binary-search"}
        );

        auto res = service.createQuestion(q);
        return res.isValid && service.getQuestionCount() == 1 && service.questionExists("Q-2001");
    });

    runTest("Service createQuestion: Duplicate ID Blocked", [&]() {
        codevault::models::Question dup(
            "Q-2001", "Another Problem", "Duplicate ID attempt",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Medium,
            "Apple", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved,
            false, "", 100, 100, 0, 0, 2, {}
        );

        auto res = service.createQuestion(dup);
        return !res.isValid && res.errorMessage.find("already exists") != std::string::npos &&
               service.getQuestionCount() == 1;
    });

    runTest("Service createQuestion: Validation Failure Blocks Persistence", [&]() {
        codevault::models::Question invalid(
            "Q-INVALID", "", "Empty title", // Invalid empty title
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Medium,
            "Apple", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved,
            false, "", 100, 100, 0, 0, 2, {}
        );

        auto res = service.createQuestion(invalid);
        return !res.isValid && service.getQuestionCount() == 1;
    });

    runTest("Service updateQuestion: Modifies Existing Question", [&]() {
        auto qOpt = service.getQuestionById("Q-2001");
        if (!qOpt.has_value()) return false;

        auto q = qOpt.value();
        q.setTitle("Binary Search (Optimized)");
        q.setDifficulty(codevault::models::Difficulty::Medium);

        auto res = service.updateQuestion(q);
        auto reloaded = service.getQuestionById("Q-2001");

        return res.isValid && reloaded.has_value() &&
               reloaded->getTitle() == "Binary Search (Optimized)" &&
               reloaded->getDifficulty() == codevault::models::Difficulty::Medium;
    });

    runTest("Service updateQuestion: Nonexistent ID Rejected", [&]() {
        codevault::models::Question nonExistent(
            "Q-DOES-NOT-EXIST", "Phantom Problem", "Desc",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Google", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved,
            false, "", 100, 100, 0, 0, 2, {}
        );

        auto res = service.updateQuestion(nonExistent);
        return !res.isValid && res.errorMessage.find("does not exist") != std::string::npos;
    });

    runTest("Service ID Auto-Generation Strategy", [&]() {
        // Currently Q-2001 exists. Next should be Q-2002.
        std::string nextId = service.generateNextId();
        return nextId == "Q-2002";
    });

    runTest("Service Aggregations: Difficulty & Status Counts", [&]() {
        // Add a second question
        codevault::models::Question q2(
            "Q-2002", "Invert Binary Tree", "Invert tree",
            codevault::models::Topic::Trees,
            codevault::models::Difficulty::Easy,
            "Google", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::InProgress,
            false, "", 100, 100, 0, 0, 2, {"tree"}
        );
        service.createQuestion(q2);

        size_t total = service.getQuestionCount();
        size_t easy = service.countByDifficulty(codevault::models::Difficulty::Easy);
        size_t med = service.countByDifficulty(codevault::models::Difficulty::Medium);
        size_t hard = service.countByDifficulty(codevault::models::Difficulty::Hard);
        size_t solved = service.countByStatus(codevault::models::Status::Solved);
        size_t inProg = service.countByStatus(codevault::models::Status::InProgress);

        return total == 2 && easy == 1 && med == 1 && hard == 0 && solved == 1 && inProg == 1;
    });

    runTest("Service deleteQuestion: Deletes Question and Decrements Count", [&]() {
        bool delSuccess = service.deleteQuestion("Q-2001");
        bool delFail = service.deleteQuestion("Q-2001"); // Second delete should fail

        return delSuccess && !delFail && service.getQuestionCount() == 1 &&
               !service.questionExists("Q-2001") && service.questionExists("Q-2002");
    });

    std::filesystem::remove(testServicePath);

    // =========================================================================
    // 6. Prefix Trie Tests
    // =========================================================================
    std::cout << "\n--- 6. Custom PrefixTrie Engine ---\n";

    runTest("Trie: Initial State is Empty", []() {
        codevault::dsa::PrefixTrie trie;
        return trie.empty() && trie.size() == 0 && !trie.contains("anything") && !trie.startsWith("a");
    });

    runTest("Trie: Single Word Insertion & Exact Search", []() {
        codevault::dsa::PrefixTrie trie;
        bool inserted = trie.insert("Binary Search");
        return inserted && trie.size() == 1 && !trie.empty() &&
               trie.contains("Binary Search") &&
               !trie.contains("Binary") &&
               !trie.contains("Linear Search");
    });

    runTest("Trie: Duplicate Insert Does Not Increase Size", []() {
        codevault::dsa::PrefixTrie trie;
        bool first = trie.insert("Two Sum");
        bool second = trie.insert("Two Sum");
        return first && !second && trie.size() == 1;
    });

    runTest("Trie: Case-Insensitive Normalization", []() {
        codevault::dsa::PrefixTrie trie;
        trie.insert("Merge Sort");
        return trie.contains("merge sort") &&
               trie.contains("MERGE SORT") &&
               trie.contains("MeRgE sOrT") &&
               trie.startsWith("mer") &&
               trie.startsWith("MER");
    });

    runTest("Trie: Special Characters & Spacing Support", []() {
        codevault::dsa::PrefixTrie trie;
        trie.insert("LRU Cache (LeetCode-146)");
        trie.insert("C++ STL Vector");
        return trie.contains("LRU Cache (LeetCode-146)") &&
               trie.contains("c++ stl vector") &&
               trie.startsWith("c++") &&
               trie.startsWith("lru cache");
    });

    runTest("Trie: Prefix Lookup (startsWith)", []() {
        codevault::dsa::PrefixTrie trie;
        trie.insert("Binary Search");
        trie.insert("Binary Tree Traversal");
        trie.insert("Bit Manipulation");
        return trie.startsWith("bin") &&
               trie.startsWith("bit") &&
               trie.startsWith("bi") &&
               !trie.startsWith("byte") &&
               !trie.startsWith("xyz");
    });

    runTest("Trie: Autocomplete / getWordsWithPrefix Sorted Results", []() {
        codevault::dsa::PrefixTrie trie;
        trie.insert("Binary Search");
        trie.insert("Binary Tree Traversal");
        trie.insert("Binary Number with Alternating Bits");
        trie.insert("Merge Sort");
        trie.insert("Merge Intervals");

        auto results = trie.autocomplete("bin");
        bool countMatch = results.size() == 3;
        bool hasAll = (std::find(results.begin(), results.end(), "Binary Search") != results.end()) &&
                      (std::find(results.begin(), results.end(), "Binary Tree Traversal") != results.end()) &&
                      (std::find(results.begin(), results.end(), "Binary Number with Alternating Bits") != results.end());

        auto mergeResults = trie.getWordsWithPrefix("merge");
        bool mergeMatch = mergeResults.size() == 2;

        auto noneResults = trie.autocomplete("quick");
        return countMatch && hasAll && mergeMatch && noneResults.empty();
    });

    runTest("Trie: Remove Existing Word & Prune", []() {
        codevault::dsa::PrefixTrie trie;
        trie.insert("Binary");
        trie.insert("Binary Search");
        bool rem1 = trie.remove("Binary Search");
        bool rem2 = trie.remove("Binary Search"); // Already removed

        return rem1 && !rem2 && trie.size() == 1 &&
               !trie.contains("Binary Search") &&
               trie.contains("Binary");
    });

    runTest("Trie: Remove Nonexistent Word Returns False", []() {
        codevault::dsa::PrefixTrie trie;
        trie.insert("QuickSort");
        return !trie.remove("MergeSort") && trie.size() == 1;
    });

    runTest("Trie: Empty String Handling is Safe", []() {
        codevault::dsa::PrefixTrie trie;
        bool ins = trie.insert("");
        bool cont = trie.contains("");
        bool rem = trie.remove("");
        return !ins && !cont && !rem && trie.size() == 0;
    });

    runTest("Trie: Clear Resets Complete State", []() {
        codevault::dsa::PrefixTrie trie;
        trie.insert("Alpha");
        trie.insert("Beta");
        trie.insert("Gamma");
        trie.clear();
        return trie.empty() && trie.size() == 0 && !trie.contains("Alpha");
    });

    runTest("Trie: Copy Constructor & Deep Copy Invariant", []() {
        codevault::dsa::PrefixTrie trie1;
        trie1.insert("Original Word");
        codevault::dsa::PrefixTrie trie2 = trie1;
        trie2.insert("New Word");

        return trie1.size() == 1 && trie2.size() == 2 &&
               !trie1.contains("New Word") && trie2.contains("Original Word");
    });

    // =========================================================================
    // 7. Custom MinHeap Tests
    // =========================================================================
    std::cout << "\n--- 7. Custom MinHeap Engine ---\n";

    runTest("MinHeap: Empty State & Underflow Guarantees", []() {
        codevault::dsa::MinHeap<int> heap;
        bool caughtTop = false;
        bool caughtPop = false;
        try { heap.top(); } catch (const std::underflow_error&) { caughtTop = true; }
        try { heap.pop(); } catch (const std::underflow_error&) { caughtPop = true; }
        return heap.empty() && heap.size() == 0 && caughtTop && caughtPop;
    });

    runTest("MinHeap: Single Element Push, Top, Pop", []() {
        codevault::dsa::MinHeap<int> heap;
        heap.push(42);
        bool sizeOne = heap.size() == 1 && !heap.empty() && heap.top() == 42;
        heap.pop();
        return sizeOne && heap.empty() && heap.size() == 0;
    });

    runTest("MinHeap: Push Ordering Maintains Minimum at Root", []() {
        codevault::dsa::MinHeap<int> heap;
        heap.push(50);
        heap.push(30);
        heap.push(70);
        heap.push(10);
        heap.push(40);

        return heap.top() == 10 && heap.size() == 5;
    });

    runTest("MinHeap: Pop Ordering Extracts Elements in Ascending Order", []() {
        codevault::dsa::MinHeap<int> heap;
        std::vector<int> inputs = {65, 23, 89, 12, 45, 1, 99, 34};
        for (int v : inputs) heap.push(v);

        std::vector<int> extracted;
        while (!heap.empty()) {
            extracted.push_back(heap.extractMin());
        }

        std::vector<int> sorted = inputs;
        std::sort(sorted.begin(), sorted.end());
        return extracted == sorted;
    });

    runTest("MinHeap: Duplicate Priorities Handled Correctly", []() {
        codevault::dsa::MinHeap<int> heap;
        heap.push(20);
        heap.push(10);
        heap.push(20);
        heap.push(10);
        heap.push(20);

        std::vector<int> extracted;
        while (!heap.empty()) extracted.push_back(heap.extractMin());

        std::vector<int> expected = {10, 10, 20, 20, 20};
        return extracted == expected;
    });

    runTest("MinHeap: Clear Reinitializes Buffer", []() {
        codevault::dsa::MinHeap<int> heap;
        heap.push(100);
        heap.push(200);
        heap.clear();
        return heap.empty() && heap.size() == 0;
    });

    runTest("MinHeap: Range Constructor / BuildHeap", []() {
        std::vector<int> nums = {40, 10, 30, 50, 20};
        codevault::dsa::MinHeap<int> heap(nums.begin(), nums.end());
        return heap.top() == 10 && heap.size() == 5;
    });

    runTest("MinHeap: Custom RevisionItem Comparator", []() {
        codevault::dsa::MinHeap<codevault::models::RevisionItem, codevault::models::RevisionItemComparator> revHeap;
        revHeap.push({"Q-3", 3000, 2});
        revHeap.push({"Q-1", 1000, 3});
        revHeap.push({"Q-2", 2000, 1});
        revHeap.push({"Q-1B", 1000, 1});

        auto topItem = revHeap.extractMin();
        bool firstCorrect = topItem.questionId == "Q-1B";

        auto secondItem = revHeap.extractMin();
        bool secondCorrect = secondItem.questionId == "Q-1";

        auto thirdItem = revHeap.extractMin();
        bool thirdCorrect = thirdItem.questionId == "Q-2";

        auto fourthItem = revHeap.extractMin();
        bool fourthCorrect = fourthItem.questionId == "Q-3";

        return firstCorrect && secondCorrect && thirdCorrect && fourthCorrect;
    });

    // =========================================================================
    // 8. Custom Queue Tests
    // =========================================================================
    std::cout << "\n--- 8. Custom Queue Engine ---\n";

    runTest("Queue: Empty State & Underflow Guarantees", []() {
        codevault::dsa::Queue<std::string> q;
        bool caughtFront = false;
        bool caughtBack = false;
        bool caughtDeq = false;
        try { q.front(); } catch (const std::underflow_error&) { caughtFront = true; }
        try { q.back(); } catch (const std::underflow_error&) { caughtBack = true; }
        try { q.dequeue(); } catch (const std::underflow_error&) { caughtDeq = true; }

        return q.empty() && q.size() == 0 && caughtFront && caughtBack && caughtDeq;
    });

    runTest("Queue: Single Element Enqueue & Dequeue", []() {
        codevault::dsa::Queue<int> q;
        q.enqueue(101);
        bool singleState = q.size() == 1 && !q.empty() && q.front() == 101 && q.back() == 101;
        q.dequeue();
        return singleState && q.empty() && q.size() == 0;
    });

    runTest("Queue: FIFO Ordering Guaranteed", []() {
        codevault::dsa::Queue<std::string> q;
        q.enqueue("Problem 1");
        q.enqueue("Problem 2");
        q.enqueue("Problem 3");

        bool order1 = q.front() == "Problem 1" && q.back() == "Problem 3";
        q.dequeue();
        bool order2 = q.front() == "Problem 2";
        q.dequeue();
        bool order3 = q.front() == "Problem 3";
        q.dequeue();
        return order1 && order2 && order3 && q.empty();
    });

    runTest("Queue: Large Batch Enqueue & Sequential Dequeue", []() {
        codevault::dsa::Queue<int> q;
        const int count = 500;
        for (int i = 0; i < count; ++i) {
            q.enqueue(i);
        }
        if (q.size() != count || q.front() != 0 || q.back() != count - 1) {
            return false;
        }
        for (int i = 0; i < count; ++i) {
            if (q.front() != i) return false;
            q.dequeue();
        }
        return q.empty() && q.size() == 0;
    });

    runTest("Queue: Clear Resets All Nodes", []() {
        codevault::dsa::Queue<int> q;
        q.enqueue(1);
        q.enqueue(2);
        q.enqueue(3);
        q.clear();
        return q.empty() && q.size() == 0;
    });

    runTest("Queue: Rule of 5 Deep Copy & Move Semantics", []() {
        codevault::dsa::Queue<int> q1;
        q1.enqueue(10);
        q1.enqueue(20);

        codevault::dsa::Queue<int> q2 = q1;
        q2.enqueue(30);

        bool copyIndependent = q1.size() == 2 && q2.size() == 3;

        codevault::dsa::Queue<int> q3 = std::move(q2);
        bool moveWorked = q3.size() == 3 && q3.front() == 10 && q3.back() == 30;

        return copyIndependent && moveWorked;
    });

    // =========================================================================
    // 9. Custom Stack Tests
    // =========================================================================
    std::cout << "\n--- 9. Custom Stack Engine ---\n";

    runTest("Stack: Empty State & Underflow Guarantees", []() {
        codevault::dsa::Stack<std::string> st;
        bool caughtTop = false;
        bool caughtPop = false;
        try { st.top(); } catch (const std::underflow_error&) { caughtTop = true; }
        try { st.pop(); } catch (const std::underflow_error&) { caughtPop = true; }
        return st.empty() && st.size() == 0 && caughtTop && caughtPop;
    });

    runTest("Stack: Single Element Push & Pop", []() {
        codevault::dsa::Stack<int> st;
        st.push(999);
        bool singleState = st.size() == 1 && !st.empty() && st.top() == 999;
        st.pop();
        return singleState && st.empty() && st.size() == 0;
    });

    runTest("Stack: LIFO Ordering Guaranteed", []() {
        codevault::dsa::Stack<std::string> st;
        st.push("Q-1001");
        st.push("Q-1004");
        st.push("Q-1007");

        bool top1 = st.top() == "Q-1007";
        st.pop();
        bool top2 = st.top() == "Q-1004";
        st.pop();
        bool top3 = st.top() == "Q-1001";
        st.pop();

        return top1 && top2 && top3 && st.empty();
    });

    runTest("Stack: Clear Frees All Resources", []() {
        codevault::dsa::Stack<int> st;
        st.push(1);
        st.push(2);
        st.clear();
        return st.empty() && st.size() == 0;
    });

    runTest("Stack: Rule of 5 Deep Copy & Move Semantics", []() {
        codevault::dsa::Stack<std::string> s1;
        s1.push("Bottom");
        s1.push("Top");

        codevault::dsa::Stack<std::string> s2 = s1;
        s2.push("NewTop");

        bool copyCheck = s1.size() == 2 && s2.size() == 3 && s1.top() == "Top" && s2.top() == "NewTop";

        codevault::dsa::Stack<std::string> s3 = std::move(s2);
        bool moveCheck = s3.size() == 3 && s3.top() == "NewTop";

        return copyCheck && moveCheck;
    });

    // =========================================================================
    // 10. Custom DoublyLinkedList Tests
    // =========================================================================
    std::cout << "\n--- 10. Custom DoublyLinkedList Engine ---\n";

    runTest("DoublyLinkedList: Empty State & Underflow", []() {
        codevault::dsa::DoublyLinkedList<int> list;
        bool caughtFront = false;
        bool caughtBack = false;
        bool caughtPopF = false;
        bool caughtPopB = false;
        try { list.front(); } catch (const std::underflow_error&) { caughtFront = true; }
        try { list.back(); } catch (const std::underflow_error&) { caughtBack = true; }
        try { list.popFront(); } catch (const std::underflow_error&) { caughtPopF = true; }
        try { list.popBack(); } catch (const std::underflow_error&) { caughtPopB = true; }

        return list.empty() && list.size() == 0 && caughtFront && caughtBack && caughtPopF && caughtPopB;
    });

    runTest("DoublyLinkedList: Single Element pushFront & popFront", []() {
        codevault::dsa::DoublyLinkedList<std::string> list;
        list.pushFront("Solo");
        bool singleState = list.size() == 1 && list.front() == "Solo" && list.back() == "Solo";
        list.popFront();
        return singleState && list.empty() && list.size() == 0;
    });

    runTest("DoublyLinkedList: Single Element pushBack & popBack", []() {
        codevault::dsa::DoublyLinkedList<std::string> list;
        list.pushBack("Solo");
        bool singleState = list.size() == 1 && list.front() == "Solo" && list.back() == "Solo";
        list.popBack();
        return singleState && list.empty() && list.size() == 0;
    });

    runTest("DoublyLinkedList: Front and Back Insertions", []() {
        codevault::dsa::DoublyLinkedList<int> list;
        list.pushBack(20);
        list.pushFront(10);
        list.pushBack(30);

        std::vector<int> expected = {10, 20, 30};
        return list.toVector() == expected && list.front() == 10 && list.back() == 30 && list.size() == 3;
    });

    runTest("DoublyLinkedList: Front & Back Removal on Multi-Element List", []() {
        codevault::dsa::DoublyLinkedList<int> list;
        list.pushBack(10);
        list.pushBack(20);
        list.pushBack(30);
        list.pushBack(40);

        list.popFront();
        list.popBack();

        std::vector<int> expected = {20, 30};
        return list.toVector() == expected && list.front() == 20 && list.back() == 30 && list.size() == 2;
    });

    runTest("DoublyLinkedList: Middle Insertion by Index", []() {
        codevault::dsa::DoublyLinkedList<std::string> list;
        list.pushBack("A");
        list.pushBack("C");
        list.insert(1, "B");

        std::vector<std::string> expected = {"A", "B", "C"};
        return list.toVector() == expected && list.size() == 3;
    });

    runTest("DoublyLinkedList: Middle Removal by Index (removeAt)", []() {
        codevault::dsa::DoublyLinkedList<std::string> list;
        list.pushBack("X");
        list.pushBack("Y");
        list.pushBack("Z");
        list.removeAt(1);

        std::vector<std::string> expected = {"X", "Z"};
        return list.toVector() == expected && list.size() == 2;
    });

    runTest("DoublyLinkedList: Removal by Value", []() {
        codevault::dsa::DoublyLinkedList<int> list;
        list.pushBack(100);
        list.pushBack(200);
        list.pushBack(300);

        bool remMid = list.remove(200);
        bool remNon = list.remove(999);

        std::vector<int> expected = {100, 300};
        return remMid && !remNon && list.toVector() == expected && list.size() == 2;
    });

    runTest("DoublyLinkedList: Forward & Backward Bidirectional Traversal", []() {
        codevault::dsa::DoublyLinkedList<int> list;
        for (int i = 1; i <= 5; ++i) list.pushBack(i);

        std::vector<int> forward = list.toVector();
        std::vector<int> backward = list.toReverseVector();

        std::vector<int> expF = {1, 2, 3, 4, 5};
        std::vector<int> expB = {5, 4, 3, 2, 1};

        return forward == expF && backward == expB;
    });

    runTest("DoublyLinkedList: Range-Based For Loop with Iterators", []() {
        codevault::dsa::DoublyLinkedList<int> list;
        list.pushBack(2);
        list.pushBack(4);
        list.pushBack(6);

        int sum = 0;
        for (int v : list) {
            sum += v;
        }
        return sum == 12;
    });

    runTest("DoublyLinkedList: Clear Reinitializes List", []() {
        codevault::dsa::DoublyLinkedList<int> list;
        list.pushBack(1);
        list.pushBack(2);
        list.clear();
        return list.empty() && list.size() == 0;
    });

    runTest("DoublyLinkedList: Rule of 5 Deep Copy & Move Semantics", []() {
        codevault::dsa::DoublyLinkedList<int> l1;
        l1.pushBack(10);
        l1.pushBack(20);

        codevault::dsa::DoublyLinkedList<int> l2 = l1;
        l2.pushBack(30);

        bool copyCheck = l1.size() == 2 && l2.size() == 3 && l1.toVector().size() == 2;

        codevault::dsa::DoublyLinkedList<int> l3 = std::move(l2);
        bool moveCheck = l3.size() == 3 && l3.back() == 30;

        return copyCheck && moveCheck;
    });

    // =========================================================================
    // 11. DSA Service Integration Tests
    // =========================================================================
    std::cout << "\n--- 11. DSA Service Integration Tests ---\n";

    runTest("Integration: QuestionService Ingestion Automatically Populates Trie Index", []() {
        std::string testPath = "build/test_integration_repo.csv";
        std::filesystem::remove(testPath);
        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);

        codevault::models::Question q1("Q-INT1", "Binary Search", "Desc", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Google", codevault::models::Platform::LeetCode, "", codevault::models::Status::Solved, false, "", 100, 100, 0, 0, 2, {});
        codevault::models::Question q2("Q-INT2", "Binary Tree Inorder", "Desc", codevault::models::Topic::Trees, codevault::models::Difficulty::Medium, "Amazon", codevault::models::Platform::LeetCode, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {});
        repo->save(q1);
        repo->save(q2);

        codevault::services::QuestionService qService(repo);
        auto searchService = qService.getSearchService();

        bool indexed = searchService && searchService->getIndexedTitleCount() == 2 &&
                       searchService->containsTitle("Binary Search") &&
                       searchService->containsTitle("Binary Tree Inorder");

        std::filesystem::remove(testPath);
        return indexed;
    });

    runTest("Integration: Prefix Search Returns Exact Question Matches", []() {
        std::string testPath = "build/test_integration_search.csv";
        std::filesystem::remove(testPath);
        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        codevault::services::QuestionService qService(repo);

        codevault::models::Question q1("Q-INT1", "Merge Intervals", "Desc", codevault::models::Topic::Arrays, codevault::models::Difficulty::Medium, "Google", codevault::models::Platform::LeetCode, "", codevault::models::Status::Solved, false, "", 100, 100, 0, 0, 2, {});
        codevault::models::Question q2("Q-INT2", "Merge k Sorted Lists", "Desc", codevault::models::Topic::Heaps, codevault::models::Difficulty::Hard, "Facebook", codevault::models::Platform::LeetCode, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {});
        codevault::models::Question q3("Q-INT3", "Quick Select", "Desc", codevault::models::Topic::Arrays, codevault::models::Difficulty::Medium, "Apple", codevault::models::Platform::LeetCode, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {});

        qService.createQuestion(q1);
        qService.createQuestion(q2);
        qService.createQuestion(q3);

        auto matches = qService.searchQuestionsByTitlePrefix("mer");
        bool countMatch = matches.size() == 2;
        bool titlesCorrect = (matches.size() == 2) &&
                             (matches[0].getTitle().rfind("Merge", 0) == 0) &&
                             (matches[1].getTitle().rfind("Merge", 0) == 0);

        std::filesystem::remove(testPath);
        return countMatch && titlesCorrect;
    });

    runTest("Integration: Updating Question Title Updates Trie Index", []() {
        std::string testPath = "build/test_integration_update.csv";
        std::filesystem::remove(testPath);
        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        codevault::services::QuestionService qService(repo);

        codevault::models::Question q("Q-UPD", "Old Dynamic Programming", "Desc", codevault::models::Topic::DynamicProgramming, codevault::models::Difficulty::Hard, "Netflix", codevault::models::Platform::LeetCode, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {});
        qService.createQuestion(q);

        bool beforeOldFound = qService.getSearchService()->containsTitle("Old Dynamic Programming");

        q.setTitle("New Dynamic Programming");
        qService.updateQuestion(q);

        bool afterOldGone = !qService.getSearchService()->containsTitle("Old Dynamic Programming");
        bool afterNewFound = qService.getSearchService()->containsTitle("New Dynamic Programming");

        std::filesystem::remove(testPath);
        return beforeOldFound && afterOldGone && afterNewFound;
    });

    runTest("Integration: Deleting Question Removes Title From Trie Index", []() {
        std::string testPath = "build/test_integration_delete.csv";
        std::filesystem::remove(testPath);
        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        codevault::services::QuestionService qService(repo);

        codevault::models::Question q("Q-DEL", "Doomed Question", "Desc", codevault::models::Topic::Strings, codevault::models::Difficulty::Easy, "Amazon", codevault::models::Platform::LeetCode, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {});
        qService.createQuestion(q);

        bool existsBefore = qService.getSearchService()->containsTitle("Doomed Question");
        qService.deleteQuestion("Q-DEL");
        bool existsAfter = qService.getSearchService()->containsTitle("Doomed Question");

        std::filesystem::remove(testPath);
        return existsBefore && !existsAfter;
    });

    runTest("Integration: RevisionService Schedules and Extracts MinHeap Priorities", []() {
        codevault::services::RevisionService revService;
        std::vector<codevault::models::Question> questions = {
            codevault::models::Question("Q-1", "Problem 1", "", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 5000, 3, {}),
            codevault::models::Question("Q-2", "Problem 2", "", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 2000, 1, {}),
            codevault::models::Question("Q-3", "Problem 3", "", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 3000, 2, {})
        };

        revService.loadFromQuestions(questions);
        bool hasDue = revService.hasDueRevisions() && revService.getDueRevisionsCount() == 3;

        auto next1 = revService.popNextDueRevision();
        auto next2 = revService.popNextDueRevision();
        auto next3 = revService.popNextDueRevision();

        bool orderCorrect = next1.has_value() && next1->questionId == "Q-2" &&
                            next2.has_value() && next2->questionId == "Q-3" &&
                            next3.has_value() && next3->questionId == "Q-1";

        return hasDue && orderCorrect && !revService.hasDueRevisions();
    });

    runTest("Integration: PracticeService Queue Enqueues, Completes, and Cycles Questions", []() {
        codevault::services::PracticeService practice;
        practice.startSession({"Q-101", "Q-102", "Q-103"});

        bool countInit = practice.getRemainingCount() == 3;
        bool firstQ = practice.getCurrentQuestion() == "Q-101";

        auto completed = practice.completeCurrentQuestion();
        bool compCorrect = completed == "Q-101" && practice.getCurrentQuestion() == "Q-102";

        bool skipped = practice.skipCurrentQuestion();
        bool afterSkip = skipped && practice.getCurrentQuestion() == "Q-103";

        practice.completeCurrentQuestion();
        bool lastQ = practice.getCurrentQuestion() == "Q-102";
        practice.completeCurrentQuestion();

        return countInit && firstQ && compCorrect && afterSkip && lastQ && !practice.hasQuestions();
    });

    runTest("Integration: RecentHistoryService Stack Tracks Views & Suppresses Duplicate Consecutive", []() {
        codevault::services::RecentHistoryService history;
        history.recordView("Q-1001");
        history.recordView("Q-1001");
        history.recordView("Q-1004");
        history.recordView("Q-1007");

        bool sizeCheck = history.size() == 3;
        bool topCheck = history.getLatest() == "Q-1007";

        auto back1 = history.goBack();
        bool backCheck = back1 == "Q-1004";

        auto historyList = history.getHistory();
        std::vector<std::string> exp = {"Q-1004", "Q-1001"};

        return sizeCheck && topCheck && backCheck && historyList == exp;
    });

    runTest("Integration: ProblemPlaylistService DoublyLinkedList Allows Bidirectional Walkthrough", []() {
        codevault::services::ProblemPlaylistService playlist;
        playlist.loadPlaylist({"P-1", "P-2", "P-3"});

        bool currInit = playlist.getCurrentProblem() == "P-1";
        bool next1 = playlist.nextProblem() == "P-2";
        bool next2 = playlist.nextProblem() == "P-3";
        bool nextEnd = !playlist.nextProblem().has_value();

        bool prev1 = playlist.previousProblem() == "P-2";
        bool prev2 = playlist.previousProblem() == "P-1";

        playlist.insertAfterCurrent("P-1.5");
        auto all = playlist.getAllProblems();
        std::vector<std::string> expAll = {"P-1", "P-1.5", "P-2", "P-3"};

        auto removed = playlist.removeCurrentProblem();
        bool remCheck = removed == "P-1" && playlist.getCurrentProblem() == "P-1.5";

        return currInit && next1 && next2 && nextEnd && prev1 && prev2 && (all == expAll) && remCheck;
    });

    // =========================================================================
    // 8. Custom Sorting Algorithms (MergeSort & QuickSort)
    // =========================================================================
    std::cout << "\n--- 8. Custom Sorting Algorithms (MergeSort & QuickSort) ---\n";

    runTest("DSA: MergeSort - Ascending Order", []() {
        std::vector<int> data = {7, 2, 9, 1, 5, 3, 8, 4, 6};
        codevault::dsa::mergeSort(data.begin(), data.end(), std::less<int>());
        std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        return data == expected;
    });

    runTest("DSA: MergeSort - Descending Order", []() {
        std::vector<int> data = {1, 8, 3, 9, 2, 7, 4, 6, 5};
        codevault::dsa::mergeSort(data.begin(), data.end(), std::greater<int>());
        std::vector<int> expected = {9, 8, 7, 6, 5, 4, 3, 2, 1};
        return data == expected;
    });

    runTest("DSA: MergeSort - Already Sorted and Reverse Sorted", []() {
        std::vector<int> sorted = {10, 20, 30, 40, 50};
        codevault::dsa::mergeSort(sorted.begin(), sorted.end(), std::less<int>());
        bool sortedOk = (sorted == std::vector<int>{10, 20, 30, 40, 50});

        std::vector<int> reversed = {50, 40, 30, 20, 10};
        codevault::dsa::mergeSort(reversed.begin(), reversed.end(), std::less<int>());
        bool reversedOk = (reversed == std::vector<int>{10, 20, 30, 40, 50});

        return sortedOk && reversedOk;
    });

    runTest("DSA: MergeSort - Duplicate Elements", []() {
        std::vector<int> data = {5, 1, 5, 3, 5, 2, 1, 3};
        codevault::dsa::mergeSort(data.begin(), data.end(), std::less<int>());
        std::vector<int> expected = {1, 1, 2, 3, 3, 5, 5, 5};
        return data == expected;
    });

    runTest("DSA: MergeSort - Empty Range and Single Element", []() {
        std::vector<int> emptyVec;
        codevault::dsa::mergeSort(emptyVec.begin(), emptyVec.end(), std::less<int>());
        bool emptyOk = emptyVec.empty();

        std::vector<int> single = {42};
        codevault::dsa::mergeSort(single.begin(), single.end(), std::less<int>());
        bool singleOk = (single.size() == 1 && single[0] == 42);

        return emptyOk && singleOk;
    });

    runTest("DSA: MergeSort - Guaranteed Stability", []() {
        // Pairs of (Key, InsertionIndex). When sorted by Key, elements with equal keys
        // must strictly preserve their original relative InsertionIndex order.
        std::vector<std::pair<int, int>> pairs = {
            {3, 101}, {1, 201}, {3, 102}, {2, 301}, {3, 103}, {1, 202}, {2, 302}
        };

        codevault::dsa::mergeSort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        bool keyOrder = (pairs[0].first == 1 && pairs[1].first == 1 &&
                         pairs[2].first == 2 && pairs[3].first == 2 &&
                         pairs[4].first == 3 && pairs[5].first == 3 && pairs[6].first == 3);

        bool stable1 = (pairs[0].second == 201 && pairs[1].second == 202);
        bool stable2 = (pairs[2].second == 301 && pairs[3].second == 302);
        bool stable3 = (pairs[4].second == 101 && pairs[5].second == 102 && pairs[6].second == 103);

        return keyOrder && stable1 && stable2 && stable3;
    });

    runTest("DSA: QuickSort - Ascending Order", []() {
        std::vector<int> data = {14, 3, 77, 23, 1, 98, 45, 12};
        codevault::dsa::quickSort(data.begin(), data.end(), std::less<int>());
        std::vector<int> expected = {1, 3, 12, 14, 23, 45, 77, 98};
        return data == expected;
    });

    runTest("DSA: QuickSort - Descending Order", []() {
        std::vector<int> data = {5, 12, 1, 99, 44, 3};
        codevault::dsa::quickSort(data.begin(), data.end(), std::greater<int>());
        std::vector<int> expected = {99, 44, 12, 5, 3, 1};
        return data == expected;
    });

    runTest("DSA: QuickSort - Duplicate Elements", []() {
        std::vector<int> data = {4, 2, 4, 1, 4, 3, 2, 1};
        codevault::dsa::quickSort(data.begin(), data.end(), std::less<int>());
        std::vector<int> expected = {1, 1, 2, 2, 3, 4, 4, 4};
        return data == expected;
    });

    runTest("DSA: QuickSort - Already Sorted and Reverse Sorted", []() {
        std::vector<int> sorted = {2, 4, 6, 8, 10, 12};
        codevault::dsa::quickSort(sorted.begin(), sorted.end(), std::less<int>());
        bool sortedOk = (sorted == std::vector<int>{2, 4, 6, 8, 10, 12});

        std::vector<int> reversed = {12, 10, 8, 6, 4, 2};
        codevault::dsa::quickSort(reversed.begin(), reversed.end(), std::less<int>());
        bool reversedOk = (reversed == std::vector<int>{2, 4, 6, 8, 10, 12});

        return sortedOk && reversedOk;
    });

    runTest("DSA: QuickSort - Empty Range and Single Element", []() {
        std::vector<std::string> emptyVec;
        codevault::dsa::quickSort(emptyVec.begin(), emptyVec.end(), std::less<std::string>());
        bool emptyOk = emptyVec.empty();

        std::vector<std::string> single = {"isolated"};
        codevault::dsa::quickSort(single.begin(), single.end(), std::less<std::string>());
        bool singleOk = (single.size() == 1 && single[0] == "isolated");

        return emptyOk && singleOk;
    });

    runTest("DSA: QuickSort - In-Place Partition on Mixed Collection", []() {
        std::vector<int> data = {100, -5, 25, 0, -20, 80, 50, 25, -5};
        codevault::dsa::quickSort(data.begin(), data.end(), std::less<int>());
        std::vector<int> expected = {-20, -5, -5, 0, 25, 25, 50, 80, 100};
        return data == expected;
    });

    // =========================================================================
    // 9. SearchService PrefixTrie Lookup & Keyword Search
    // =========================================================================
    std::cout << "\n--- 9. SearchService PrefixTrie Lookup & Keyword Search ---\n";

    auto makeTestQuestion = [](std::string id, std::string title, codevault::models::Topic topic,
                               codevault::models::Difficulty diff, std::string company,
                               codevault::models::Status status, bool fav, int32_t priority = 2,
                               std::vector<std::string> tags = {}, std::string desc = "") {
        return codevault::models::Question(
            id, title, desc, topic, diff, company,
            codevault::models::Platform::LeetCode, "", status, fav, "",
            1000, 2000, 0, 3000, priority, tags, "test_user"
        );
    };

    runTest("Search: Exact and Prefix Title Matching via PrefixTrie", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Binary Search", codevault::models::Topic::BinarySearch,
                                   codevault::models::Difficulty::Easy, "Google", codevault::models::Status::Solved, true);
        auto q2 = makeTestQuestion("Q-2", "Binary Tree Level Order", codevault::models::Topic::Trees,
                                   codevault::models::Difficulty::Medium, "Amazon", codevault::models::Status::InProgress, false);
        auto q3 = makeTestQuestion("Q-3", "LRU Cache", codevault::models::Topic::LinkedLists,
                                   codevault::models::Difficulty::Medium, "Meta", codevault::models::Status::Solved, true);

        std::vector<codevault::models::Question> catalog = {q1, q2, q3};
        search.rebuildIndex(catalog);

        auto matches = search.searchByTitlePrefix(catalog, "Binary");
        bool countOk = matches.size() == 2;
        bool hasQ1 = false, hasQ2 = false;
        for (const auto& m : matches) {
            if (m.getId() == "Q-1") hasQ1 = true;
            if (m.getId() == "Q-2") hasQ2 = true;
        }

        return countOk && hasQ1 && hasQ2;
    });

    runTest("Search: Case-Insensitive Prefix Autocomplete", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Merge Intervals", codevault::models::Topic::Arrays,
                                   codevault::models::Difficulty::Medium, "Google", codevault::models::Status::Solved, false);
        search.indexQuestion(q1);

        auto suggUpper = search.searchTitlesByPrefix("MERGE");
        auto suggLower = search.searchTitlesByPrefix("merge");
        auto suggPartial = search.searchTitlesByPrefix("mer");

        return suggUpper.size() == 1 && suggLower.size() == 1 && suggPartial.size() == 1 &&
               suggUpper[0] == "Merge Intervals";
    });

    runTest("Search: Non-Existent Prefix Yields Empty Result", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Graph Valid Tree", codevault::models::Topic::Graphs,
                                   codevault::models::Difficulty::Medium, "Meta", codevault::models::Status::Unsolved, false);
        std::vector<codevault::models::Question> catalog = {q1};
        search.rebuildIndex(catalog);

        auto matches = search.searchByTitlePrefix(catalog, "Zylophone");
        return matches.empty();
    });

    runTest("Search: Empty Search String Returns Full Collection", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Climbing Stairs", codevault::models::Topic::DynamicProgramming,
                                   codevault::models::Difficulty::Easy, "Amazon", codevault::models::Status::Solved, true);
        auto q2 = makeTestQuestion("Q-2", "Coin Change", codevault::models::Topic::DynamicProgramming,
                                   codevault::models::Difficulty::Medium, "Amazon", codevault::models::Status::InProgress, false);
        std::vector<codevault::models::Question> catalog = {q1, q2};
        search.rebuildIndex(catalog);

        auto matches = search.searchByTitlePrefix(catalog, "   ");
        return matches.size() == 2;
    });

    runTest("Search: Deleted Question Is Removed from Prefix Search", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Word Break", codevault::models::Topic::DynamicProgramming,
                                   codevault::models::Difficulty::Medium, "Amazon", codevault::models::Status::Solved, false);
        search.indexQuestion(q1);
        bool before = search.containsTitle("Word Break");

        search.removeQuestion(q1);
        bool after = search.containsTitle("Word Break");
        auto sugg = search.searchTitlesByPrefix("Word");

        return before && !after && sugg.empty();
    });

    runTest("Search: Edited Title Updates Trie Results Cleanly", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Old Title Algorithm", codevault::models::Topic::Arrays,
                                   codevault::models::Difficulty::Easy, "Google", codevault::models::Status::Unsolved, false);
        search.indexQuestion(q1);

        auto updatedQ1 = q1;
        updatedQ1.setTitle("New Refactored Algorithm");
        search.updateQuestion("Old Title Algorithm", updatedQ1);

        bool oldGone = !search.containsTitle("Old Title Algorithm");
        bool newFound = search.containsTitle("New Refactored Algorithm");
        auto suggNew = search.searchTitlesByPrefix("New Ref");

        return oldGone && newFound && suggNew.size() == 1;
    });

    runTest("Keyword Search: Matches Across Title and Description", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Median of Two Sorted Arrays", codevault::models::Topic::BinarySearch,
                                   codevault::models::Difficulty::Hard, "Google", codevault::models::Status::Unsolved, true, 1,
                                   {}, "Find median using binary search on partition");
        auto q2 = makeTestQuestion("Q-2", "Subarray Sum Equals K", codevault::models::Topic::Arrays,
                                   codevault::models::Difficulty::Medium, "Facebook", codevault::models::Status::Solved, false, 2,
                                   {}, "Use prefix sum and hash map for O(N)");
        std::vector<codevault::models::Question> catalog = {q1, q2};

        auto matchTitle = search.searchByKeyword(catalog, "Median");
        auto matchDesc = search.searchByKeyword(catalog, "prefix sum");

        return matchTitle.size() == 1 && matchTitle[0].getId() == "Q-1" &&
               matchDesc.size() == 1 && matchDesc[0].getId() == "Q-2";
    });

    runTest("Keyword Search: Matches Tags and Company Case-Insensitively", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Number of Islands", codevault::models::Topic::Graphs,
                                   codevault::models::Difficulty::Medium, "Bloomberg", codevault::models::Status::Solved, true, 1,
                                   {"bfs", "dfs", "matrix"}, "Count connected components");
        std::vector<codevault::models::Question> catalog = {q1};

        auto matchTag = search.searchByKeyword(catalog, "matrix");
        auto matchComp = search.searchByKeyword(catalog, "bloomberg");
        auto matchNone = search.searchByKeyword(catalog, "cryptocurrency");

        return matchTag.size() == 1 && matchComp.size() == 1 && matchNone.empty();
    });

    // =========================================================================
    // 10. Multi-Criteria Filtering
    // =========================================================================
    std::cout << "\n--- 10. Multi-Criteria Filtering ---\n";

    runTest("Filter: Topic Only Filter", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "A", codevault::models::Topic::Trees, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Solved, false);
        auto q2 = makeTestQuestion("Q-2", "B", codevault::models::Topic::Graphs, codevault::models::Difficulty::Medium, "M", codevault::models::Status::Unsolved, false);
        auto q3 = makeTestQuestion("Q-3", "C", codevault::models::Topic::Trees, codevault::models::Difficulty::Hard, "A", codevault::models::Status::Solved, true);

        auto filtered = search.filterByTopic({q1, q2, q3}, codevault::models::Topic::Trees);
        return filtered.size() == 2 && filtered[0].getId() == "Q-1" && filtered[1].getId() == "Q-3";
    });

    runTest("Filter: Difficulty Only Filter", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "A", codevault::models::Topic::Arrays, codevault::models::Difficulty::Hard, "G", codevault::models::Status::Solved, false);
        auto q2 = makeTestQuestion("Q-2", "B", codevault::models::Topic::Graphs, codevault::models::Difficulty::Hard, "M", codevault::models::Status::Unsolved, false);
        auto q3 = makeTestQuestion("Q-3", "C", codevault::models::Topic::Trees, codevault::models::Difficulty::Easy, "A", codevault::models::Status::Solved, true);

        auto filtered = search.filterByDifficulty({q1, q2, q3}, codevault::models::Difficulty::Hard);
        return filtered.size() == 2;
    });

    runTest("Filter: Status Only Filter", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "A", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Mastered, false);
        auto q2 = makeTestQuestion("Q-2", "B", codevault::models::Topic::Graphs, codevault::models::Difficulty::Medium, "M", codevault::models::Status::Unsolved, false);

        auto filtered = search.filterByStatus({q1, q2}, codevault::models::Status::Mastered);
        return filtered.size() == 1 && filtered[0].getId() == "Q-1";
    });

    runTest("Filter: Company Substring Matching Case-Insensitively", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "A", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Google LLC", codevault::models::Status::Solved, false);
        auto q2 = makeTestQuestion("Q-2", "B", codevault::models::Topic::Graphs, codevault::models::Difficulty::Medium, "Microsoft", codevault::models::Status::Unsolved, false);

        auto filtered = search.filterByCompany({q1, q2}, "google");
        return filtered.size() == 1 && filtered[0].getId() == "Q-1";
    });

    runTest("Filter: Favorites Only and Non-Favorites Only", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "A", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Solved, true);
        auto q2 = makeTestQuestion("Q-2", "B", codevault::models::Topic::Graphs, codevault::models::Difficulty::Medium, "M", codevault::models::Status::Unsolved, false);

        auto favs = search.filterFavorites({q1, q2}, true);
        auto nonFavs = search.filterFavorites({q1, q2}, false);

        return favs.size() == 1 && favs[0].getId() == "Q-1" &&
               nonFavs.size() == 1 && nonFavs[0].getId() == "Q-2";
    });

    runTest("Filter: Multiple Simultaneous Criteria (Topic + Difficulty + Status)", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Alien Dictionary", codevault::models::Topic::Graphs,
                                   codevault::models::Difficulty::Hard, "Meta", codevault::models::Status::Unsolved, false);
        auto q2 = makeTestQuestion("Q-2", "Network Delay Time", codevault::models::Topic::Graphs,
                                   codevault::models::Difficulty::Medium, "Google", codevault::models::Status::Solved, true);
        auto q3 = makeTestQuestion("Q-3", "Critical Connections", codevault::models::Topic::Graphs,
                                   codevault::models::Difficulty::Hard, "Amazon", codevault::models::Status::Solved, true);

        codevault::models::QuestionFilter filter;
        filter.topic = codevault::models::Topic::Graphs;
        filter.difficulty = codevault::models::Difficulty::Hard;
        filter.status = codevault::models::Status::Unsolved;

        auto results = search.filter({q1, q2, q3}, filter);
        return results.size() == 1 && results[0].getId() == "Q-1";
    });

    runTest("Filter: Non-Matching Multi-Filter Produces Empty Result", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Course Schedule", codevault::models::Topic::Graphs,
                                   codevault::models::Difficulty::Medium, "Amazon", codevault::models::Status::Solved, false);

        codevault::models::QuestionFilter filter;
        filter.topic = codevault::models::Topic::DynamicProgramming;
        filter.difficulty = codevault::models::Difficulty::Hard;

        auto results = search.filter({q1}, filter);
        return results.empty();
    });

    runTest("Filter: Empty Filter Criteria Returns Full Collection", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "A", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Solved, false);
        auto q2 = makeTestQuestion("Q-2", "B", codevault::models::Topic::Trees, codevault::models::Difficulty::Hard, "M", codevault::models::Status::Unsolved, true);

        codevault::models::QuestionFilter emptyFilter;
        auto results = search.filter({q1, q2}, emptyFilter);
        return results.size() == 2;
    });

    // =========================================================================
    // 11. Question Sorting & Tie-Breaking
    // =========================================================================
    std::cout << "\n--- 11. Question Sorting & Tie-Breaking ---\n";

    runTest("Sort: Questions by Title Ascending and Descending", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "ZigZag Conversion", codevault::models::Topic::Strings, codevault::models::Difficulty::Medium, "PayPal", codevault::models::Status::Solved, false);
        auto q2 = makeTestQuestion("Q-2", "Add Two Numbers", codevault::models::Topic::LinkedLists, codevault::models::Difficulty::Medium, "Amazon", codevault::models::Status::Solved, false);
        auto q3 = makeTestQuestion("Q-3", "Median of Arrays", codevault::models::Topic::Arrays, codevault::models::Difficulty::Hard, "Google", codevault::models::Status::Unsolved, true);

        std::vector<codevault::models::Question> listAsc = {q1, q2, q3};
        search.sort(listAsc, {codevault::models::SortField::Title, codevault::models::SortDirection::Ascending});
        bool ascOk = (listAsc[0].getTitle() == "Add Two Numbers" &&
                      listAsc[1].getTitle() == "Median of Arrays" &&
                      listAsc[2].getTitle() == "ZigZag Conversion");

        std::vector<codevault::models::Question> listDesc = {q1, q2, q3};
        search.sort(listDesc, {codevault::models::SortField::Title, codevault::models::SortDirection::Descending});
        bool descOk = (listDesc[0].getTitle() == "ZigZag Conversion" &&
                       listDesc[1].getTitle() == "Median of Arrays" &&
                       listDesc[2].getTitle() == "Add Two Numbers");

        return ascOk && descOk;
    });

    runTest("Sort: Questions by Difficulty (Easy -> Medium -> Hard)", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto qHard = makeTestQuestion("Q-1", "Trapping Rain Water", codevault::models::Topic::Arrays, codevault::models::Difficulty::Hard, "Google", codevault::models::Status::Solved, false);
        auto qEasy = makeTestQuestion("Q-2", "Valid Parentheses", codevault::models::Topic::StacksQueues, codevault::models::Difficulty::Easy, "Amazon", codevault::models::Status::Solved, false);
        auto qMed = makeTestQuestion("Q-3", "3Sum", codevault::models::Topic::Arrays, codevault::models::Difficulty::Medium, "Meta", codevault::models::Status::Unsolved, false);

        std::vector<codevault::models::Question> list = {qHard, qEasy, qMed};
        search.sort(list, {codevault::models::SortField::Difficulty, codevault::models::SortDirection::Ascending});

        return list[0].getDifficulty() == codevault::models::Difficulty::Easy &&
               list[1].getDifficulty() == codevault::models::Difficulty::Medium &&
               list[2].getDifficulty() == codevault::models::Difficulty::Hard;
    });

    runTest("Sort: Questions by Difficulty Descending (Hard -> Medium -> Easy)", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto qHard = makeTestQuestion("Q-1", "Trapping Rain Water", codevault::models::Topic::Arrays, codevault::models::Difficulty::Hard, "Google", codevault::models::Status::Solved, false);
        auto qEasy = makeTestQuestion("Q-2", "Valid Parentheses", codevault::models::Topic::StacksQueues, codevault::models::Difficulty::Easy, "Amazon", codevault::models::Status::Solved, false);
        auto qMed = makeTestQuestion("Q-3", "3Sum", codevault::models::Topic::Arrays, codevault::models::Difficulty::Medium, "Meta", codevault::models::Status::Unsolved, false);

        std::vector<codevault::models::Question> list = {qHard, qEasy, qMed};
        search.sort(list, {codevault::models::SortField::Difficulty, codevault::models::SortDirection::Descending});

        return list[0].getDifficulty() == codevault::models::Difficulty::Hard &&
               list[1].getDifficulty() == codevault::models::Difficulty::Medium &&
               list[2].getDifficulty() == codevault::models::Difficulty::Easy;
    });

    runTest("Sort: Questions by Topic Alphabetical", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "T", codevault::models::Topic::Trees, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Solved, false);
        auto q2 = makeTestQuestion("Q-2", "A", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Solved, false);
        auto q3 = makeTestQuestion("Q-3", "G", codevault::models::Topic::Graphs, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Solved, false);

        std::vector<codevault::models::Question> list = {q1, q2, q3};
        search.sort(list, {codevault::models::SortField::Topic, codevault::models::SortDirection::Ascending});

        return list[0].getTopic() == codevault::models::Topic::Arrays &&
               list[1].getTopic() == codevault::models::Topic::Graphs &&
               list[2].getTopic() == codevault::models::Topic::Trees;
    });

    runTest("Sort: Questions by Company Alphabetical", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "A", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Microsoft", codevault::models::Status::Solved, false);
        auto q2 = makeTestQuestion("Q-2", "B", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Apple", codevault::models::Status::Solved, false);
        auto q3 = makeTestQuestion("Q-3", "C", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Google", codevault::models::Status::Solved, false);

        std::vector<codevault::models::Question> list = {q1, q2, q3};
        search.sort(list, {codevault::models::SortField::Company, codevault::models::SortDirection::Ascending});

        return list[0].getCompany() == "Apple" &&
               list[1].getCompany() == "Google" &&
               list[2].getCompany() == "Microsoft";
    });

    runTest("Sort: Questions by Status Workflow Progression", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "A", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Mastered, false);
        auto q2 = makeTestQuestion("Q-2", "B", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Unsolved, false);
        auto q3 = makeTestQuestion("Q-3", "C", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::InProgress, false);
        auto q4 = makeTestQuestion("Q-4", "D", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Solved, false);

        std::vector<codevault::models::Question> list = {q1, q2, q3, q4};
        search.sort(list, {codevault::models::SortField::Status, codevault::models::SortDirection::Ascending});

        return list[0].getStatus() == codevault::models::Status::Unsolved &&
               list[1].getStatus() == codevault::models::Status::InProgress &&
               list[2].getStatus() == codevault::models::Status::Solved &&
               list[3].getStatus() == codevault::models::Status::Mastered;
    });

    runTest("Sort: Deterministic Secondary Tie-Breaking by ID", [makeTestQuestion]() {
        codevault::services::SearchService search;
        // Two questions with the identical Title, different IDs
        auto qB = makeTestQuestion("Q-200", "Identical Problem", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Solved, false);
        auto qA = makeTestQuestion("Q-100", "Identical Problem", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "G", codevault::models::Status::Solved, false);

        std::vector<codevault::models::Question> list = {qB, qA};
        search.sort(list, {codevault::models::SortField::Title, codevault::models::SortDirection::Ascending});

        // Even though titles match, secondary ID tie-breaker guarantees Q-100 precedes Q-200
        return list[0].getId() == "Q-100" && list[1].getId() == "Q-200";
    });

    runTest("Sort: Algorithm Consistency (MergeSort vs QuickSort Produce Identical Order)", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-3", "LRU Cache", codevault::models::Topic::LinkedLists, codevault::models::Difficulty::Medium, "Meta", codevault::models::Status::Solved, true);
        auto q2 = makeTestQuestion("Q-1", "Binary Search", codevault::models::Topic::BinarySearch, codevault::models::Difficulty::Easy, "Google", codevault::models::Status::Solved, true);
        auto q3 = makeTestQuestion("Q-2", "Binary Tree Level Order", codevault::models::Topic::Trees, codevault::models::Difficulty::Medium, "Amazon", codevault::models::Status::InProgress, false);

        std::vector<codevault::models::Question> listMerge = {q1, q2, q3};
        std::vector<codevault::models::Question> listQuick = {q1, q2, q3};

        search.sort(listMerge, {codevault::models::SortField::Title, codevault::models::SortDirection::Ascending, codevault::models::SortAlgorithm::MergeSort});
        search.sort(listQuick, {codevault::models::SortField::Title, codevault::models::SortDirection::Ascending, codevault::models::SortAlgorithm::QuickSort});

        bool match = (listMerge.size() == listQuick.size());
        for (size_t i = 0; i < listMerge.size() && match; ++i) {
            if (listMerge[i].getId() != listQuick[i].getId()) match = false;
        }
        return match;
    });

    // =========================================================================
    // 12. Combined Workflows & Repository Immutability
    // =========================================================================
    std::cout << "\n--- 12. Combined Workflows & Repository Immutability ---\n";

    runTest("Combined: Search Keyword -> Filter Topic -> Sort Difficulty", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Binary Search Tree Validation", codevault::models::Topic::Trees,
                                   codevault::models::Difficulty::Hard, "Meta", codevault::models::Status::Solved, false, 1,
                                   {"binary", "tree", "bst"}, "Validate recursive BST invariants");
        auto q2 = makeTestQuestion("Q-2", "Binary Search on Array", codevault::models::Topic::BinarySearch,
                                   codevault::models::Difficulty::Easy, "Google", codevault::models::Status::Solved, true, 2,
                                   {"binary", "search"}, "Standard iterative binary search");
        auto q3 = makeTestQuestion("Q-3", "Invert Binary Tree", codevault::models::Topic::Trees,
                                   codevault::models::Difficulty::Easy, "Google", codevault::models::Status::Solved, true, 3,
                                   {"binary", "tree"}, "Swap left and right children recursively");
        auto q4 = makeTestQuestion("Q-4", "Lowest Common Ancestor in Binary Tree", codevault::models::Topic::Trees,
                                   codevault::models::Difficulty::Medium, "Amazon", codevault::models::Status::InProgress, false, 2,
                                   {"binary", "tree"}, "Find ancestor in tree");

        std::vector<codevault::models::Question> catalog = {q1, q2, q3, q4};

        codevault::models::QuestionFilter filter;
        filter.keyword = "Binary";
        filter.topic = codevault::models::Topic::Trees;

        codevault::models::SortOptions sortOpts{codevault::models::SortField::Difficulty, codevault::models::SortDirection::Ascending};

        auto results = search.searchAndFilter(catalog, filter, sortOpts);

        // All 3 Tree questions have "Binary". Sorted by Difficulty Easy -> Medium -> Hard
        return results.size() == 3 &&
               results[0].getId() == "Q-3" && // Easy
               results[1].getId() == "Q-4" && // Medium
               results[2].getId() == "Q-1";   // Hard
    });

    runTest("Combined: Filter Company & Difficulty -> Sort Title", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-1", "Zigzag Traversal", codevault::models::Topic::Trees, codevault::models::Difficulty::Medium, "Google", codevault::models::Status::Solved, false);
        auto q2 = makeTestQuestion("Q-2", "Clone Graph", codevault::models::Topic::Graphs, codevault::models::Difficulty::Medium, "Google", codevault::models::Status::Solved, false);
        auto q3 = makeTestQuestion("Q-3", "Two Sum", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Google", codevault::models::Status::Solved, false);

        codevault::models::QuestionFilter filter;
        filter.company = "Google";
        filter.difficulty = codevault::models::Difficulty::Medium;

        codevault::models::SortOptions sortOpts{codevault::models::SortField::Title, codevault::models::SortDirection::Ascending};

        auto results = search.searchAndFilter({q1, q2, q3}, filter, sortOpts);

        return results.size() == 2 &&
               results[0].getTitle() == "Clone Graph" &&
               results[1].getTitle() == "Zigzag Traversal";
    });

    runTest("Safety: View-Only Sorting and Filtering Leaves Original Collection Unmutated", [makeTestQuestion]() {
        codevault::services::SearchService search;
        auto q1 = makeTestQuestion("Q-10", "Zeta Problem", codevault::models::Topic::Strings, codevault::models::Difficulty::Hard, "Meta", codevault::models::Status::Unsolved, false);
        auto q2 = makeTestQuestion("Q-20", "Alpha Problem", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Amazon", codevault::models::Status::Solved, true);

        const std::vector<codevault::models::Question> originalCatalog = {q1, q2};

        // Filter and sort should produce new vector, never mutating originalCatalog
        codevault::models::QuestionFilter filter;
        filter.difficulty = codevault::models::Difficulty::Easy;
        codevault::models::SortOptions sortOpts{codevault::models::SortField::Title, codevault::models::SortDirection::Ascending};

        auto filteredAndSorted = search.searchAndFilter(originalCatalog, filter, sortOpts);

        bool originalIntact = (originalCatalog.size() == 2 &&
                               originalCatalog[0].getId() == "Q-10" &&
                               originalCatalog[1].getId() == "Q-20");
        bool resultCorrect = (filteredAndSorted.size() == 1 && filteredAndSorted[0].getId() == "Q-20");

        return originalIntact && resultCorrect;
    });

    // =========================================================================
    // 13. Stage 5: Spaced Revision Subsystem (Intervals, Progression, Verdicts)
    // =========================================================================
    std::cout << "\n--- 13. Stage 5: Spaced Revision Schedule & Progression ---\n";

    runTest("Revision: 1-Day Interval Calculation (Level 1)", []() {
        return codevault::services::getIntervalForLevel(1) == 86400 &&
               codevault::services::getIntervalForLevel(0) == 86400;
    });

    runTest("Revision: 3-Day Interval Calculation (Level 2)", []() {
        return codevault::services::getIntervalForLevel(2) == 3 * 86400;
    });

    runTest("Revision: 7-Day Interval Calculation (Level 3)", []() {
        return codevault::services::getIntervalForLevel(3) == 7 * 86400;
    });

    runTest("Revision: 14-Day Interval Calculation (Level 4)", []() {
        return codevault::services::getIntervalForLevel(4) == 14 * 86400;
    });

    runTest("Revision: 30-Day Interval Calculation (Level 5+)", []() {
        return codevault::services::getIntervalForLevel(5) == 30 * 86400 &&
               codevault::services::getIntervalForLevel(6) == 30 * 86400 &&
               codevault::services::getIntervalForLevel(10) == 30 * 86400;
    });

    runTest("Revision: Unpracticed Question Initial Progression on Solved", []() {
        auto mockClock = std::make_shared<codevault::utils::MockClock>(1000000);
        codevault::services::RevisionService revService(mockClock);

        codevault::models::Question q("Q-REV1", "Initial Problem", "", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy, "Google", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::Unsolved, false, "", 1000000, 1000000, 0, 0, 2, {});

        auto sched = revService.calculateNextSchedule(q, codevault::models::PracticeVerdict::Solved, 1000000);

        bool levelCorrect = (sched.nextLevel == 1);
        bool intervalCorrect = (sched.intervalSeconds == 86400);
        bool nextDateCorrect = (sched.nextRevisionAt == 1000000 + 86400);
        bool lastPracticedCorrect = (sched.lastPracticedAt == 1000000);
        bool statusCorrect = (sched.newStatus == codevault::models::Status::Solved);

        return levelCorrect && intervalCorrect && nextDateCorrect && lastPracticedCorrect && statusCorrect;
    });

    runTest("Revision: Multi-Step Solved Progression Across Intervals (1 -> 3 -> 7 -> 14 -> 30 days)", []() {
        auto mockClock = std::make_shared<codevault::utils::MockClock>(1000000);
        codevault::services::RevisionService revService(mockClock);

        codevault::models::Question q("Q-REV2", "Progression Problem", "", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Medium, "Amazon", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::Unsolved, false, "", 1000000, 1000000, 0, 0, 2, {});

        // Step 1: Unpracticed -> Solved (Level 1: 1 day)
        revService.markRevisionResult(q, codevault::models::PracticeVerdict::Solved, mockClock->now());
        bool step1 = (q.getNextRevisionAt() == 1000000 + 86400) && (q.getStatus() == codevault::models::Status::Solved);

        // Step 2: Level 1 -> Solved (Level 2: 3 days)
        mockClock->advanceDays(1);
        revService.markRevisionResult(q, codevault::models::PracticeVerdict::Solved, mockClock->now());
        bool step2 = (q.getNextRevisionAt() == mockClock->now() + 3 * 86400);

        // Step 3: Level 2 -> Solved (Level 3: 7 days)
        mockClock->advanceDays(3);
        revService.markRevisionResult(q, codevault::models::PracticeVerdict::Solved, mockClock->now());
        bool step3 = (q.getNextRevisionAt() == mockClock->now() + 7 * 86400);

        // Step 4: Level 3 -> Solved (Level 4: 14 days)
        mockClock->advanceDays(7);
        revService.markRevisionResult(q, codevault::models::PracticeVerdict::Solved, mockClock->now());
        bool step4 = (q.getNextRevisionAt() == mockClock->now() + 14 * 86400);

        // Step 5: Level 4 -> Solved (Level 5: 30 days)
        mockClock->advanceDays(14);
        revService.markRevisionResult(q, codevault::models::PracticeVerdict::Solved, mockClock->now());
        bool step5 = (q.getNextRevisionAt() == mockClock->now() + 30 * 86400) && (q.getStatus() == codevault::models::Status::Mastered);

        return step1 && step2 && step3 && step4 && step5;
    });

    runTest("Revision: Level 5 Caps Interval at 30 Days and Preserves Mastered Status", []() {
        auto mockClock = std::make_shared<codevault::utils::MockClock>(2000000);
        codevault::services::RevisionService revService(mockClock);

        // A question already at level 5 (last practiced at 1000000, next revision at 1000000 + 30 days)
        codevault::models::Question q("Q-REV3", "Mastered Problem", "", codevault::models::Topic::DynamicProgramming,
            codevault::models::Difficulty::Hard, "Meta", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::Mastered, true, "", 1000000, 1000000, 1000000, 1000000 + 30 * 86400, 5, {});

        auto sched = revService.calculateNextSchedule(q, codevault::models::PracticeVerdict::Solved, mockClock->now());

        bool levelCapped = (sched.nextLevel == 5);
        bool intervalCapped = (sched.intervalSeconds == 30 * 86400);
        bool statusMastered = (sched.newStatus == codevault::models::Status::Mastered);
        bool priorityRelaxed = (sched.revisionPriority == 5);

        return levelCapped && intervalCapped && statusMastered && priorityRelaxed;
    });

    runTest("Revision: Needs Review Verdict Resets Interval to 1 Day and Sets Urgency to Priority 1", []() {
        auto mockClock = std::make_shared<codevault::utils::MockClock>(2000000);
        codevault::services::RevisionService revService(mockClock);

        // Question previously was mastered at level 5
        codevault::models::Question q("Q-REV4", "Tricky DP Problem", "", codevault::models::Topic::DynamicProgramming,
            codevault::models::Difficulty::Hard, "Apple", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::Mastered, true, "", 1000000, 1000000, 1000000, 1000000 + 30 * 86400, 5, {});

        auto sched = revService.calculateNextSchedule(q, codevault::models::PracticeVerdict::NeedsReview, mockClock->now());

        bool levelReset = (sched.nextLevel == 1);
        bool intervalReset = (sched.intervalSeconds == 86400); // 1 day
        bool dateCorrect = (sched.nextRevisionAt == mockClock->now() + 86400);
        bool priorityUrgent = (sched.revisionPriority == 1); // 1 = Highest urgency
        bool statusInProgress = (sched.newStatus == codevault::models::Status::InProgress);

        return levelReset && intervalReset && dateCorrect && priorityUrgent && statusInProgress;
    });

    runTest("Revision: Skipped Verdict Leaves State, Timestamps, and Status Intact", []() {
        auto mockClock = std::make_shared<codevault::utils::MockClock>(2000000);
        codevault::services::RevisionService revService(mockClock);

        codevault::models::Question q("Q-REV5", "Skipped Problem", "", codevault::models::Topic::Graphs,
            codevault::models::Difficulty::Medium, "Netflix", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::InProgress, false, "", 1000000, 1500000, 1200000, 1800000, 2, {});

        auto sched = revService.calculateNextSchedule(q, codevault::models::PracticeVerdict::Skipped, mockClock->now());

        bool lastPracticedPreserved = (sched.lastPracticedAt == 1200000);
        bool nextRevisionPreserved = (sched.nextRevisionAt == 1800000);
        bool priorityPreserved = (sched.revisionPriority == 2);
        bool statusPreserved = (sched.newStatus == codevault::models::Status::InProgress);

        return lastPracticedPreserved && nextRevisionPreserved && priorityPreserved && statusPreserved;
    });

    // =========================================================================
    // 14. Stage 5: Custom MinHeap Ordering, Tie-Breaking & Due Queries
    // =========================================================================
    std::cout << "\n--- 14. Stage 5: MinHeap Ordering, Tie-Breaking & Due Queries ---\n";

    runTest("MinHeap: Primary Ordering by Earliest nextRevisionAt", []() {
        codevault::services::RevisionService revService;
        revService.scheduleQuestion("Q-LATE", 50000, 2);
        revService.scheduleQuestion("Q-EARLY", 10000, 2);
        revService.scheduleQuestion("Q-MID", 30000, 2);

        auto first = revService.popNextDueRevision();
        auto second = revService.popNextDueRevision();
        auto third = revService.popNextDueRevision();

        return first.has_value() && first->questionId == "Q-EARLY" &&
               second.has_value() && second->questionId == "Q-MID" &&
               third.has_value() && third->questionId == "Q-LATE";
    });

    runTest("MinHeap: Secondary Tie-Breaking by Priority (1 Urgent before 5)", []() {
        codevault::services::RevisionService revService;
        // Same timestamp 10000, different priorities
        revService.scheduleQuestion("Q-LOW", 10000, 4);
        revService.scheduleQuestion("Q-URGENT", 10000, 1);
        revService.scheduleQuestion("Q-MED", 10000, 2);

        auto first = revService.popNextDueRevision();
        auto second = revService.popNextDueRevision();
        auto third = revService.popNextDueRevision();

        return first.has_value() && first->questionId == "Q-URGENT" &&
               second.has_value() && second->questionId == "Q-MED" &&
               third.has_value() && third->questionId == "Q-LOW";
    });

    runTest("MinHeap: Tertiary Deterministic Tie-Breaking by Question ID", []() {
        codevault::services::RevisionService revService;
        // Same timestamp and same priority: tie-break alphabetically by questionId
        revService.scheduleQuestion("Q-ZZZ", 20000, 2);
        revService.scheduleQuestion("Q-AAA", 20000, 2);
        revService.scheduleQuestion("Q-MMM", 20000, 2);

        auto first = revService.popNextDueRevision();
        auto second = revService.popNextDueRevision();
        auto third = revService.popNextDueRevision();

        return first.has_value() && first->questionId == "Q-AAA" &&
               second.has_value() && second->questionId == "Q-MMM" &&
               third.has_value() && third->questionId == "Q-ZZZ";
    });

    runTest("Due Logic: Questions with nextRevisionAt <= now are Detected as Due", []() {
        auto mockClock = std::make_shared<codevault::utils::MockClock>(1000);
        codevault::services::RevisionService revService(mockClock);

        revService.scheduleQuestion("Q-PAST", 500, 2);
        revService.scheduleQuestion("Q-NOW", 1000, 2);
        revService.scheduleQuestion("Q-FUTURE", 1500, 2);

        auto due = revService.getDueQuestions(mockClock->now());
        bool dueCount = (due.size() == 2);
        bool containsPast = (due.size() >= 2 && due[0].questionId == "Q-PAST");
        bool containsNow = (due.size() >= 2 && due[1].questionId == "Q-NOW");

        return dueCount && containsPast && containsNow;
    });

    runTest("Due Logic: Future Questions (nextRevisionAt > now) Excluded from Due List", []() {
        auto mockClock = std::make_shared<codevault::utils::MockClock>(1000);
        codevault::services::RevisionService revService(mockClock);

        revService.scheduleQuestion("Q-FUT1", 1001, 1);
        revService.scheduleQuestion("Q-FUT2", 2000, 1);

        auto due = revService.getDueQuestions(mockClock->now());
        size_t dueCount = revService.getDueCount(mockClock->now());

        return due.empty() && dueCount == 0 && !revService.hasDueRevisions(mockClock->now());
    });

    runTest("Due Logic: Unscheduled Questions (nextRevisionAt <= 0) Excluded From Revision Queue", []() {
        codevault::services::RevisionService revService;
        std::vector<codevault::models::Question> list = {
            codevault::models::Question("Q-SCHED", "Scheduled", "", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 5000, 2, {}),
            codevault::models::Question("Q-NONE", "Unscheduled", "", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom, "", codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {})
        };

        revService.loadFromQuestions(list);
        return revService.getTotalScheduledCount() == 1 && revService.getDueRevisionsCount() == 1;
    });

    runTest("Upcoming Revisions: Extracted in Ascending Chronological Order", []() {
        auto mockClock = std::make_shared<codevault::utils::MockClock>(1000);
        codevault::services::RevisionService revService(mockClock);

        revService.scheduleQuestion("Q-DUE", 500, 2);
        revService.scheduleQuestion("Q-UP2", 3000, 2);
        revService.scheduleQuestion("Q-UP1", 2000, 2);

        auto upcoming = revService.getUpcomingRevisions(10, mockClock->now());
        bool countCorrect = (upcoming.size() == 2);
        bool orderCorrect = (countCorrect && upcoming[0].questionId == "Q-UP1" && upcoming[1].questionId == "Q-UP2");

        return countCorrect && orderCorrect;
    });

    runTest("Scheduling & Rescheduling: rescheduleQuestion Replaces Existing Item Without Duplicates", []() {
        codevault::services::RevisionService revService;
        revService.scheduleQuestion("Q-100", 5000, 3);
        revService.scheduleQuestion("Q-200", 7000, 3);

        bool initialCount = (revService.getTotalScheduledCount() == 2);

        // Reschedule Q-100 with earlier timestamp and higher priority
        revService.rescheduleQuestion("Q-100", 3000, 1);

        bool afterCount = (revService.getTotalScheduledCount() == 2);
        auto top = revService.getNextDueRevision();
        bool topIsUpdated = (top.has_value() && top->questionId == "Q-100" && top->nextRevisionAt == 3000 && top->priority == 1);

        return initialCount && afterCount && topIsUpdated;
    });

    runTest("Removal: removeQuestion Removes Item from MinHeap Queue", []() {
        codevault::services::RevisionService revService;
        revService.scheduleQuestion("Q-DEL", 1000, 2);
        revService.scheduleQuestion("Q-KEEP", 2000, 2);

        revService.removeQuestion("Q-DEL");

        bool countCheck = (revService.getTotalScheduledCount() == 1);
        auto top = revService.getNextDueRevision();
        bool topIsKeep = (top.has_value() && top->questionId == "Q-KEEP");

        return countCheck && topIsKeep;
    });

    // =========================================================================
    // 15. Stage 5: Custom Practice Queue Mechanics (FIFO, Cycling, Progress)
    // =========================================================================
    std::cout << "\n--- 15. Stage 5: Practice Session Queue Mechanics ---\n";

    runTest("Practice Queue: Empty Session State Invariants", []() {
        codevault::services::PracticeService practice;
        bool emptyInitial = !practice.hasQuestions() && practice.getRemainingCount() == 0;
        bool currentEmpty = !practice.getCurrentQuestion().has_value();
        bool completeEmpty = !practice.completeCurrentQuestion().has_value();
        bool skippedOnEmpty = practice.skipCurrentQuestion();
        auto prog = practice.getProgress();

        return emptyInitial && currentEmpty && completeEmpty && !skippedOnEmpty && prog.total == 0 && prog.completed == 0;
    });

    runTest("Practice Queue: Preserves Strict FIFO Enqueue and Dequeue Order", []() {
        codevault::services::PracticeService practice;
        practice.startSession({"Q-ALPHA", "Q-BETA", "Q-GAMMA"});

        bool first = (practice.getCurrentQuestion() == "Q-ALPHA");
        auto c1 = practice.completeCurrentQuestion();
        bool second = (practice.getCurrentQuestion() == "Q-BETA");
        auto c2 = practice.completeCurrentQuestion();
        bool third = (practice.getCurrentQuestion() == "Q-GAMMA");
        auto c3 = practice.completeCurrentQuestion();
        bool done = !practice.hasQuestions();

        return first && c1 == "Q-ALPHA" && second && c2 == "Q-BETA" && third && c3 == "Q-GAMMA" && done;
    });

    runTest("Practice Queue: Skip Cycles Front Question to Back of FIFO Queue", []() {
        codevault::services::PracticeService practice;
        practice.startSession({"Q-1", "Q-2", "Q-3"});

        bool skipSuccess = practice.skipCurrentQuestion();
        bool newFront = (practice.getCurrentQuestion() == "Q-2");

        practice.completeCurrentQuestion(); // completed Q-2
        bool nextFront = (practice.getCurrentQuestion() == "Q-3");
        practice.completeCurrentQuestion(); // completed Q-3

        bool cycledFront = (practice.getCurrentQuestion() == "Q-1");
        practice.completeCurrentQuestion(); // completed Q-1

        return skipSuccess && newFront && nextFront && cycledFront && !practice.hasQuestions();
    });

    runTest("Practice Queue: Skip on Single Item Session Returns False", []() {
        codevault::services::PracticeService practice;
        practice.startSession({"Q-SOLO"});

        bool skipped = practice.skipCurrentQuestion();
        bool stillFront = (practice.getCurrentQuestion() == "Q-SOLO");

        return !skipped && stillFront;
    });

    runTest("Practice Queue: Session Progress Tracks Total, Completed, Remaining, and Skipped", []() {
        codevault::services::PracticeService practice;
        practice.startSession({"Q-A", "Q-B", "Q-C"});

        practice.completeCurrentQuestion(); // Complete Q-A
        practice.skipCurrentQuestion();     // Skip Q-B -> now Q-C is front

        auto progress = practice.getProgress();
        bool totalCheck = (progress.total == 3);
        bool completedCheck = (progress.completed == 1);
        bool remainingCheck = (progress.remaining == 2);
        bool skippedCheck = (progress.skipped == 1);

        return totalCheck && completedCheck && remainingCheck && skippedCheck;
    });

    runTest("Practice: Filtered Session Creation by Unsolved, Topic, Difficulty, and Favorites", []() {
        std::string testPath = "build/test_filtered_session.csv";
        std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto qService = std::make_shared<codevault::services::QuestionService>(repo);

        codevault::models::Question q1("Q-1", "Array Easy Solved", "", codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom, "", codevault::models::Status::Solved, true, "", 100, 100, 0, 0, 2, {});
        codevault::models::Question q2("Q-2", "Array Hard Unsolved", "", codevault::models::Topic::Arrays, codevault::models::Difficulty::Hard, "", codevault::models::Platform::Custom, "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {});
        codevault::models::Question q3("Q-3", "Tree Easy Unsolved", "", codevault::models::Topic::Trees, codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom, "", codevault::models::Status::Unsolved, true, "", 100, 100, 0, 0, 2, {});
        qService->createQuestion(q1);
        qService->createQuestion(q2);
        qService->createQuestion(q3);

        codevault::services::PracticeService practice;

        // Filter 1: All Unsolved
        std::vector<std::string> unsolvedIds;
        for (const auto& q : qService->getAllQuestions()) {
            if (q.getStatus() == codevault::models::Status::Unsolved) unsolvedIds.push_back(q.getId());
        }
        practice.startSession(unsolvedIds);
        bool unsolvedCheck = (practice.getRemainingCount() == 2);

        // Filter 2: Topic Arrays
        std::vector<std::string> arrayIds;
        for (const auto& q : qService->getAllQuestions()) {
            if (q.getTopic() == codevault::models::Topic::Arrays) arrayIds.push_back(q.getId());
        }
        practice.startSession(arrayIds);
        bool arrayCheck = (practice.getRemainingCount() == 2);

        // Filter 3: Difficulty Hard
        std::vector<std::string> hardIds;
        for (const auto& q : qService->getAllQuestions()) {
            if (q.getDifficulty() == codevault::models::Difficulty::Hard) hardIds.push_back(q.getId());
        }
        practice.startSession(hardIds);
        bool hardCheck = (practice.getRemainingCount() == 1 && practice.getCurrentQuestion() == "Q-2");

        // Filter 4: Favorites
        std::vector<std::string> favIds;
        for (const auto& q : qService->getAllQuestions()) {
            if (q.isFavorite()) favIds.push_back(q.getId());
        }
        practice.startSession(favIds);
        bool favCheck = (practice.getRemainingCount() == 2);

        std::filesystem::remove(testPath);
        return unsolvedCheck && arrayCheck && hardCheck && favCheck;
    });

    // =========================================================================
    // 16. Stage 5: Service Coordination & Verdict Processing
    // =========================================================================
    std::cout << "\n--- 16. Stage 5: Service Coordination & Verdict Processing ---\n";

    runTest("Coordination: recordPracticeAttempt(Solved) Updates Domain State & Advances Queue", []() {
        std::string testPath = "build/test_coord_solved.csv";
        std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        codevault::models::Question q("Q-C1", "Coord Solved", "Desc", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy, "Google", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {});
        repo->save(q);

        auto mockClock = std::make_shared<codevault::utils::MockClock>(100000);
        auto revService = std::make_shared<codevault::services::RevisionService>(mockClock);
        auto qService = std::make_shared<codevault::services::QuestionService>(repo, nullptr, revService);
        auto practice = std::make_shared<codevault::services::PracticeService>(qService, revService);

        practice->startSession({"Q-C1"});
        auto result = practice->recordPracticeAttempt(codevault::models::PracticeVerdict::Solved, mockClock->now());

        bool resultValid = result.has_value() && result->nextLevel == 1 && result->intervalSeconds == 86400;
        bool sessionDone = !practice->hasQuestions();

        auto savedOpt = qService->getQuestionById("Q-C1");
        bool domainUpdated = savedOpt.has_value() &&
                             savedOpt->getStatus() == codevault::models::Status::Solved &&
                             savedOpt->getLastPracticedAt() == 100000 &&
                             savedOpt->getNextRevisionAt() == 100000 + 86400;

        std::filesystem::remove(testPath);
        return resultValid && sessionDone && domainUpdated;
    });

    runTest("Coordination: recordPracticeAttempt(NeedsReview) Sets 1-Day Interval & Urgency Priority 1", []() {
        std::string testPath = "build/test_coord_review.csv";
        std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        codevault::models::Question q("Q-C2", "Coord Review", "Desc", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Medium, "Amazon", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::Solved, false, "", 100, 100, 100, 100 + 14 * 86400, 4, {});
        repo->save(q);

        auto mockClock = std::make_shared<codevault::utils::MockClock>(200000);
        auto revService = std::make_shared<codevault::services::RevisionService>(mockClock);
        auto qService = std::make_shared<codevault::services::QuestionService>(repo, nullptr, revService);
        auto practice = std::make_shared<codevault::services::PracticeService>(qService, revService);

        practice->startSession({"Q-C2"});
        auto result = practice->recordPracticeAttempt(codevault::models::PracticeVerdict::NeedsReview, mockClock->now());

        bool resultValid = result.has_value() && result->nextLevel == 1 && result->revisionPriority == 1;
        bool sessionDone = !practice->hasQuestions();

        auto savedOpt = qService->getQuestionById("Q-C2");
        bool domainUpdated = savedOpt.has_value() &&
                             savedOpt->getStatus() == codevault::models::Status::InProgress &&
                             savedOpt->getLastPracticedAt() == 200000 &&
                             savedOpt->getNextRevisionAt() == 200000 + 86400 &&
                             savedOpt->getRevisionPriority() == 1;

        std::filesystem::remove(testPath);
        return resultValid && sessionDone && domainUpdated;
    });

    runTest("Coordination: recordPracticeAttempt(Skipped) Preserves Entity State & Cycles Queue", []() {
        std::string testPath = "build/test_coord_skip.csv";
        std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        codevault::models::Question q1("Q-S1", "Skip Q1", "Desc", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy, "Meta", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {});
        codevault::models::Question q2("Q-S2", "Skip Q2", "Desc", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy, "Meta", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {});
        repo->save(q1);
        repo->save(q2);

        auto mockClock = std::make_shared<codevault::utils::MockClock>(300000);
        auto revService = std::make_shared<codevault::services::RevisionService>(mockClock);
        auto qService = std::make_shared<codevault::services::QuestionService>(repo, nullptr, revService);
        auto practice = std::make_shared<codevault::services::PracticeService>(qService, revService);

        practice->startSession({"Q-S1", "Q-S2"});
        practice->recordPracticeAttempt(codevault::models::PracticeVerdict::Skipped, mockClock->now());

        // Queue should now have Q-S2 at front
        bool queueCycled = (practice->getCurrentQuestion() == "Q-S2");

        // Q-S1 state must not be marked solved or have timestamp changed
        auto savedOpt = qService->getQuestionById("Q-S1");
        bool entityPreserved = savedOpt.has_value() &&
                               savedOpt->getStatus() == codevault::models::Status::Unsolved &&
                               savedOpt->getLastPracticedAt() == 0 &&
                               savedOpt->getNextRevisionAt() == 0;

        std::filesystem::remove(testPath);
        return queueCycled && entityPreserved;
    });

    // =========================================================================
    // 17. Stage 5: Persistence Across Application Restarts
    // =========================================================================
    std::cout << "\n--- 17. Stage 5: Persistence Across Application Restarts ---\n";

    runTest("Persistence: Revision & Practice State Survives Complete Process Restart via CSV", []() {
        std::string testPath = "build/test_restart_persistence.csv";
        std::filesystem::remove(testPath);

        int64_t practiceTime = 1700000000;
        int64_t scheduledTime = practiceTime + 7 * 86400; // Level 3: 7 days

        // Process 1: Setup, practice question, persist to CSV, exit
        {
            auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
            auto revService = std::make_shared<codevault::services::RevisionService>();
            auto qService = std::make_shared<codevault::services::QuestionService>(repo, nullptr, revService);

            codevault::models::Question q("Q-PERSIST", "Survives Restart Problem", "Desc",
                codevault::models::Topic::DynamicProgramming, codevault::models::Difficulty::Medium,
                "Microsoft", codevault::models::Platform::LeetCode, "https://leetcode.com",
                codevault::models::Status::Solved, true, "Special notes for viva",
                100, 200, practiceTime, scheduledTime, 4, {"dp", "memo"});

            qService->createQuestion(q);
        }

        // Process 2: Restart application from cold storage file
        {
            auto newRepo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
            auto newRevService = std::make_shared<codevault::services::RevisionService>();
            auto newQService = std::make_shared<codevault::services::QuestionService>(newRepo, nullptr, newRevService);

            auto reloadedOpt = newQService->getQuestionById("Q-PERSIST");
            if (!reloadedOpt.has_value()) {
                std::filesystem::remove(testPath);
                return false;
            }

            const auto& q = reloadedOpt.value();
            bool titleMatches = (q.getTitle() == "Survives Restart Problem");
            bool statusMatches = (q.getStatus() == codevault::models::Status::Solved);
            bool lastPracticedMatches = (q.getLastPracticedAt() == practiceTime);
            bool nextRevisionMatches = (q.getNextRevisionAt() == scheduledTime);
            bool priorityMatches = (q.getRevisionPriority() == 4);
            bool favoriteMatches = q.isFavorite();

            // Check that RevisionService populated the MinHeap from reloaded questions
            bool revServicePopulated = (newRevService->getTotalScheduledCount() == 1);
            auto nextRev = newRevService->getNextDueRevision();
            bool heapMatches = nextRev.has_value() &&
                               nextRev->questionId == "Q-PERSIST" &&
                               nextRev->nextRevisionAt == scheduledTime &&
                               nextRev->priority == 4;

            std::filesystem::remove(testPath);
            return titleMatches && statusMatches && lastPracticedMatches &&
                   nextRevisionMatches && priorityMatches && favoriteMatches &&
                   revServicePopulated && heapMatches;
        }
    });

    runTest("Integration: Full End-to-End Workflow (Create -> Schedule -> Due -> Solved -> Persist -> Reload)", []() {
        std::string testPath = "build/test_e2e_workflow.csv";
        std::filesystem::remove(testPath);

        auto mockClock = std::make_shared<codevault::utils::MockClock>(100000);

        // 1. Create and schedule question
        {
            auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
            auto revService = std::make_shared<codevault::services::RevisionService>(mockClock);
            auto qService = std::make_shared<codevault::services::QuestionService>(repo, nullptr, revService);

            codevault::models::Question q("Q-E2E", "E2E Problem", "Test Description",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy,
                "Google", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Unsolved, false, "", mockClock->now(), mockClock->now(), 0, 0, 2, {});
            qService->createQuestion(q);

            // Initially not due because next_revision_at is 0
            bool initialDue = revService->hasDueRevisions(mockClock->now());
            if (initialDue) {
                std::filesystem::remove(testPath);
                return false;
            }

            // Start practice session with this unsolved problem
            auto practice = std::make_shared<codevault::services::PracticeService>(qService, revService);
            practice->startSession({"Q-E2E"});

            // Solve it at time 100,000
            practice->recordPracticeAttempt(codevault::models::PracticeVerdict::Solved, mockClock->now());
        }

        // 2. Advance time by 1 day (86,400s) -> now it must be DUE!
        mockClock->advanceDays(1);

        // 3. Restart and reload
        {
            auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
            auto revService = std::make_shared<codevault::services::RevisionService>(mockClock);
            auto qService = std::make_shared<codevault::services::QuestionService>(repo, nullptr, revService);

            // Verify it is now DUE
            auto dueQuestions = revService->getDueQuestions(mockClock->now());
            if (dueQuestions.empty() || dueQuestions[0].questionId != "Q-E2E") {
                std::filesystem::remove(testPath);
                return false;
            }

            // Practice due question again -> mark Needs Review
            auto practice = std::make_shared<codevault::services::PracticeService>(qService, revService);
            practice->startSession({dueQuestions[0].questionId});
            practice->recordPracticeAttempt(codevault::models::PracticeVerdict::NeedsReview, mockClock->now());

            // Check that it was rescheduled for 1 day later with Priority 1
            auto updatedOpt = qService->getQuestionById("Q-E2E");
            if (!updatedOpt.has_value()) {
                std::filesystem::remove(testPath);
                return false;
            }

            const auto& updated = updatedOpt.value();
            bool prioCorrect = (updated.getRevisionPriority() == 1);
            bool statusInProgress = (updated.getStatus() == codevault::models::Status::InProgress);
            bool nextRevScheduled = (updated.getNextRevisionAt() == mockClock->now() + 86400);

            std::filesystem::remove(testPath);
            return prioCorrect && statusInProgress && nextRevScheduled;
        }
    });

    // =========================================================================
    // 18. Stage 6: Statistics & Dashboard Metrics
    // =========================================================================
    std::cout << "\n--- 18. Stage 6: Statistics & Dashboard Metrics ---\n";

    // Test 153: Empty Dataset Zero-Division Protection & All Zero Invariants
    runTest("Stats: Empty Dataset Zero-Division Protection & All Zero Invariants", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> emptyQuestions;
        auto snapshot = statsService.computeDashboardSnapshot(emptyQuestions, 1000);

        bool overallSafe = (snapshot.overall.totalQuestions == 0 &&
                            snapshot.overall.completionPercentage == 0.0 &&
                            snapshot.overall.solvedPercentage == 0.0 &&
                            snapshot.overall.masteredPercentage == 0.0 &&
                            snapshot.overall.inProgressPercentage == 0.0 &&
                            snapshot.overall.unsolvedPercentage == 0.0 &&
                            snapshot.overall.favoritePercentage == 0.0 &&
                            snapshot.overall.dueForRevisionCount == 0);

        bool diffSafe = (snapshot.difficulty.totalQuestions == 0 &&
                         snapshot.difficulty.easyPercentage == 0.0 &&
                         snapshot.difficulty.mediumPercentage == 0.0 &&
                         snapshot.difficulty.hardPercentage == 0.0);

        bool topicSafe = (snapshot.topic.totalQuestions == 0 &&
                          snapshot.topic.distinctTopicsCount == 0);

        bool revSafe = (snapshot.revision.scheduledCount == 0 &&
                        snapshot.revision.dueCount == 0 &&
                        snapshot.revision.duePercentageOfScheduled == 0.0 &&
                        snapshot.revision.scheduledPercentageOfTotal == 0.0);

        bool pracSafe = (snapshot.practice.practicedCount == 0 &&
                         snapshot.practice.unpracticedCount == 0 &&
                         snapshot.practice.practicedPercentage == 0.0 &&
                         snapshot.practice.lastPracticedTimestamp == 0);

        return overallSafe && diffSafe && topicSafe && revSafe && pracSafe;
    });

    // Test 154: Single Question Dataset Accuracy
    runTest("Stats: Single Question Dataset Accuracy", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> single;
        single.emplace_back(
            "Q-S1", "Problem One", "Desc",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Google", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Solved,
            true, "Notes",
            100, 100, 200, 500, 3, std::vector<std::string>{"array"}
        );

        auto snapshot = statsService.computeDashboardSnapshot(single, 300);

        bool overallOk = (snapshot.overall.totalQuestions == 1 &&
                          snapshot.overall.solvedCount == 1 &&
                          snapshot.overall.completionPercentage == 100.0 &&
                          snapshot.overall.solvedPercentage == 100.0 &&
                          snapshot.overall.favoriteCount == 1 &&
                          snapshot.overall.favoritePercentage == 100.0 &&
                          snapshot.overall.upcomingRevisionsCount == 1 &&
                          snapshot.overall.dueForRevisionCount == 0);

        bool diffOk = (snapshot.difficulty.easyCount == 1 &&
                       snapshot.difficulty.easyPercentage == 100.0 &&
                       snapshot.difficulty.mediumCount == 0 &&
                       snapshot.difficulty.hardCount == 0);

        bool topicOk = (snapshot.topic.distinctTopicsCount == 1);

        return overallOk && diffOk && topicOk;
    });

    // Test 155: All Questions Solved Dataset
    runTest("Stats: All Questions Solved Dataset", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        for (int i = 1; i <= 5; ++i) {
            questions.emplace_back(
                "Q-" + std::to_string(i), "Solved Q", "Desc",
                codevault::models::Topic::Graphs,
                codevault::models::Difficulty::Medium,
                "Meta", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::Solved,
                false, "", 100, 100, 100, 0, 2, std::vector<std::string>{}
            );
        }

        auto overall = statsService.computeOverallStatistics(questions, 1000);
        return (overall.totalQuestions == 5 &&
                overall.solvedCount == 5 &&
                overall.completionPercentage == 100.0 &&
                overall.solvedPercentage == 100.0 &&
                overall.unsolvedPercentage == 0.0 &&
                overall.inProgressPercentage == 0.0);
    });

    // Test 156: All Questions Unsolved Dataset
    runTest("Stats: All Questions Unsolved Dataset", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        for (int i = 1; i <= 4; ++i) {
            questions.emplace_back(
                "Q-" + std::to_string(i), "Unsolved Q", "Desc",
                codevault::models::Topic::DynamicProgramming,
                codevault::models::Difficulty::Hard,
                "Amazon", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::Unsolved,
                false, "", 100, 100, 0, 0, 2, std::vector<std::string>{}
            );
        }

        auto overall = statsService.computeOverallStatistics(questions, 1000);
        return (overall.totalQuestions == 4 &&
                overall.unsolvedCount == 4 &&
                overall.completionPercentage == 0.0 &&
                overall.solvedPercentage == 0.0 &&
                overall.unsolvedPercentage == 100.0);
    });

    // Test 157: Mixed Status Distribution (Unsolved, InProgress, Solved, Mastered)
    runTest("Stats: Mixed Status Distribution (Unsolved, InProgress, Solved, Mastered)", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        // 1 Unsolved (10%), 2 InProgress (20%), 4 Solved (40%), 3 Mastered (30%) -> Total 10
        auto addWithStatus = [&](const std::string& id, codevault::models::Status st) {
            questions.emplace_back(
                id, "Title", "Desc", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode,
                "", st, false, "", 100, 100, 0, 0, 2, std::vector<std::string>{}
            );
        };

        addWithStatus("Q-1", codevault::models::Status::Unsolved);
        addWithStatus("Q-2", codevault::models::Status::InProgress);
        addWithStatus("Q-3", codevault::models::Status::InProgress);
        addWithStatus("Q-4", codevault::models::Status::Solved);
        addWithStatus("Q-5", codevault::models::Status::Solved);
        addWithStatus("Q-6", codevault::models::Status::Solved);
        addWithStatus("Q-7", codevault::models::Status::Solved);
        addWithStatus("Q-8", codevault::models::Status::Mastered);
        addWithStatus("Q-9", codevault::models::Status::Mastered);
        addWithStatus("Q-10", codevault::models::Status::Mastered);

        auto statusStats = statsService.computeStatusStatistics(questions);
        auto overall = statsService.computeOverallStatistics(questions, 1000);

        bool countsMatch = (statusStats.unsolvedCount == 1 &&
                            statusStats.inProgressCount == 2 &&
                            statusStats.solvedCount == 4 &&
                            statusStats.masteredCount == 3);

        bool percentagesMatch = (statusStats.unsolvedPercentage == 10.0 &&
                                 statusStats.inProgressPercentage == 20.0 &&
                                 statusStats.solvedPercentage == 40.0 &&
                                 statusStats.masteredPercentage == 30.0);

        // Completion rate must be (4 Solved + 3 Mastered) / 10 = 70.0%
        bool completionMatch = (overall.completionPercentage == 70.0);

        return countsMatch && percentagesMatch && completionMatch;
    });

    // Test 158: Difficulty Distribution (Easy, Medium, Hard Counts & Percentages)
    runTest("Stats: Difficulty Distribution (Easy, Medium, Hard Counts & Percentages)", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        // 2 Easy (25%), 4 Medium (50%), 2 Hard (25%) -> Total 8
        auto addWithDiff = [&](const std::string& id, codevault::models::Difficulty d) {
            questions.emplace_back(
                id, "Title", "Desc", codevault::models::Topic::Arrays,
                d, "", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, std::vector<std::string>{}
            );
        };

        addWithDiff("Q-1", codevault::models::Difficulty::Easy);
        addWithDiff("Q-2", codevault::models::Difficulty::Easy);
        addWithDiff("Q-3", codevault::models::Difficulty::Medium);
        addWithDiff("Q-4", codevault::models::Difficulty::Medium);
        addWithDiff("Q-5", codevault::models::Difficulty::Medium);
        addWithDiff("Q-6", codevault::models::Difficulty::Medium);
        addWithDiff("Q-7", codevault::models::Difficulty::Hard);
        addWithDiff("Q-8", codevault::models::Difficulty::Hard);

        auto diffStats = statsService.computeDifficultyStatistics(questions);
        return (diffStats.totalQuestions == 8 &&
                diffStats.easyCount == 2 && diffStats.easyPercentage == 25.0 &&
                diffStats.mediumCount == 4 && diffStats.mediumPercentage == 50.0 &&
                diffStats.hardCount == 2 && diffStats.hardPercentage == 25.0);
    });

    // Test 159: Topic Distribution Enumeration & Distinct Topics Count
    runTest("Stats: Topic Distribution Enumeration & Distinct Topics Count", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        // 3 Arrays, 2 Trees, 1 Graphs -> Total 6 questions across 3 distinct topics
        auto addWithTopic = [&](const std::string& id, codevault::models::Topic t) {
            questions.emplace_back(
                id, "Title", "Desc", t,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, std::vector<std::string>{}
            );
        };

        addWithTopic("Q-1", codevault::models::Topic::Arrays);
        addWithTopic("Q-2", codevault::models::Topic::Arrays);
        addWithTopic("Q-3", codevault::models::Topic::Arrays);
        addWithTopic("Q-4", codevault::models::Topic::Trees);
        addWithTopic("Q-5", codevault::models::Topic::Trees);
        addWithTopic("Q-6", codevault::models::Topic::Graphs);

        auto topicStats = statsService.computeTopicStatistics(questions, true);

        if (topicStats.totalQuestions != 6 || topicStats.distinctTopicsCount != 3) {
            return false;
        }

        // When sorted by count descending: index 0 should be Arrays (3), index 1 Trees (2), index 2 Graphs (1)
        if (topicStats.topicCounts[0].topic != codevault::models::Topic::Arrays ||
            topicStats.topicCounts[0].count != 3 ||
            topicStats.topicCounts[0].percentage != 50.0) {
            return false;
        }

        if (topicStats.topicCounts[1].topic != codevault::models::Topic::Trees ||
            topicStats.topicCounts[1].count != 2) {
            return false;
        }

        if (topicStats.topicCounts[2].topic != codevault::models::Topic::Graphs ||
            topicStats.topicCounts[2].count != 1) {
            return false;
        }

        return true;
    });

    // Test 160: Favorite Question Count & Percentage
    runTest("Stats: Favorite Question Count & Percentage", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        for (int i = 1; i <= 10; ++i) {
            bool isFav = (i <= 3); // 3 favorites out of 10
            questions.emplace_back(
                "Q-" + std::to_string(i), "Title", "Desc",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy,
                "", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::Unsolved, isFav, "", 100, 100, 0, 0, 2, std::vector<std::string>{}
            );
        }

        auto overall = statsService.computeOverallStatistics(questions, 1000);
        return (overall.favoriteCount == 3 && overall.favoritePercentage == 30.0);
    });

    // Test 161: Revision Due vs Upcoming Scheduling Logic
    runTest("Stats: Revision Due vs Upcoming Scheduling Logic", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        int64_t now = 1000;

        auto addWithSchedule = [&](const std::string& id, int64_t revAt) {
            questions.emplace_back(
                id, "Title", "Desc", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::InProgress, false, "", 100, 100, 100, revAt, 2, std::vector<std::string>{}
            );
        };

        addWithSchedule("Q-1", 900);  // Due
        addWithSchedule("Q-2", 1000); // Due (exact boundary)
        addWithSchedule("Q-3", 1100); // Upcoming
        addWithSchedule("Q-4", 1200); // Upcoming
        addWithSchedule("Q-5", 1300); // Upcoming
        addWithSchedule("Q-6", 0);    // Unscheduled

        auto revStats = statsService.computeRevisionStatistics(questions, now);

        bool countsMatch = (revStats.dueCount == 2 &&
                            revStats.upcomingCount == 3 &&
                            revStats.scheduledCount == 5 &&
                            revStats.unscheduledCount == 1);

        // Due percentage of scheduled = 2 / 5 * 100 = 40.0%
        bool duePctMatch = (revStats.duePercentageOfScheduled == 40.0);

        return countsMatch && duePctMatch;
    });

    // Test 162: Revision Priority Distribution (Tiers 1 to 5)
    runTest("Stats: Revision Priority Distribution (Tiers 1 to 5)", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;

        auto addWithPrio = [&](const std::string& id, int32_t prio) {
            questions.emplace_back(
                id, "Title", "Desc", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::InProgress, false, "", 100, 100, 100, 2000, prio, std::vector<std::string>{}
            );
        };

        addWithPrio("Q-1", 1);
        addWithPrio("Q-2", 1);
        addWithPrio("Q-3", 2);
        addWithPrio("Q-4", 4);
        addWithPrio("Q-5", 5);

        auto revStats = statsService.computeRevisionStatistics(questions, 1000);

        return (revStats.priorityCounts[1] == 2 &&
                revStats.priorityCounts[2] == 1 &&
                revStats.priorityCounts[3] == 0 &&
                revStats.priorityCounts[4] == 1 &&
                revStats.priorityCounts[5] == 1);
    });

    // Test 163: Revision Level Progression Distribution
    runTest("Stats: Revision Level Progression Distribution", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;

        auto addWithInterval = [&](const std::string& id, int64_t lastPracticed, int64_t nextRev) {
            questions.emplace_back(
                id, "Title", "Desc", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::InProgress, false, "", 100, 100, lastPracticed, nextRev, 2, std::vector<std::string>{}
            );
        };

        int64_t t = 1000000;
        addWithInterval("Q-1", t, t + 86400);               // Level 1 (1 day)
        addWithInterval("Q-2", t, t + 3 * 86400);           // Level 2 (3 days)
        addWithInterval("Q-3", t, t + 7 * 86400);           // Level 3 (7 days)
        addWithInterval("Q-4", t, t + 14 * 86400);          // Level 4 (14 days)
        addWithInterval("Q-5", t, t + 30 * 86400);          // Level 5 (30 days)

        auto revStats = statsService.computeRevisionStatistics(questions, t);

        return (revStats.levelCounts[1] == 1 &&
                revStats.levelCounts[2] == 1 &&
                revStats.levelCounts[3] == 1 &&
                revStats.levelCounts[4] == 1 &&
                revStats.levelCounts[5] == 1);
    });

    // Test 164: Practice Activity Metrics (Practiced vs Unpracticed & Latest Timestamp)
    runTest("Stats: Practice Activity Metrics (Practiced vs Unpracticed & Latest Timestamp)", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;

        auto addWithPractice = [&](const std::string& id, int64_t practicedAt) {
            questions.emplace_back(
                id, "Title", "Desc", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::InProgress, false, "", 100, 100, practicedAt, 0, 2, std::vector<std::string>{}
            );
        };

        addWithPractice("Q-1", 100);
        addWithPractice("Q-2", 300);
        addWithPractice("Q-3", 500); // Max timestamp
        addWithPractice("Q-4", 200);
        addWithPractice("Q-5", 0);   // Unpracticed
        addWithPractice("Q-6", 0);   // Unpracticed

        auto pracStats = statsService.computePracticeStatistics(questions);

        return (pracStats.practicedCount == 4 &&
                pracStats.unpracticedCount == 2 &&
                pracStats.lastPracticedTimestamp == 500 &&
                std::abs(pracStats.practicedPercentage - (4.0 / 6.0 * 100.0)) < 0.001);
    });

    // Test 165: Mastered Status Boundary Calculation
    runTest("Stats: Mastered Status Boundary Calculation", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        for (int i = 1; i <= 3; ++i) {
            questions.emplace_back(
                "Q-" + std::to_string(i), "Mastered Problem", "Desc",
                codevault::models::Topic::Heaps, codevault::models::Difficulty::Hard,
                "", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::Mastered, false, "", 100, 100, 500, 0, 5, std::vector<std::string>{}
            );
        }

        auto overall = statsService.computeOverallStatistics(questions, 1000);
        return (overall.masteredCount == 3 &&
                overall.solvedCount == 0 &&
                overall.masteredPercentage == 100.0 &&
                overall.completionPercentage == 100.0);
    });

    // Test 166: Safe Boundary Percentages & Non-Overflow Protection
    runTest("Stats: Safe Boundary Percentages & Non-Overflow Protection", [&]() {
        double p0 = codevault::services::StatisticsService::calculatePercentage(0, 0);
        double p1 = codevault::services::StatisticsService::calculatePercentage(5, 0);
        double p2 = codevault::services::StatisticsService::calculatePercentage(5, 10);
        double p3 = codevault::services::StatisticsService::calculatePercentage(10, 10);
        double p4 = codevault::services::StatisticsService::calculatePercentage(100, 100);

        return (p0 == 0.0 && p1 == 0.0 && p2 == 50.0 && p3 == 100.0 && p4 == 100.0);
    });

    // Test 167: Deterministic Repeated Calculations Invariance
    runTest("Stats: Deterministic Repeated Calculations Invariance", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        for (int i = 1; i <= 5; ++i) {
            questions.emplace_back(
                "Q-" + std::to_string(i), "Title " + std::to_string(i), "Desc",
                codevault::models::Topic::Trees, codevault::models::Difficulty::Medium,
                "Google", codevault::models::Platform::LeetCode,
                "", (i % 2 == 0) ? codevault::models::Status::Solved : codevault::models::Status::Unsolved,
                (i == 1), "", 100, 100, 0, 0, 2, std::vector<std::string>{}
            );
        }

        auto snap1 = statsService.computeDashboardSnapshot(questions, 1000);
        auto snap2 = statsService.computeDashboardSnapshot(questions, 1000);

        bool overallEq = (snap1.overall.totalQuestions == snap2.overall.totalQuestions &&
                          snap1.overall.solvedCount == snap2.overall.solvedCount &&
                          snap1.overall.completionPercentage == snap2.overall.completionPercentage);

        bool diffEq = (snap1.difficulty.mediumCount == snap2.difficulty.mediumCount &&
                       snap1.difficulty.mediumPercentage == snap2.difficulty.mediumPercentage);

        return overallEq && diffEq;
    });

    // Test 168: Safety: Statistics Calculations Guarantee Collection Immutability
    runTest("Safety: Statistics Calculations Guarantee Collection Immutability", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;
        questions.emplace_back(
            "Q-IMMUT", "Immutable Title", "Desc",
            codevault::models::Topic::Graphs, codevault::models::Difficulty::Hard,
            "Uber", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::InProgress, true, "Notes",
            100, 200, 300, 400, 1, std::vector<std::string>{"tag"}
        );

        auto copyBefore = questions;
        auto snapshot = statsService.computeDashboardSnapshot(questions, 350);

        if (questions.size() != copyBefore.size()) return false;
        const auto& q1 = questions[0];
        const auto& q2 = copyBefore[0];

        return (q1.getId() == q2.getId() &&
                q1.getTitle() == q2.getTitle() &&
                q1.getStatus() == q2.getStatus() &&
                q1.getLastPracticedAt() == q2.getLastPracticedAt() &&
                q1.getNextRevisionAt() == q2.getNextRevisionAt() &&
                q1.getRevisionPriority() == q2.getRevisionPriority() &&
                snapshot.overall.totalQuestions == 1);
    });

    // Test 169: Integration: StatisticsService with QuestionService and RevisionService Live Wiring
    runTest("Integration: StatisticsService with QuestionService and RevisionService Live Wiring", [&]() {
        std::string testPath = "data/test_stats_integration.csv";
        std::filesystem::remove(testPath);

        auto mockClock = std::make_shared<codevault::utils::MockClock>(1000);
        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto revService = std::make_shared<codevault::services::RevisionService>(mockClock);
        auto qService = std::make_shared<codevault::services::QuestionService>(repo, nullptr, revService);

        // Add 2 questions: 1 Solved (due), 1 Unsolved (unscheduled)
        codevault::models::Question q1("Q-S1", "P1", "D", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy, "G", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Solved, true, "", 100, 100, 500, 900, 2, {});
        codevault::models::Question q2("Q-S2", "P2", "D", codevault::models::Topic::Trees,
            codevault::models::Difficulty::Hard, "A", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {});

        qService->createQuestion(q1);
        qService->createQuestion(q2);

        codevault::services::StatisticsService statsService(qService, revService);
        auto snapshot = statsService.getDashboardSnapshot(mockClock->now());

        bool countsOk = (snapshot.overall.totalQuestions == 2 &&
                         snapshot.overall.solvedCount == 1 &&
                         snapshot.overall.unsolvedCount == 1 &&
                         snapshot.overall.completionPercentage == 50.0 &&
                         snapshot.overall.dueForRevisionCount == 1);

        std::filesystem::remove(testPath);
        return countsOk;
    });

    // Test 170: UI Helper: CliShell::renderProgressBar Text Formatting & Clamping
    runTest("UI Helper: CliShell::renderProgressBar Text Formatting & Clamping", [&]() {
        std::string bar0 = codevault::app::CliShell::renderProgressBar(0.0, 10);
        std::string bar50 = codevault::app::CliShell::renderProgressBar(50.0, 10);
        std::string bar100 = codevault::app::CliShell::renderProgressBar(100.0, 10);
        std::string barNeg = codevault::app::CliShell::renderProgressBar(-20.0, 10);
        std::string barOver = codevault::app::CliShell::renderProgressBar(120.0, 10);

        bool bar0Ok = (bar0 == "[----------]");
        bool bar50Ok = (bar50 == "[#####-----]");
        bool bar100Ok = (bar100 == "[##########]");
        bool barNegOk = (barNeg == "[----------]");
        bool barOverOk = (barOver == "[##########]");

        return bar0Ok && bar50Ok && bar100Ok && barNegOk && barOverOk;
    });

    // =========================================================================
    // 19. Stage 7: Hardening, Edge Cases & Robustness
    // =========================================================================
    std::cout << "\n--- 19. Stage 7: Hardening, Edge Cases & Robustness ---\n";

    // Test 171: Utils: datetime::formatTimestamp and formatDateOnly Boundaries
    runTest("Utils: datetime::formatTimestamp and formatDateOnly Boundaries", [&]() {
        bool zeroTs = (codevault::utils::formatTimestamp(0) == "Never");
        bool negTs = (codevault::utils::formatTimestamp(-500) == "Never");
        bool zeroDate = (codevault::utils::formatDateOnly(0) == "None");
        bool negDate = (codevault::utils::formatDateOnly(-1) == "None");

        std::string formattedTs = codevault::utils::formatTimestamp(1700000000);
        std::string formattedDate = codevault::utils::formatDateOnly(1700000000);

        bool tsValid = (formattedTs.size() >= 16 && formattedTs.find('-') != std::string::npos && formattedTs.find(':') != std::string::npos);
        bool dateValid = (formattedDate.size() == 10 && formattedDate.find('-') != std::string::npos);

        return zeroTs && negTs && zeroDate && negDate && tsValid && dateValid;
    });

    // Test 172: Persistence Hardening: Non-existent, Empty, and Header-Only CSV Files
    runTest("Persistence Hardening: Non-existent, Empty, and Header-Only CSV Files", [&]() {
        std::string missingPath = "data/test_missing_stage7.csv";
        std::filesystem::remove(missingPath);

        codevault::persistence::FileQuestionRepository repoMissing(missingPath);
        bool missingOk = (repoMissing.count() == 0 && repoMissing.findAll().empty() && !repoMissing.findById("Q-1").has_value());

        std::string emptyPath = "data/test_empty_stage7.csv";
        {
            std::ofstream ofs(emptyPath);
        }
        codevault::persistence::FileQuestionRepository repoEmpty(emptyPath);
        bool emptyOk = (repoEmpty.count() == 0 && repoEmpty.findAll().empty());
        std::filesystem::remove(emptyPath);

        std::string headerOnlyPath = "data/test_header_only_stage7.csv";
        {
            std::ofstream ofs(headerOnlyPath);
            ofs << "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\n";
        }
        codevault::persistence::FileQuestionRepository repoHeader(headerOnlyPath);
        bool headerOk = (repoHeader.count() == 0 && repoHeader.findAll().empty());
        std::filesystem::remove(headerOnlyPath);

        return missingOk && emptyOk && headerOk;
    });

    // Test 173: Persistence Hardening: Malformed Rows and Corrupt Data Fallback
    runTest("Persistence Hardening: Malformed Rows and Corrupt Data Fallback", [&]() {
        std::string corruptPath = "data/test_corrupt_stage7.csv";
        {
            std::ofstream ofs(corruptPath);
            ofs << "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\n";
            // Row with only 3 columns (too few)
            ofs << "Q-BAD-1,Incomplete Row,Desc\n";
            // Row with empty id
            ofs << ",No ID,Desc,Arrays,Easy,Comp,LeetCode,http://url,Solved,true,notes,100,100,0,0,2,tags,owner\n";
            // Row with non-numeric timestamps and priority
            ofs << "Q-CORRUPT,Corrupt Numbers,Desc,Arrays,Easy,Comp,LeetCode,http://url,Solved,false,notes,bad_ts,bad_ts,bad_ts,bad_ts,bad_prio,tags,owner\n";
            // Valid row
            ofs << "Q-GOOD,Good Problem,Desc,Trees,Medium,Comp,LeetCode,http://url,Solved,false,notes,100,100,0,0,2,tags,owner\n";
        }

        codevault::persistence::FileQuestionRepository repo(corruptPath);
        // Bad-1 (too few columns) is skipped
        // Bad-2 (empty ID) is skipped
        // Q-CORRUPT parses with safe defaults (ts=0, prio=2) and has ID and title, so valid
        // Q-GOOD is valid
        bool hasGood = repo.exists("Q-GOOD");
        bool noBad1 = !repo.exists("Q-BAD-1");
        bool countOk = (repo.count() >= 1 && hasGood && noBad1);

        std::filesystem::remove(corruptPath);
        return countOk;
    });

    // Test 174: Persistence Hardening: Quoted Fields with Commas and Quotes Round-Trip
    runTest("Persistence Hardening: Quoted Fields with Commas and Quotes Round-Trip", [&]() {
        std::string roundtripPath = "data/test_quotes_stage7.csv";
        std::filesystem::remove(roundtripPath);

        {
            codevault::persistence::FileQuestionRepository repo(roundtripPath);
            codevault::models::Question q(
                "Q-QUOTE-1",
                "Two Sum, \"Special Edition\"",
                "Find indices i, j such that nums[i] + nums[j] == target, where target > 0.",
                codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy,
                "Google, Inc.",
                codevault::models::Platform::LeetCode,
                "https://leetcode.com/problems/two-sum",
                codevault::models::Status::InProgress,
                true,
                "Important \"tricky\" edge cases, remember 0.",
                100, 200, 150, 250, 1,
                {"array;hash", "google,inc"}
            );
            repo.save(q);
        }

        // Reload fresh from disk
        codevault::persistence::FileQuestionRepository repoReload(roundtripPath);
        auto loadedOpt = repoReload.findById("Q-QUOTE-1");
        bool found = loadedOpt.has_value();
        bool titleMatches = false;
        bool descMatches = false;
        bool companyMatches = false;
        bool notesMatches = false;

        if (found) {
            const auto& loaded = loadedOpt.value();
            titleMatches = (loaded.getTitle() == "Two Sum, \"Special Edition\"");
            descMatches = (loaded.getDescription() == "Find indices i, j such that nums[i] + nums[j] == target, where target > 0.");
            companyMatches = (loaded.getCompany() == "Google, Inc.");
            notesMatches = (loaded.getNotes() == "Important \"tricky\" edge cases, remember 0.");
        }

        std::filesystem::remove(roundtripPath);
        return found && titleMatches && descMatches && companyMatches && notesMatches;
    });

    // Test 175: Persistence Hardening: Duplicate IDs in CSV Preserve Single Key
    runTest("Persistence Hardening: Duplicate IDs in CSV Preserve Single Key", [&]() {
        std::string dupPath = "data/test_dup_stage7.csv";
        {
            std::ofstream ofs(dupPath);
            ofs << "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\n";
            ofs << "Q-DUP,First Version,Desc1,Arrays,Easy,Comp,LeetCode,http://url,Solved,false,notes,100,100,0,0,2,tags,owner\n";
            ofs << "Q-DUP,Second Version,Desc2,Trees,Hard,Comp,LeetCode,http://url,Solved,true,notes,200,200,0,0,1,tags,owner\n";
        }

        codevault::persistence::FileQuestionRepository repo(dupPath);
        bool singleEntry = (repo.count() == 1);
        auto qOpt = repo.findById("Q-DUP");
        bool hasUpdatedTitle = qOpt.has_value() && (qOpt->getTitle() == "Second Version");

        std::filesystem::remove(dupPath);
        return singleEntry && hasUpdatedTitle;
    });

    // Test 176: DSA Hardening: DoublyLinkedList Copy/Move and Underflow Invariants
    runTest("DSA Hardening: DoublyLinkedList Copy/Move and Underflow Invariants", [&]() {
        codevault::dsa::DoublyLinkedList<int> list;
        list.pushBack(10);
        list.pushBack(20);
        list.pushBack(30);

        // Copy assignment
        codevault::dsa::DoublyLinkedList<int> copyList;
        copyList = list;
        bool copyOk = (copyList.size() == 3 && copyList.front() == 10 && copyList.back() == 30);

        // Move assignment
        codevault::dsa::DoublyLinkedList<int> moveList;
        moveList = std::move(copyList);
        bool moveOk = (moveList.size() == 3 && moveList.front() == 10 && moveList.back() == 30);

        // Clear and test underflow
        list.clear();
        bool threwFront = false;
        try {
            list.front();
        } catch (const std::underflow_error&) {
            threwFront = true;
        }

        bool threwBack = false;
        try {
            list.back();
        } catch (const std::underflow_error&) {
            threwBack = true;
        }

        bool threwPopFront = false;
        try {
            list.popFront();
        } catch (const std::underflow_error&) {
            threwPopFront = true;
        }

        bool threwPopBack = false;
        try {
            list.popBack();
        } catch (const std::underflow_error&) {
            threwPopBack = true;
        }

        bool threwOutOfRangeRemove = false;
        try {
            list.removeAt(5);
        } catch (const std::out_of_range&) {
            threwOutOfRangeRemove = true;
        }

        return copyOk && moveOk && threwFront && threwBack && threwPopFront && threwPopBack && threwOutOfRangeRemove;
    });

    // Test 177: DSA Hardening: Stack Copy/Move and Underflow Safety
    runTest("DSA Hardening: Stack Copy/Move and Underflow Safety", [&]() {
        codevault::dsa::Stack<std::string> stack;
        stack.push("Alpha");
        stack.push("Beta");

        // Copy assignment
        codevault::dsa::Stack<std::string> copyStack;
        copyStack = stack;
        bool copyOk = (copyStack.size() == 2 && copyStack.top() == "Beta");

        // Move assignment
        codevault::dsa::Stack<std::string> moveStack;
        moveStack = std::move(copyStack);
        bool moveOk = (moveStack.size() == 2 && moveStack.top() == "Beta");

        stack.pop();
        stack.pop();
        bool emptyOk = stack.empty();

        bool threwTop = false;
        try {
            stack.top();
        } catch (const std::underflow_error&) {
            threwTop = true;
        }

        bool threwPop = false;
        try {
            stack.pop();
        } catch (const std::underflow_error&) {
            threwPop = true;
        }

        return copyOk && moveOk && emptyOk && threwTop && threwPop;
    });

    // Test 178: DSA Hardening: Queue Copy/Move and Underflow Safety
    runTest("DSA Hardening: Queue Copy/Move and Underflow Safety", [&]() {
        codevault::dsa::Queue<std::string> queue;
        queue.enqueue("First");
        queue.enqueue("Second");

        // Copy assignment
        codevault::dsa::Queue<std::string> copyQueue;
        copyQueue = queue;
        bool copyOk = (copyQueue.size() == 2 && copyQueue.front() == "First" && copyQueue.back() == "Second");

        // Move assignment
        codevault::dsa::Queue<std::string> moveQueue;
        moveQueue = std::move(copyQueue);
        bool moveOk = (moveQueue.size() == 2 && moveQueue.front() == "First" && moveQueue.back() == "Second");

        queue.dequeue();
        queue.dequeue();
        bool emptyOk = queue.empty();

        bool threwFront = false;
        try {
            queue.front();
        } catch (const std::underflow_error&) {
            threwFront = true;
        }

        bool threwBack = false;
        try {
            queue.back();
        } catch (const std::underflow_error&) {
            threwBack = true;
        }

        bool threwDequeue = false;
        try {
            queue.dequeue();
        } catch (const std::underflow_error&) {
            threwDequeue = true;
        }

        return copyOk && moveOk && emptyOk && threwFront && threwBack && threwDequeue;
    });

    // Test 179: DSA Hardening: MinHeap Copy/Move, Underflow Safety & Equal Key Stability
    runTest("DSA Hardening: MinHeap Copy/Move, Underflow Safety & Equal Key Stability", [&]() {
        codevault::dsa::MinHeap<int> heap;
        heap.push(5);
        heap.push(3);
        heap.push(8);

        // Copy assignment
        codevault::dsa::MinHeap<int> copyHeap;
        copyHeap = heap;
        bool copyOk = (copyHeap.size() == 3 && copyHeap.top() == 3);

        // Move assignment
        codevault::dsa::MinHeap<int> moveHeap;
        moveHeap = std::move(copyHeap);
        bool moveOk = (moveHeap.size() == 3 && moveHeap.top() == 3);

        heap.clear();
        bool threwTop = false;
        try {
            heap.top();
        } catch (const std::underflow_error&) {
            threwTop = true;
        }

        bool threwPop = false;
        try {
            heap.pop();
        } catch (const std::underflow_error&) {
            threwPop = true;
        }

        // Test with identical keys
        codevault::dsa::MinHeap<int> equalHeap;
        for (int i = 0; i < 5; ++i) {
            equalHeap.push(42);
        }
        bool size5 = (equalHeap.size() == 5);
        bool allEqual = true;
        while (!equalHeap.empty()) {
            if (equalHeap.extractMin() != 42) allEqual = false;
        }

        return copyOk && moveOk && threwTop && threwPop && size5 && allEqual;
    });

    // Test 180: DSA Hardening: PrefixTrie Long Strings, Boundary Queries, and Punctuation
    runTest("DSA Hardening: PrefixTrie Long Strings, Boundary Queries, and Punctuation", [&]() {
        codevault::dsa::PrefixTrie trie;

        // Empty string operations
        bool emptyInsert = !trie.insert("");
        bool emptyContains = !trie.contains("");
        bool emptyRemove = !trie.remove("");

        // Very long string (300 characters)
        std::string longWord(300, 'a');
        longWord[150] = 'b';
        bool longInsert = trie.insert(longWord);
        bool longContains = trie.contains(longWord);
        bool longPrefix = trie.startsWith(longWord.substr(0, 100));
        bool longRemove = trie.remove(longWord);
        bool longGone = !trie.contains(longWord);

        // String with punctuation and hyphen
        std::string punct = "k-closest-points-to-origin";
        trie.insert(punct);
        bool punctMatch = trie.contains(punct);
        bool punctPrefix = trie.startsWith("k-closest");
        auto autoResults = trie.autocomplete("k-cl");
        bool autoFound = (!autoResults.empty() && autoResults[0] == punct);

        return emptyInsert && emptyContains && emptyRemove &&
               longInsert && longContains && longPrefix && longRemove && longGone &&
               punctMatch && punctPrefix && autoFound;
    });

    // Test 181: Domain Hardening: Question Title Length Boundary and Priority Validation
    runTest("Domain Hardening: Question Title Length Boundary and Priority Validation", [&]() {
        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>("data/sample/questions.csv");
        codevault::services::QuestionService service(repo);

        // Exact 255 char title
        std::string title255(255, 'A');
        codevault::models::Question q255(
            "Q-LEN-255", title255, "Desc",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Google", codevault::models::Platform::LeetCode,
            "https://leetcode.com/q", codevault::models::Status::Todo,
            false, "", 100, 100, 0, 0, 3, {}
        );
        auto val255 = service.validateQuestion(q255, true);
        bool ok255 = val255.isValid;

        // 256 char title
        std::string title256(256, 'B');
        codevault::models::Question q256(
            "Q-LEN-256", title256, "Desc",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Google", codevault::models::Platform::LeetCode,
            "https://leetcode.com/q", codevault::models::Status::Todo,
            false, "", 100, 100, 0, 0, 3, {}
        );
        auto val256 = service.validateQuestion(q256, true);
        bool rejected256 = !val256.isValid;

        // Whitespace only title
        codevault::models::Question qWhitespace(
            "Q-WHITE", "   \t\n   ", "Desc",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Google", codevault::models::Platform::LeetCode,
            "https://leetcode.com/q", codevault::models::Status::Todo,
            false, "", 100, 100, 0, 0, 3, {}
        );
        auto valWhite = service.validateQuestion(qWhitespace, true);
        bool rejectedWhite = !valWhite.isValid;

        // Invalid ID characters (contains '#' or space)
        codevault::models::Question qBadId(
            "Q#INVALID 1", "Title", "Desc",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Google", codevault::models::Platform::LeetCode,
            "https://leetcode.com/q", codevault::models::Status::Todo,
            false, "", 100, 100, 0, 0, 3, {}
        );
        auto valBadId = service.validateQuestion(qBadId, true);
        bool rejectedBadId = !valBadId.isValid;

        // Priority boundary: 0 and 6 rejected, 1 and 5 accepted
        q255.setRevisionPriority(0);
        bool rejectedPrio0 = !service.validateQuestion(q255, true).isValid;
        q255.setRevisionPriority(6);
        bool rejectedPrio6 = !service.validateQuestion(q255, true).isValid;
        q255.setRevisionPriority(1);
        bool okPrio1 = service.validateQuestion(q255, true).isValid;
        q255.setRevisionPriority(5);
        bool okPrio5 = service.validateQuestion(q255, true).isValid;

        return ok255 && rejected256 && rejectedWhite && rejectedBadId &&
               rejectedPrio0 && rejectedPrio6 && okPrio1 && okPrio5;
    });

    // Test 182: Revision Hardening: Exact Timestamp Boundaries & Unscheduled Exclusions
    runTest("Revision Hardening: Exact Timestamp Boundaries & Unscheduled Exclusions", [&]() {
        auto mockClock = std::make_shared<codevault::utils::MockClock>(1000000);
        codevault::services::RevisionService revisionService(mockClock);

        // Question 1: nextRevisionAt == now (1000000) -> DUE
        codevault::models::Question qDue(
            "Q-DUE-EXACT", "Due Exact", "Desc",
            codevault::models::Topic::Arrays, codevault::models::Difficulty::Medium,
            "Company", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::InProgress, false, "",
            100, 100, 1000, 1000000, 2, {}
        );

        // Question 2: nextRevisionAt == now + 1 (1000001) -> NOT due (Upcoming)
        codevault::models::Question qFuture(
            "Q-FUTURE-1", "Future 1", "Desc",
            codevault::models::Topic::Arrays, codevault::models::Difficulty::Medium,
            "Company", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::InProgress, false, "",
            100, 100, 1000, 1000001, 2, {}
        );

        // Question 3: nextRevisionAt == 0 -> Unscheduled
        codevault::models::Question qUnscheduled(
            "Q-UNSCHED", "Unscheduled", "Desc",
            codevault::models::Topic::Arrays, codevault::models::Difficulty::Medium,
            "Company", codevault::models::Platform::LeetCode, "",
            codevault::models::Status::Todo, false, "",
            100, 100, 0, 0, 2, {}
        );

        revisionService.loadFromQuestions({qDue, qFuture, qUnscheduled});

        auto dueList = revisionService.getDueQuestions();
        auto upcomingList = revisionService.getUpcomingRevisions();

        bool dueExactFound = (dueList.size() == 1 && dueList[0].questionId == "Q-DUE-EXACT");
        bool futureFoundInUpcoming = false;
        for (const auto& item : upcomingList) {
            if (item.questionId == "Q-FUTURE-1") futureFoundInUpcoming = true;
        }

        bool unscheduledExcludedFromDue = true;
        for (const auto& item : dueList) {
            if (item.questionId == "Q-UNSCHED") unscheduledExcludedFromDue = false;
        }

        bool unscheduledExcludedFromUpcoming = true;
        for (const auto& item : upcomingList) {
            if (item.questionId == "Q-UNSCHED") unscheduledExcludedFromUpcoming = false;
        }

        return dueExactFound && futureFoundInUpcoming && unscheduledExcludedFromDue && unscheduledExcludedFromUpcoming;
    });

    // Test 183: Practice Hardening: Empty Session Invariants & Single Question Cycle
    runTest("Practice Hardening: Empty Session Invariants & Single Question Cycle", [&]() {
        auto practiceService = std::make_shared<codevault::services::PracticeService>();

        // Empty session queries
        bool noCurrent = !practiceService->getCurrentQuestion().has_value();
        bool noRemaining = (practiceService->getRemainingCount() == 0);
        bool noSkipEmpty = !practiceService->skipCurrentQuestion();
        auto attemptRes = practiceService->recordPracticeAttempt(codevault::models::PracticeVerdict::Solved);
        bool noRecordEmpty = !attemptRes.has_value();

        // Single question session
        practiceService->startSession({"Q-SINGLE"});
        bool singleCurrent = (practiceService->getCurrentQuestion() == "Q-SINGLE");
        // Skipping on single question must return false and keep question
        bool singleSkipFailed = !practiceService->skipCurrentQuestion();
        bool stillSingleCurrent = (practiceService->getCurrentQuestion() == "Q-SINGLE");

        return noCurrent && noRemaining && noSkipEmpty && noRecordEmpty &&
               singleCurrent && singleSkipFailed && stillSingleCurrent;
    });

    // Test 184: UI Helper Hardening: CliShell::renderProgressBar Boundary Widths & Rounding
    runTest("UI Helper Hardening: CliShell::renderProgressBar Boundary Widths & Rounding", [&]() {
        // Zero or negative width defaults to 20
        std::string barDef = codevault::app::CliShell::renderProgressBar(50.0, 0);
        bool width20 = (barDef == "[##########----------]");

        // Width 4, 75% -> 3 hashes, 1 dash
        std::string bar4 = codevault::app::CliShell::renderProgressBar(75.0, 4);
        bool width4 = (bar4 == "[###-]");

        // Width 1, 100% -> 1 hash
        std::string bar1 = codevault::app::CliShell::renderProgressBar(100.0, 1);
        bool width1 = (bar1 == "[#]");

        // Width 1, 0% -> 1 dash
        std::string bar1Zero = codevault::app::CliShell::renderProgressBar(0.0, 1);
        bool width1Zero = (bar1Zero == "[-]");

        return width20 && width4 && width1 && width1Zero;
    });

    // =========================================================================
    // 20. Stage 7: End-to-End Integration Workflows (A through F)
    // =========================================================================
    std::cout << "\n--- 20. Stage 7: End-to-End Integration Workflows ---\n";

    // Test 185: Workflow A: Create -> Persist -> Reload -> Search -> Filter -> Sort
    runTest("Workflow A: Create -> Persist -> Reload -> Search -> Filter -> Sort", [&]() {
        const std::string testPath = "test_workflow_a.csv";
        if (std::filesystem::exists(testPath)) std::filesystem::remove(testPath);

        {
            auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
            auto searchService = std::make_shared<codevault::services::SearchService>();
            codevault::services::QuestionService qService(repo, searchService);

            qService.createQuestion(codevault::models::Question(
                "Q-A1", "Two Sum", "Find two numbers",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy,
                "Google", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Unsolved, true, "classic",
                100, 100, 0, 0, 2, {"hashmap", "array"}
            ));

            qService.createQuestion(codevault::models::Question(
                "Q-A2", "3Sum", "Find three numbers summing to zero",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Medium,
                "Facebook", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::InProgress, false, "",
                100, 100, 0, 0, 2, {"twopointer"}
            ));

            qService.createQuestion(codevault::models::Question(
                "Q-A3", "Course Schedule", "Detect cycle in graph",
                codevault::models::Topic::Graphs, codevault::models::Difficulty::Medium,
                "Amazon", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Unsolved, false, "",
                100, 100, 0, 0, 2, {"toposort"}
            ));

            qService.createQuestion(codevault::models::Question(
                "Q-A4", "Alien Dictionary", "Topological order of letters",
                codevault::models::Topic::Graphs, codevault::models::Difficulty::Hard,
                "Google", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Solved, true, "",
                100, 100, 200, 500, 1, {"graph", "hard"}
            ));
        }

        // Reload fresh repository and services from disk
        auto reloadRepo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto reloadSearch = std::make_shared<codevault::services::SearchService>();
        codevault::services::QuestionService reloadService(reloadRepo, reloadSearch);

        // 1. Prefix search
        auto prefixResults = reloadService.searchQuestionsByTitlePrefix("Two");
        bool prefixFound = (prefixResults.size() == 1 && prefixResults[0].getId() == "Q-A1");

        // 2. Keyword search
        auto keywordResults = reloadSearch->searchByKeyword(reloadService.getAllQuestions(), "Dictionary");
        bool keywordFound = (keywordResults.size() == 1 && keywordResults[0].getId() == "Q-A4");

        // 3. Filter by Topic Graphs
        codevault::models::QuestionFilter graphFilter;
        graphFilter.topic = codevault::models::Topic::Graphs;
        auto graphFiltered = reloadSearch->filter(reloadService.getAllQuestions(), graphFilter);
        bool filterMatch = (graphFiltered.size() == 2);

        // 4. Sort graph filtered results by Difficulty descending (Hard -> Medium)
        codevault::models::SortOptions sortOptions{codevault::models::SortField::Difficulty, codevault::models::SortDirection::Descending};
        reloadSearch->sort(graphFiltered, sortOptions);
        bool sortMatch = (graphFiltered.size() == 2 &&
                          graphFiltered[0].getId() == "Q-A4" &&
                          graphFiltered[1].getId() == "Q-A3");

        std::filesystem::remove(testPath);
        return prefixFound && keywordFound && filterMatch && sortMatch;
    });

    // Test 186: Workflow B: Create -> Practice -> Mark Solved -> Revision Schedule Generated -> Persist -> Reload
    runTest("Workflow B: Create -> Practice -> Solved -> Schedule -> Persist -> Reload", [&]() {
        const std::string testPath = "test_workflow_b.csv";
        if (std::filesystem::exists(testPath)) std::filesystem::remove(testPath);

        auto mockClock = std::make_shared<codevault::utils::MockClock>(100000);
        {
            auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
            auto search = std::make_shared<codevault::services::SearchService>();
            auto revService = std::make_shared<codevault::services::RevisionService>(mockClock);
            auto qService = std::make_shared<codevault::services::QuestionService>(repo, search, revService);
            auto practiceService = std::make_shared<codevault::services::PracticeService>(qService, revService);

            qService->createQuestion(codevault::models::Question(
                "Q-B1", "Binary Search", "Find target in sorted array",
                codevault::models::Topic::BinarySearch, codevault::models::Difficulty::Easy,
                "Apple", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Unsolved, false, "",
                100000, 100000, 0, 0, 2, {}
            ));

            // Start practice session with Q-B1
            practiceService->startSession({"Q-B1"});

            // Solve question through practice service (which delegates to revision service and question service)
            auto verdictRes = practiceService->recordPracticeAttempt(codevault::models::PracticeVerdict::Solved, 100000);
            (void)verdictRes;
        }

        // Reload fresh stack
        auto reloadRepo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto reloadRev = std::make_shared<codevault::services::RevisionService>(mockClock);
        auto reloadService = std::make_shared<codevault::services::QuestionService>(reloadRepo, nullptr, reloadRev);

        auto optReloaded = reloadService->getQuestionById("Q-B1");
        bool questionPresent = optReloaded.has_value();
        bool isSolved = (questionPresent && optReloaded->getStatus() == codevault::models::Status::Solved);
        // Next revision should be 100000 + 86400 = 186400 (Level 1)
        bool nextRevScheduled = (questionPresent && optReloaded->getNextRevisionAt() == 186400);

        // At now = 100000, it is upcoming, not yet due
        auto dueBefore = reloadRev->getDueQuestions(mockClock->now());
        bool notDueYet = dueBefore.empty();

        // Advance clock past due timestamp
        mockClock->setTime(190000);
        auto dueAfter = reloadRev->getDueQuestions(mockClock->now());
        bool nowDue = (dueAfter.size() == 1 && dueAfter[0].questionId == "Q-B1");

        std::filesystem::remove(testPath);
        return questionPresent && isSolved && nextRevScheduled && notDueYet && nowDue;
    });

    // Test 187: Workflow C: Needs Review -> Revision Resets to 1 Day & Urgent Priority -> Persist -> Reload
    runTest("Workflow C: Needs Review -> Interval Reset & Urgent Priority -> Persist -> Reload", [&]() {
        const std::string testPath = "test_workflow_c.csv";
        if (std::filesystem::exists(testPath)) std::filesystem::remove(testPath);

        auto mockClock = std::make_shared<codevault::utils::MockClock>(500000);
        {
            auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
            auto revService = std::make_shared<codevault::services::RevisionService>(mockClock);
            auto qService = std::make_shared<codevault::services::QuestionService>(repo, nullptr, revService);

            // Question was previously at level 4 (+14 days = 1209600s), priority 3
            qService->createQuestion(codevault::models::Question(
                "Q-C1", "LRU Cache", "Implement doubly linked list cache",
                codevault::models::Topic::LinkedLists, codevault::models::Difficulty::Medium,
                "Amazon", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Solved, false, "",
                100, 100, 100, 500000 + 1209600, 3, {}
            ));

            auto optQ = qService->getQuestionById("Q-C1");
            auto q = optQ.value();
            // Process NeedsReview verdict
            revService->markRevisionResult(q, codevault::models::PracticeVerdict::NeedsReview, 500000);
            qService->updateQuestion(q);
        }

        // Reload fresh stack
        auto reloadRepo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto reloadRev = std::make_shared<codevault::services::RevisionService>(mockClock);
        auto reloadService = std::make_shared<codevault::services::QuestionService>(reloadRepo, nullptr, reloadRev);

        auto optReloaded = reloadService->getQuestionById("Q-C1");
        bool qOk = optReloaded.has_value();
        // Priority must be urgent (1)
        bool prioUrgent = (qOk && optReloaded->getRevisionPriority() == 1);
        // Status must be InProgress
        bool statusInProgress = (qOk && optReloaded->getStatus() == codevault::models::Status::InProgress);
        // Interval reset to 1 day: 500000 + 86400 = 586400
        bool intervalReset = (qOk && optReloaded->getNextRevisionAt() == 586400);

        // At clock = 600000, verify due at highest priority
        mockClock->setTime(600000);
        auto dueList = reloadRev->getDueQuestions(mockClock->now());
        bool dueWithHighPrio = (!dueList.empty() && dueList[0].questionId == "Q-C1" && dueList[0].priority == 1);

        std::filesystem::remove(testPath);
        return qOk && prioUrgent && statusInProgress && intervalReset && dueWithHighPrio;
    });

    // Test 188: Workflow D: Multiple Questions -> Practice Queue -> Skip -> Solve -> Exit -> State & Persistence
    runTest("Workflow D: Practice Queue Skip -> Solve -> Exit Session State & Persistence", [&]() {
        const std::string testPath = "test_workflow_d.csv";
        if (std::filesystem::exists(testPath)) std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto practiceService = std::make_shared<codevault::services::PracticeService>();
        auto qService = std::make_shared<codevault::services::QuestionService>(repo);

        qService->createQuestion(codevault::models::Question(
            "Q-D1", "First Question", "Desc", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {}
        ));
        qService->createQuestion(codevault::models::Question(
            "Q-D2", "Second Question", "Desc", codevault::models::Topic::Strings,
            codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {}
        ));
        qService->createQuestion(codevault::models::Question(
            "Q-D3", "Third Question", "Desc", codevault::models::Topic::Trees,
            codevault::models::Difficulty::Hard, "", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {}
        ));

        // Start session with {Q-D1, Q-D2, Q-D3}
        practiceService->startSession({"Q-D1", "Q-D2", "Q-D3"});
        bool frontIsD1 = (practiceService->getCurrentQuestion() == "Q-D1");

        // Skip Q-D1 -> front moves to Q-D2, Q-D1 pushed to tail
        bool skipSuccess = practiceService->skipCurrentQuestion();
        bool frontIsD2 = (practiceService->getCurrentQuestion() == "Q-D2");

        // Solve Q-D2: complete from practice queue
        auto completedId = practiceService->completeCurrentQuestion();
        bool solveSuccess = (completedId.has_value() && *completedId == "Q-D2");
        auto optD2 = qService->getQuestionById("Q-D2");
        auto d2 = optD2.value();
        d2.setStatus(codevault::models::Status::Solved);
        qService->updateQuestion(d2);

        // Next front should be Q-D3
        bool frontIsD3 = (practiceService->getCurrentQuestion() == "Q-D3");

        // Exit session without completing Q-D3 or Q-D1
        auto summary = practiceService->getProgress();
        bool summaryCorrect = (summary.total == 3 &&
                               summary.completed == 1 &&
                               summary.skipped == 1 &&
                               summary.remaining == 2);
        practiceService->clearSession();
        bool sessionEmpty = !practiceService->getCurrentQuestion().has_value();

        // Verify persistence: Q-D2 is Solved, Q-D1 and Q-D3 are still Unsolved
        auto reloadRepo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto reloadService = std::make_shared<codevault::services::QuestionService>(reloadRepo);
        bool d1Unsolved = (reloadService->getQuestionById("Q-D1")->getStatus() == codevault::models::Status::Unsolved);
        bool d2Solved = (reloadService->getQuestionById("Q-D2")->getStatus() == codevault::models::Status::Solved);
        bool d3Unsolved = (reloadService->getQuestionById("Q-D3")->getStatus() == codevault::models::Status::Unsolved);

        std::filesystem::remove(testPath);
        return frontIsD1 && skipSuccess && frontIsD2 && solveSuccess && frontIsD3 &&
               summaryCorrect && sessionEmpty && d1Unsolved && d2Solved && d3Unsolved;
    });

    // Test 189: Workflow E: Modify Question -> Dashboard Statistics Update -> Source Immutability
    runTest("Workflow E: Modify Question -> Dashboard Statistics Update -> Source Immutability", [&]() {
        codevault::services::StatisticsService statsService;
        std::vector<codevault::models::Question> questions;

        questions.push_back(codevault::models::Question(
            "Q-E1", "Q1", "Desc", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy, "Google", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Solved, false, "", 100, 100, 100, 0, 2, {}
        ));
        questions.push_back(codevault::models::Question(
            "Q-E2", "Q2", "Desc", codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Medium, "Amazon", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {}
        ));
        questions.push_back(codevault::models::Question(
            "Q-E3", "Q3", "Desc", codevault::models::Topic::DynamicProgramming,
            codevault::models::Difficulty::Hard, "Meta", codevault::models::Platform::LeetCode,
            "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 2, {}
        ));

        // Snapshot 1: 1 Solved out of 3 -> ~33.33%
        auto snap1 = statsService.computeDashboardSnapshot(questions, 1000);
        bool snap1Solved = (snap1.overall.solvedCount == 1 && snap1.overall.totalQuestions == 3);
        double diff1 = std::abs(snap1.overall.completionPercentage - (100.0 / 3.0));

        // Modify Q-E2 to Solved
        questions[1].setStatus(codevault::models::Status::Solved);

        // Snapshot 2: 2 Solved out of 3 -> ~66.67%
        auto snap2 = statsService.computeDashboardSnapshot(questions, 1000);
        bool snap2Solved = (snap2.overall.solvedCount == 2 && snap2.overall.unsolvedCount == 1);
        double diff2 = std::abs(snap2.overall.completionPercentage - (200.0 / 3.0));

        // Source question data integrity: verify original attributes were not modified by statistics
        bool q3Intact = (questions[2].getTitle() == "Q3" &&
                         questions[2].getDifficulty() == codevault::models::Difficulty::Hard &&
                         questions[2].getStatus() == codevault::models::Status::Unsolved);

        return snap1Solved && (diff1 < 0.01) && snap2Solved && (diff2 < 0.01) && q3Intact;
    });

    // Test 190: Workflow F: Multi-Service Pipeline -> Restart Parity Consistency
    runTest("Workflow F: Multi-Service Pipeline -> Restart Parity Consistency", [&]() {
        const std::string testPath = "test_workflow_f.csv";
        if (std::filesystem::exists(testPath)) std::filesystem::remove(testPath);

        auto mockClock = std::make_shared<codevault::utils::MockClock>(200000);
        {
            auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
            auto search = std::make_shared<codevault::services::SearchService>();
            auto rev = std::make_shared<codevault::services::RevisionService>(mockClock);
            codevault::services::QuestionService qService(repo, search, rev);

            qService.createQuestion(codevault::models::Question(
                "Q-F1", "Graph Valid Tree", "Desc", codevault::models::Topic::Graphs,
                codevault::models::Difficulty::Medium, "Google", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::Solved, true, "", 100, 100, 150000, 180000, 1, {"graph", "tree"}
            ));
            qService.createQuestion(codevault::models::Question(
                "Q-F2", "Word Ladder", "Desc", codevault::models::Topic::Graphs,
                codevault::models::Difficulty::Hard, "Amazon", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::InProgress, false, "", 100, 100, 160000, 250000, 2, {"bfs"}
            ));
            qService.createQuestion(codevault::models::Question(
                "Q-F3", "Subsets", "Desc", codevault::models::Topic::RecursionBacktracking,
                codevault::models::Difficulty::Medium, "Facebook", codevault::models::Platform::LeetCode,
                "", codevault::models::Status::Unsolved, false, "", 100, 100, 0, 0, 3, {"recursion"}
            ));
        }

        // Function to run identical pipeline and return checksum metrics
        auto runPipeline = [&](const std::string& path) {
            auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(path);
            auto search = std::make_shared<codevault::services::SearchService>();
            auto rev = std::make_shared<codevault::services::RevisionService>(mockClock);
            codevault::services::StatisticsService stats;
            codevault::services::QuestionService qService(repo, search, rev);

            // Search
            auto prefix = qService.searchQuestionsByTitlePrefix("Word");
            // Filter
            codevault::models::QuestionFilter filter;
            filter.topic = codevault::models::Topic::Graphs;
            auto filtered = search->filter(qService.getAllQuestions(), filter);
            // Sort
            codevault::models::SortOptions sortOpt{codevault::models::SortField::Difficulty, codevault::models::SortDirection::Ascending};
            search->sort(filtered, sortOpt);

            // Dashboard
            auto snap = stats.computeDashboardSnapshot(qService.getAllQuestions(), 200000);

            // Revision due query (at 200000, Q-F1 with nextRevisionAt=180000 is due, Q-F2 at 250000 is upcoming)
            auto due = rev->getDueQuestions(mockClock->now());

            return std::make_tuple(prefix.size(), filtered.size(), filtered.size(),
                                   snap.overall.totalQuestions, due.size(),
                                   (filtered.empty() ? "" : filtered[0].getId()));
        };

        auto run1 = runPipeline(testPath);
        auto run2 = runPipeline(testPath);

        bool identical = (run1 == run2);
        bool validValues = (std::get<0>(run1) == 1 &&   // Prefix "Word" -> Word Ladder
                            std::get<1>(run1) == 2 &&   // Graphs -> 2
                            std::get<2>(run1) == 2 &&   // Sorted -> 2
                            std::get<3>(run1) == 3 &&   // Total -> 3
                            std::get<4>(run1) == 1 &&   // Due -> 1 (Q-F1)
                            std::get<5>(run1) == "Q-F1"); // Ascending diff: Medium before Hard

        std::filesystem::remove(testPath);
        return identical && validValues;
    });

    // =========================================================================
    // 21. Stage 8: Web Integration, JSON Serialization & HTTP API Routing
    // =========================================================================
    std::cout << "\n--- 21. Stage 8: Web Integration & JSON Serialization ---\n";

    runTest("JSON: String Escaping & Special Characters", []() {
        std::string raw = "Problem with \"quotes\", \\backslashes\\, and \n newlines \t tabs.";
        std::string escaped = codevault::utils::json::escape(raw);
        bool hasEscapedQuote = (escaped.find("\\\"") != std::string::npos);
        bool hasEscapedBackslash = (escaped.find("\\\\") != std::string::npos);
        bool hasEscapedNewline = (escaped.find("\\n") != std::string::npos);
        bool hasEscapedTab = (escaped.find("\\t") != std::string::npos);
        return hasEscapedQuote && hasEscapedBackslash && hasEscapedNewline && hasEscapedTab;
    });

    runTest("JSON: Question Entity Serialization Round-Trip", []() {
        codevault::models::Question q(
            "Q-8001", "Serialize Test", "Testing serialization logic",
            codevault::models::Topic::DynamicProgramming, codevault::models::Difficulty::Medium,
            "Amazon", codevault::models::Platform::LeetCode, "https://leetcode.com",
            codevault::models::Status::Solved, true, "Memoization + Tabulation",
            1700000, 1710000, 1715000, 1720000, 1, {"dp", "memo"}, "owner_x"
        );
        std::string jsonStr = codevault::utils::json::questionToJson(q);
        auto parsed = codevault::utils::json::parse(jsonStr);

        codevault::models::Question qBack;
        std::string err;
        bool ok = codevault::utils::json::jsonToQuestion(parsed, qBack, err);

        return ok && qBack.getId() == "Q-8001" && qBack.getTitle() == "Serialize Test" &&
               qBack.getTopic() == codevault::models::Topic::DynamicProgramming &&
               qBack.getDifficulty() == codevault::models::Difficulty::Medium &&
               qBack.isFavorite() && qBack.getStatus() == codevault::models::Status::Solved &&
               qBack.getRevisionPriority() == 1 && qBack.getTags().size() == 2;
    });

    runTest("JSON: DashboardSnapshot Serialization Completeness", []() {
        codevault::models::DashboardSnapshot snap;
        snap.generatedAt = 123456789;
        snap.overall.totalQuestions = 10;
        snap.overall.solvedCount = 5;
        snap.overall.completionPercentage = 50.0;
        snap.difficulty.easyCount = 4;
        snap.difficulty.easyPercentage = 40.0;
        snap.revision.dueCount = 3;

        std::string jsonStr = codevault::utils::json::dashboardSnapshotToJson(snap);
        auto parsed = codevault::utils::json::parse(jsonStr);

        return parsed.getInt("generatedAt") == 123456789 &&
               parsed.isObject() &&
               jsonStr.find("\"totalQuestions\":10") != std::string::npos &&
               jsonStr.find("\"solvedCount\":5") != std::string::npos &&
               jsonStr.find("\"completionPercentage\":50.0") != std::string::npos &&
               jsonStr.find("\"dueCount\":3") != std::string::npos;
    });

    runTest("JSON: RevisionItem & SessionProgress Serialization", []() {
        codevault::models::RevisionItem item{"Q-8002", 1750000, 1};
        std::string itemJson = codevault::utils::json::revisionItemToJson(item);

        codevault::services::SessionProgress prog{10, 4, 5, 1};
        std::string progJson = codevault::utils::json::sessionProgressToJson(prog);

        return itemJson.find("\"questionId\":\"Q-8002\"") != std::string::npos &&
               itemJson.find("\"priority\":1") != std::string::npos &&
               progJson.find("\"total\":10") != std::string::npos &&
               progJson.find("\"completed\":4") != std::string::npos &&
               progJson.find("\"remaining\":5") != std::string::npos &&
               progJson.find("\"skipped\":1") != std::string::npos;
    });

    runTest("JSON: Request Payload Parser & jsonToQuestion Extraction", []() {
        std::string rawJson = "{\"title\":\"Parsed Problem\",\"topic\":\"Graphs\",\"difficulty\":\"Hard\","
                              "\"company\":\"Google\",\"status\":\"InProgress\",\"is_favorite\":true,"
                              "\"revision_priority\":1,\"tags\":[\"bfs\",\"dfs\"]}";
        auto root = codevault::utils::json::parse(rawJson);

        codevault::models::Question q;
        std::string err;
        bool ok = codevault::utils::json::jsonToQuestion(root, q, err);

        return ok && q.getTitle() == "Parsed Problem" &&
               q.getTopic() == codevault::models::Topic::Graphs &&
               q.getDifficulty() == codevault::models::Difficulty::Hard &&
               q.getCompany() == "Google" &&
               q.getStatus() == codevault::models::Status::InProgress &&
               q.isFavorite() && q.getRevisionPriority() == 1 &&
               q.getTags().size() == 2;
    });

    runTest("HTTP API: Dashboard Endpoint /api/dashboard Returns Valid Snapshot", []() {
        std::string testPath = "data/test_http_dashboard.csv";
        std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto rev = std::make_shared<codevault::services::RevisionService>();
        auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
        auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
        auto hist = std::make_shared<codevault::services::RecentHistoryService>();
        auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

        codevault::models::Question q("Q-API1", "API Question", "", codevault::models::Topic::Arrays,
                                      codevault::models::Difficulty::Easy, "Apple",
                                      codevault::models::Platform::LeetCode, "",
                                      codevault::models::Status::Solved, true, "",
                                      1000, 2000, 0, 3000, 2, {});
        qs->createQuestion(q);

        codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, 8891);

        codevault::app::HttpRequest req;
        req.method = "GET";
        req.path = "/api/dashboard";

        auto res = server.handleRequest(req);

        std::filesystem::remove(testPath);
        return res.statusCode == 200 && res.body.find("\"totalQuestions\":1") != std::string::npos &&
               res.body.find("\"solvedCount\":1") != std::string::npos;
    });

    runTest("HTTP API: Question CRUD & Query Filtering via /api/questions", []() {
        std::string testPath = "data/test_http_crud.csv";
        std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto rev = std::make_shared<codevault::services::RevisionService>();
        auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
        auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
        auto hist = std::make_shared<codevault::services::RecentHistoryService>();
        auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

        codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, 8892);

        // 1. Create Question via POST
        codevault::app::HttpRequest postReq;
        postReq.method = "POST";
        postReq.path = "/api/questions";
        postReq.body = "{\"id\":\"Q-HTTP-1\",\"title\":\"Two Pointers Target\",\"topic\":\"Arrays\",\"difficulty\":\"Medium\",\"company\":\"Meta\"}";
        auto postRes = server.handleRequest(postReq);
        bool postOk = (postRes.statusCode == 201 && postRes.body.find("\"id\":\"Q-HTTP-1\"") != std::string::npos);

        // 2. Query Question via GET by ID
        codevault::app::HttpRequest getByIdReq;
        getByIdReq.method = "GET";
        getByIdReq.path = "/api/questions/Q-HTTP-1";
        auto getByIdRes = server.handleRequest(getByIdReq);
        bool getByIdOk = (getByIdRes.statusCode == 200 && getByIdRes.body.find("\"title\":\"Two Pointers Target\"") != std::string::npos);

        // 3. Filter via GET with query params
        codevault::app::HttpRequest filterReq;
        filterReq.method = "GET";
        filterReq.path = "/api/questions";
        filterReq.queryParams["topic"] = "Arrays";
        filterReq.queryParams["difficulty"] = "Medium";
        auto filterRes = server.handleRequest(filterReq);
        bool filterOk = (filterRes.statusCode == 200 && filterRes.body.find("Q-HTTP-1") != std::string::npos);

        // 4. Delete Question via DELETE
        codevault::app::HttpRequest delReq;
        delReq.method = "DELETE";
        delReq.path = "/api/questions/Q-HTTP-1";
        auto delRes = server.handleRequest(delReq);
        bool delOk = (delRes.statusCode == 200 && delRes.body.find("\"success\":true") != std::string::npos);

        std::filesystem::remove(testPath);
        return postOk && getByIdOk && filterOk && delOk;
    });

    runTest("HTTP API: Practice Session Queue Endpoints (/api/practice/start, /current, /verdict)", []() {
        std::string testPath = "data/test_http_practice.csv";
        std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto rev = std::make_shared<codevault::services::RevisionService>();
        auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
        auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
        auto hist = std::make_shared<codevault::services::RecentHistoryService>();
        auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

        codevault::models::Question q1("Q-PRAC-1", "Practice Prob 1", "", codevault::models::Topic::Arrays,
                                       codevault::models::Difficulty::Easy, "",
                                       codevault::models::Platform::LeetCode, "",
                                       codevault::models::Status::Unsolved, false, "",
                                       1000, 2000, 0, 0, 2, {});
        qs->createQuestion(q1);

        codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, 8893);

        // Start session
        codevault::app::HttpRequest startReq;
        startReq.method = "POST";
        startReq.path = "/api/practice/start";
        startReq.body = "{\"questionIds\":[\"Q-PRAC-1\"]}";
        auto startRes = server.handleRequest(startReq);
        bool startOk = (startRes.statusCode == 200 && startRes.body.find("\"total\":1") != std::string::npos);

        // Get current
        codevault::app::HttpRequest curReq;
        curReq.method = "GET";
        curReq.path = "/api/practice/current";
        auto curRes = server.handleRequest(curReq);
        bool curOk = (curRes.statusCode == 200 && curRes.body.find("\"hasQuestion\":true") != std::string::npos);

        // Verdict Solved
        codevault::app::HttpRequest verdictReq;
        verdictReq.method = "POST";
        verdictReq.path = "/api/practice/verdict";
        verdictReq.body = "{\"verdict\":\"Solved\"}";
        auto verdictRes = server.handleRequest(verdictReq);
        bool verdictOk = (verdictRes.statusCode == 200 && verdictRes.body.find("\"success\":true") != std::string::npos);

        std::filesystem::remove(testPath);
        return startOk && curOk && verdictOk;
    });

    runTest("HTTP API: Spaced Revision Queue Endpoints (/api/revision/due, /schedule)", []() {
        std::string testPath = "data/test_http_revision.csv";
        std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto mockClock = std::make_shared<codevault::utils::MockClock>(200000);
        auto rev = std::make_shared<codevault::services::RevisionService>(mockClock);
        auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
        auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
        auto hist = std::make_shared<codevault::services::RecentHistoryService>();
        auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

        codevault::models::Question q("Q-REV-1", "Revision Prob", "", codevault::models::Topic::Trees,
                                      codevault::models::Difficulty::Medium, "",
                                      codevault::models::Platform::LeetCode, "",
                                      codevault::models::Status::Solved, false, "",
                                      1000, 2000, 1000, 150000, 1, {});
        qs->createQuestion(q);

        codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, 8894);

        // 1. Get Due
        codevault::app::HttpRequest dueReq;
        dueReq.method = "GET";
        dueReq.path = "/api/revision/due";
        auto dueRes = server.handleRequest(dueReq);
        bool dueOk = (dueRes.statusCode == 200 && dueRes.body.find("Q-REV-1") != std::string::npos);

        // 2. Schedule custom
        codevault::app::HttpRequest schedReq;
        schedReq.method = "POST";
        schedReq.path = "/api/revision/schedule";
        schedReq.body = "{\"questionId\":\"Q-REV-1\",\"nextRevisionAt\":500000,\"priority\":2}";
        auto schedRes = server.handleRequest(schedReq);
        bool schedOk = (schedRes.statusCode == 200 && schedRes.body.find("\"success\":true") != std::string::npos);

        std::filesystem::remove(testPath);
        return dueOk && schedOk;
    });

    runTest("HTTP API: History Stack & Diagnostics Endpoints (/api/history, /settings/diagnostics)", []() {
        std::string testPath = "data/test_http_history.csv";
        std::filesystem::remove(testPath);

        auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(testPath);
        auto rev = std::make_shared<codevault::services::RevisionService>();
        auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
        auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
        auto hist = std::make_shared<codevault::services::RecentHistoryService>();
        auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

        codevault::models::Question q("Q-HIST-1", "History Prob", "", codevault::models::Topic::Heaps,
                                      codevault::models::Difficulty::Hard, "",
                                      codevault::models::Platform::LeetCode, "",
                                      codevault::models::Status::Unsolved, false, "",
                                      1000, 2000, 0, 0, 2, {});
        qs->createQuestion(q);

        codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, 8895);

        // View question to push on history stack
        codevault::app::HttpRequest viewReq;
        viewReq.method = "GET";
        viewReq.path = "/api/questions/Q-HIST-1";
        server.handleRequest(viewReq);

        // Query history
        codevault::app::HttpRequest histReq;
        histReq.method = "GET";
        histReq.path = "/api/history";
        auto histRes = server.handleRequest(histReq);
        bool histOk = (histRes.statusCode == 200 && histRes.body.find("Q-HIST-1") != std::string::npos);

        // Query diagnostics
        codevault::app::HttpRequest diagReq;
        diagReq.method = "GET";
        diagReq.path = "/api/settings/diagnostics";
        auto diagRes = server.handleRequest(diagReq);
        bool diagOk = (diagRes.statusCode == 200 && diagRes.body.find("\"version\":\"0.1.0\"") != std::string::npos &&
                       diagRes.body.find("\"prefixTrieActive\":true") != std::string::npos);

        std::filesystem::remove(testPath);
        return histOk && diagOk;
    });

    // =========================================================================
    // 22. Stage 9: SQLite Persistence, Transactions & Migration Tests
    // =========================================================================
    std::cout << "\n--- 22. Stage 9: SQLite Persistence, Transactions & Migration ---\n";

    auto cleanupDb = [](const std::string& path) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
        std::filesystem::remove(path + "-wal", ec);
        std::filesystem::remove(path + "-shm", ec);
    };

    auto findSampleCsv = []() -> std::string {
        if (std::filesystem::exists("data/sample/questions.csv")) {
            return "data/sample/questions.csv";
        }
        if (std::filesystem::exists("../data/sample/questions.csv")) {
            return "../data/sample/questions.csv";
        }
        return "data/sample/questions.csv";
    };

    runTest("SQLite DB Initialization & File Creation", [&cleanupDb]() {
        std::string dbPath = "build/test_init.db";
        cleanupDb(dbPath);

        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            if (!repo.isOpen()) return false;
            if (repo.getDbPath() != dbPath) return false;
            repo.close();
        }

        bool fileCreated = std::filesystem::exists(dbPath);
        cleanupDb(dbPath);
        return fileCreated;
    });

    runTest("Schema Version & Migration Metadata", [&cleanupDb]() {
        std::string dbPath = "build/test_schema.db";
        cleanupDb(dbPath);

        int v = 0;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            v = repo.getSchemaVersion();
            repo.close();
        }

        cleanupDb(dbPath);
        return v == 3;
    });

    runTest("Reopening Existing Database Without Corruption", [&cleanupDb]() {
        std::string dbPath = "build/test_reopen.db";
        cleanupDb(dbPath);

        {
            codevault::persistence::SqliteQuestionRepository repo1(dbPath);
            codevault::models::Question q("Q-SQL-1", "Initial Problem", "Desc",
                                          codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy,
                                          "Google", codevault::models::Platform::LeetCode,
                                          "", codevault::models::Status::Todo, false, "",
                                          100, 100, 0, 0, 2, {"array"});
            repo1.save(q);
            repo1.close();
        }

        bool verified = false;
        {
            codevault::persistence::SqliteQuestionRepository repo2(dbPath);
            verified = (repo2.count() == 1 && repo2.exists("Q-SQL-1"));
            auto qOpt = repo2.findById("Q-SQL-1");
            if (!qOpt || qOpt->getTitle() != "Initial Problem") {
                verified = false;
            }
            repo2.close();
        }

        cleanupDb(dbPath);
        return verified;
    });

    runTest("Basic CRUD: Insert and FindById All 18 Domain Fields", [&cleanupDb]() {
        std::string dbPath = "build/test_crud.db";
        cleanupDb(dbPath);

        bool match = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            codevault::models::Question original(
                "Q-FULL-1", "Full Spec Problem", "Comprehensive description with quotes \"and\" commas, test",
                codevault::models::Topic::DynamicProgramming,
                codevault::models::Difficulty::Hard,
                "Meta",
                codevault::models::Platform::LeetCode,
                "https://leetcode.com/problems/full-spec",
                codevault::models::Status::Solved,
                true,
                "Key observation: optimal substructure",
                1234567890, 1234567990, 1234568000, 1234569000, 1,
                {"dp", "memoization", "hard-interview"},
                "author_42"
            );

            bool saved = repo.save(original);
            auto retrievedOpt = repo.findById("Q-FULL-1");

            if (saved && retrievedOpt.has_value()) {
                const auto& r = *retrievedOpt;
                match = (
                    r.getId() == "Q-FULL-1" &&
                    r.getTitle() == "Full Spec Problem" &&
                    r.getDescription() == "Comprehensive description with quotes \"and\" commas, test" &&
                    r.getTopic() == codevault::models::Topic::DynamicProgramming &&
                    r.getDifficulty() == codevault::models::Difficulty::Hard &&
                    r.getCompany() == "Meta" &&
                    r.getPlatform() == codevault::models::Platform::LeetCode &&
                    r.getSourceUrl() == "https://leetcode.com/problems/full-spec" &&
                    r.getStatus() == codevault::models::Status::Solved &&
                    r.isFavorite() == true &&
                    r.getNotes() == "Key observation: optimal substructure" &&
                    r.getCreatedAt() == 1234567890 &&
                    r.getUpdatedAt() == 1234567990 &&
                    r.getLastPracticedAt() == 1234568000 &&
                    r.getNextRevisionAt() == 1234569000 &&
                    r.getRevisionPriority() == 1 &&
                    r.getTags().size() == 3 &&
                    r.getTags()[0] == "dp" &&
                    r.getTags()[1] == "memoization" &&
                    r.getTags()[2] == "hard-interview" &&
                    r.getOwnerId() == "author_42"
                );
            }
            repo.close();
        }

        cleanupDb(dbPath);
        return match;
    });

    runTest("Basic CRUD: Exists Positive and Negative Lookup", [&cleanupDb]() {
        std::string dbPath = "build/test_exists.db";
        cleanupDb(dbPath);

        bool pos = false, neg = false, emptySafe = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            codevault::models::Question q("Q-EX-1", "Exists Problem", "",
                                          codevault::models::Topic::Trees,
                                          codevault::models::Difficulty::Medium,
                                          "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "",
                                          10, 10, 0, 0, 2, {});
            repo.save(q);

            pos = repo.exists("Q-EX-1");
            neg = !repo.exists("Q-DOES-NOT-EXIST");
            emptySafe = !repo.exists("");
            repo.close();
        }

        cleanupDb(dbPath);
        return pos && neg && emptySafe;
    });

    runTest("Basic CRUD: FindAll and Row Count Invariant", [&cleanupDb]() {
        std::string dbPath = "build/test_findall.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            for (int i = 1; i <= 5; ++i) {
                std::string id = "Q-FA-" + std::to_string(i);
                codevault::models::Question q(id, "Title " + std::to_string(i), "",
                                              codevault::models::Topic::Strings,
                                              codevault::models::Difficulty::Easy,
                                              "", codevault::models::Platform::Custom,
                                              "", codevault::models::Status::Todo, false, "",
                                              i * 100, i * 100, 0, 0, 2, {});
                repo.save(q);
            }

            auto all = repo.findAll();
            size_t cnt = repo.count();
            ok = (all.size() == 5 && cnt == 5 && all[0].getId() == "Q-FA-1" && all[4].getId() == "Q-FA-5");
            repo.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Basic CRUD: Upsert In-Place Without Row Duplication", [&cleanupDb]() {
        std::string dbPath = "build/test_upsert.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            codevault::models::Question q("Q-UP-1", "Original Title", "",
                                          codevault::models::Topic::Graphs,
                                          codevault::models::Difficulty::Medium,
                                          "Amazon", codevault::models::Platform::LeetCode,
                                          "", codevault::models::Status::Todo, false, "",
                                          100, 100, 0, 0, 2, {});
            repo.save(q);

            // Update title and status
            codevault::models::Question updated("Q-UP-1", "Updated Title", "Added notes",
                                                codevault::models::Topic::Graphs,
                                                codevault::models::Difficulty::Medium,
                                                "Amazon", codevault::models::Platform::LeetCode,
                                                "", codevault::models::Status::Solved, true, "New note",
                                                100, 200, 200, 300, 1, {"graph", "bfs"});
            repo.save(updated);

            size_t count = repo.count();
            auto retrieved = repo.findById("Q-UP-1");

            ok = (count == 1 && retrieved.has_value() &&
                  retrieved->getTitle() == "Updated Title" &&
                  retrieved->getStatus() == codevault::models::Status::Solved &&
                  retrieved->isFavorite() == true &&
                  retrieved->getUpdatedAt() == 200);
            repo.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Basic CRUD: Remove Question and Post-Condition Verification", [&cleanupDb]() {
        std::string dbPath = "build/test_remove.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            codevault::models::Question q("Q-REM-1", "Delete Me", "",
                                          codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy,
                                          "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "",
                                          100, 100, 0, 0, 2, {});
            repo.save(q);

            bool removed = repo.remove("Q-REM-1");
            bool secondRemove = repo.remove("Q-REM-1"); // already gone
            bool nonExistentRemove = repo.remove("Q-NONEXISTENT");

            size_t remaining = repo.count();
            auto found = repo.findById("Q-REM-1");

            ok = (removed && !secondRemove && !nonExistentRemove && remaining == 0 && !found.has_value());
            repo.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Constraints: Validation Rejection and Invalid Input Safety", [&cleanupDb]() {
        std::string dbPath = "build/test_invalid.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            // Invalid question (empty id and title)
            codevault::models::Question invalidQ;
            bool saveInvalid = repo.save(invalidQ);

            // Empty ID query
            auto emptyFind = repo.findById("");
            bool emptyExists = repo.exists("");
            bool emptyRemove = repo.remove("");

            ok = (!saveInvalid && !emptyFind.has_value() && !emptyExists && !emptyRemove && repo.count() == 0);
            repo.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Persistence: Data Integrity Across Database Close and Reopen", [&cleanupDb]() {
        std::string dbPath = "build/test_reopen_integrity.db";
        cleanupDb(dbPath);

        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            codevault::models::Question q(
                "Q-PERSIST", "Survives Restart", "Restart notes",
                codevault::models::Topic::DynamicProgramming,
                codevault::models::Difficulty::Hard,
                "Google", codevault::models::Platform::LeetCode,
                "https://leetcode.com", codevault::models::Status::Solved,
                true, "Notes here", 1000, 2000, 1500, 3500, 1, {"dp"}
            );
            repo.save(q);
            repo.close();
        }

        // Reopen database from disk
        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repoReopened(dbPath);
            auto opt = repoReopened.findById("Q-PERSIST");

            ok = (opt.has_value() &&
                  opt->getTitle() == "Survives Restart" &&
                  opt->getLastPracticedAt() == 1500 &&
                  opt->getNextRevisionAt() == 3500 &&
                  opt->getRevisionPriority() == 1 &&
                  opt->isFavorite());
            repoReopened.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Transactions: Atomic Commit with executeTransaction", [&cleanupDb]() {
        std::string dbPath = "build/test_tx_commit.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);

            bool txResult = repo.executeTransaction([&]() -> bool {
                codevault::models::Question q1("Q-TX-1", "Tx Prob 1", "", codevault::models::Topic::Arrays,
                                               codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                               "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {});
                codevault::models::Question q2("Q-TX-2", "Tx Prob 2", "", codevault::models::Topic::Strings,
                                               codevault::models::Difficulty::Medium, "", codevault::models::Platform::Custom,
                                               "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {});
                return repo.save(q1) && repo.save(q2);
            });

            size_t count = repo.count();
            bool q1Exists = repo.exists("Q-TX-1");
            bool q2Exists = repo.exists("Q-TX-2");

            ok = (txResult && count == 2 && q1Exists && q2Exists);
            repo.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Transactions: Atomic Rollback on Failure Leaves DB Unchanged", [&cleanupDb]() {
        std::string dbPath = "build/test_tx_rollback.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);

            // Pre-populate with baseline question
            codevault::models::Question initial("Q-BASE", "Base Problem", "", codevault::models::Topic::Arrays,
                                                codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                                "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {});
            repo.save(initial);
            size_t countBefore = repo.count();

            // Transaction that intentionally fails halfway
            bool txResult = repo.executeTransaction([&]() -> bool {
                codevault::models::Question q1("Q-ROLL-1", "Rollback 1", "", codevault::models::Topic::Arrays,
                                               codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                               "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {});
                repo.save(q1);

                // Abort transaction intentionally
                return false;
            });

            size_t countAfter = repo.count();
            bool rolledBackItemExists = repo.exists("Q-ROLL-1");
            bool baseStillExists = repo.exists("Q-BASE");

            ok = (!txResult && countAfter == countBefore && !rolledBackItemExists && baseStillExists);
            repo.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Transactions: Exception Safety Automatically Rolls Back", [&cleanupDb]() {
        std::string dbPath = "build/test_tx_exception.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);

            bool exceptionCaught = false;
            try {
                repo.executeTransaction([&]() -> bool {
                    codevault::models::Question q1("Q-EXCP-1", "Ex Prob", "", codevault::models::Topic::Arrays,
                                                   codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                                   "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {});
                    repo.save(q1);
                    throw std::runtime_error("Simulated transient failure");
                });
            } catch (const std::runtime_error&) {
                exceptionCaught = true;
            }

            size_t count = repo.count();
            bool exists = repo.exists("Q-EXCP-1");

            ok = (exceptionCaught && count == 0 && !exists);
            repo.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("CSV Migration: Ingestion and Count Verification from Sample CSV", [&cleanupDb, &findSampleCsv]() {
        std::string dbPath = "build/test_migrate_count.db";
        cleanupDb(dbPath);

        bool success = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            auto report = codevault::persistence::MigrationService::migrateCsvToSqlite(
                findSampleCsv(),
                repo,
                false
            );

            success = (report.success && report.importedCount >= 8 && repo.count() == report.importedCount);
            repo.close();
        }

        cleanupDb(dbPath);
        return success;
    });

    runTest("CSV Migration: Field Integrity & Tags Preservation", [&cleanupDb, &findSampleCsv]() {
        std::string dbPath = "build/test_migrate_fields.db";
        cleanupDb(dbPath);

        bool verified = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            codevault::persistence::MigrationService::migrateCsvToSqlite(
                findSampleCsv(),
                repo,
                false
            );

            auto twoSumOpt = repo.findById("Q-1001");
            if (twoSumOpt.has_value()) {
                const auto& q = *twoSumOpt;
                verified = (q.getTitle() == "Two Sum" &&
                            q.getTopic() == codevault::models::Topic::Arrays &&
                            q.getDifficulty() == codevault::models::Difficulty::Easy &&
                            q.getStatus() == codevault::models::Status::Solved &&
                            !q.getTags().empty());
            }
            repo.close();
        }

        cleanupDb(dbPath);
        return verified;
    });

    runTest("CSV Migration: Backup File Generation", [&cleanupDb]() {
        std::string testCsv = "build/test_backup_source.csv";
        std::string testBak = "build/test_backup_source.csv.bak";
        std::error_code ec;
        std::filesystem::remove(testCsv, ec);
        std::filesystem::remove(testBak, ec);

        // Create sample CSV file
        {
            std::ofstream out(testCsv);
            out << "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\n"
                << "Q-BAK-1,Backup Test,Desc,Arrays,Easy,Google,LeetCode,,Todo,false,,10,10,0,0,2,tag,local\n";
        }

        std::string dbPath = "build/test_backup.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            auto report = codevault::persistence::MigrationService::migrateCsvToSqlite(testCsv, repo);
            bool bakCreated = std::filesystem::exists(testBak);
            ok = (report.success && bakCreated);
            repo.close();
        }

        std::filesystem::remove(testCsv, ec);
        std::filesystem::remove(testBak, ec);
        cleanupDb(dbPath);
        return ok;
    });

    runTest("CSV Migration: Tolerance of Malformed Rows and Warning Reporting", [&cleanupDb]() {
        std::string testCsv = "build/test_malformed.csv";
        std::error_code ec;
        std::filesystem::remove(testCsv, ec);

        {
            std::ofstream out(testCsv);
            out << "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\n"
                << "Q-OK-1,Valid One,Desc,Arrays,Easy,Google,LeetCode,,Todo,false,,10,10,0,0,2,tag,local\n"
                << "corrupt,truncated,row\n"
                << ",Empty ID Row,,Arrays,Easy,,,,Todo,false,,10,10,0,0,2,,local\n"
                << "Q-OK-2,Valid Two,Desc,Strings,Medium,Meta,LeetCode,,Todo,false,,10,10,0,0,2,tag,local\n";
        }

        std::string dbPath = "build/test_malformed.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            auto report = codevault::persistence::MigrationService::migrateCsvToSqlite(testCsv, repo);

            ok = (report.success &&
                  report.importedCount == 2 &&
                  report.malformedRowsCount >= 1 &&
                  repo.count() == 2 &&
                  repo.exists("Q-OK-1") &&
                  repo.exists("Q-OK-2"));
            repo.close();
        }

        std::filesystem::remove(testCsv, ec);
        std::filesystem::remove(testCsv + ".bak", ec);
        cleanupDb(dbPath);
        return ok;
    });

    runTest("CSV Migration: Idempotence and Re-run Duplicate Protection", [&cleanupDb, &findSampleCsv]() {
        std::string dbPath = "build/test_idempotent.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);

            // First migration
            auto report1 = codevault::persistence::MigrationService::migrateCsvToSqlite(
                findSampleCsv(),
                repo,
                false
            );

            size_t countAfterFirst = repo.count();

            // Second migration without force
            auto report2 = codevault::persistence::MigrationService::migrateCsvToSqlite(
                findSampleCsv(),
                repo,
                false
            );

            size_t countAfterSecond = repo.count();

            ok = (report1.success && report2.success &&
                  report2.skippedExistingCount == countAfterFirst &&
                  countAfterSecond == countAfterFirst);
            repo.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Service Integration: QuestionService, RevisionService & PracticeService on SQLite", [&cleanupDb]() {
        std::string dbPath = "build/test_services_sqlite.db";
        cleanupDb(dbPath);

        bool statusOk = false;
        bool statsOk = false;
        bool practiceAttemptOk = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::models::Question q(
                "Q-SRV-1", "Services SQLite Test", "Desc",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy,
                "Amazon", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Unsolved, false, "",
                1000, 1000, 0, 0, 2, {}
            );
            qs->createQuestion(q);

            // Practice session
            ps->startSession({q.getId()});
            auto practiceAttempt = ps->recordPracticeAttempt(codevault::models::PracticeVerdict::Solved);

            // Check that QuestionService and repo received updated status and spaced repetition timestamps
            auto updated = qs->getQuestionById("Q-SRV-1");
            statusOk = (updated.has_value() &&
                        updated->getStatus() == codevault::models::Status::Solved &&
                        updated->getNextRevisionAt() > 1000);

            // Check Statistics
            auto snap = stats->getDashboardSnapshot();
            statsOk = (snap.overall.totalQuestions == 1 && snap.overall.solvedCount == 1);
            practiceAttemptOk = practiceAttempt.has_value();

            repo->close();
        }

        cleanupDb(dbPath);
        return statusOk && statsOk && practiceAttemptOk;
    });

    runTest("HTTP API Integration: Settings Diagnostics Reports SQLite Engine Metadata", [&cleanupDb]() {
        std::string dbPath = "build/test_diag_sqlite.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, 8899);

            codevault::app::HttpRequest req;
            req.method = "GET";
            req.path = "/api/settings/diagnostics";

            auto res = server.handleRequest(req);

            ok = (res.statusCode == 200 &&
                  res.body.find("\"storageType\":\"sqlite\"") != std::string::npos &&
                  res.body.find("\"schemaVersion\":3") != std::string::npos &&
                  res.body.find("\"databaseAvailable\":true") != std::string::npos);

            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    // =========================================================================
    // 23. Stage 9.2: Multi-User Data Foundation & Owner-Scoped Data Architecture
    // =========================================================================
    std::cout << "\n--- 23. Stage 9.2: Multi-User Data Foundation & Owner-Scoped Architecture ---\n";

    runTest("User Domain Model: Field Validation, Getters & Defaults", []() {
        codevault::models::User u("usr_1", "alice", "Alice Smith", "alice@example.com", 1000, 2000, true);
        bool validUser = u.isValid() &&
                         u.getId() == "usr_1" &&
                         u.getUsername() == "alice" &&
                         u.getDisplayName() == "Alice Smith" &&
                         u.getEmail() == "alice@example.com" &&
                         u.getCreatedAt() == 1000 &&
                         u.getUpdatedAt() == 2000 &&
                         u.isActive();

        codevault::models::User emptyId("", "alice", "Alice");
        bool invalidId = !emptyId.isValid();

        codevault::models::User emptyUsername("usr_2", "", "Bob");
        bool invalidUsername = !emptyUsername.isValid();

        return validUser && invalidId && invalidUsername;
    });

    runTest("User Repository: Create and FindById", [&cleanupDb]() {
        std::string dbPath = "build/test_user_repo.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteUserRepository userRepo(dbPath);
            codevault::models::User u("usr_101", "bob", "Bob Dylan", "bob@example.com", 100, 100, true);
            bool created = userRepo.createUser(u);

            auto found = userRepo.findById("usr_101");
            auto notFound = userRepo.findById("usr_unknown");

            ok = created && found.has_value() &&
                 found->getId() == "usr_101" &&
                 found->getUsername() == "bob" &&
                 found->getDisplayName() == "Bob Dylan" &&
                 !notFound.has_value();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("User Repository: FindByUsername and FindByEmail", [&cleanupDb]() {
        std::string dbPath = "build/test_user_find.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteUserRepository userRepo(dbPath);
            codevault::models::User u("usr_charlie", "charlie", "Charlie Brown", "charlie@peanuts.com");
            userRepo.createUser(u);

            auto byUser = userRepo.findByUsername("charlie");
            auto byEmail = userRepo.findByEmail("charlie@peanuts.com");
            auto notFound = userRepo.findByUsername("nobody");

            ok = byUser.has_value() && byUser->getId() == "usr_charlie" &&
                 byEmail.has_value() && byEmail->getId() == "usr_charlie" &&
                 !notFound.has_value();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("User Repository: Duplicate Username Rejection", [&cleanupDb]() {
        std::string dbPath = "build/test_user_dup.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteUserRepository userRepo(dbPath);
            codevault::models::User u1("usr_1", "johndoe", "John 1");
            codevault::models::User u2("usr_2", "johndoe", "John 2");

            bool first = userRepo.createUser(u1);
            bool second = userRepo.createUser(u2); // Duplicate username should fail

            ok = first && !second;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("User Repository: Update User Profile", [&cleanupDb]() {
        std::string dbPath = "build/test_user_update.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteUserRepository userRepo(dbPath);
            codevault::models::User u("usr_up", "david", "David Bowie", "david@music.com");
            userRepo.createUser(u);

            codevault::models::User updated("usr_up", "david", "David Starman", "starman@music.com", 100, 500, true);
            bool updatedOk = userRepo.updateUser(updated);

            auto retrieved = userRepo.findById("usr_up");
            ok = updatedOk && retrieved.has_value() &&
                 retrieved->getDisplayName() == "David Starman" &&
                 retrieved->getEmail() == "starman@music.com";
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("User Repository: Deactivate User", [&cleanupDb]() {
        std::string dbPath = "build/test_user_deact.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            codevault::persistence::SqliteUserRepository userRepo(dbPath);
            codevault::models::User u("usr_deact", "eva", "Eva Green");
            userRepo.createUser(u);

            bool deactOk = userRepo.deactivateUser("usr_deact");
            auto retrieved = userRepo.findById("usr_deact");

            ok = deactOk && retrieved.has_value() && !retrieved->isActive();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Schema Migration v1 to v2: Upgrades Schema and Preserves Existing Questions", [&cleanupDb]() {
        std::string dbPath = "build/test_migrate_v1_v2.db";
        cleanupDb(dbPath);

        // 1. Manually create a Schema v1 database (mimicking Stage 9.1 before Stage 9.2)
        {
            sqlite3* rawDb = nullptr;
            sqlite3_open(dbPath.c_str(), &rawDb);
            const char* v1Schema =
                "CREATE TABLE schema_version (version INTEGER PRIMARY KEY, applied_at INTEGER, description TEXT);"
                "INSERT INTO schema_version VALUES (1, 1000, 'Schema v1');"
                "CREATE TABLE questions ("
                "  id TEXT PRIMARY KEY NOT NULL,"
                "  title TEXT NOT NULL,"
                "  description TEXT DEFAULT '',"
                "  topic TEXT NOT NULL,"
                "  difficulty TEXT NOT NULL,"
                "  company TEXT DEFAULT '',"
                "  platform TEXT DEFAULT '',"
                "  source_url TEXT DEFAULT '',"
                "  status TEXT NOT NULL,"
                "  is_favorite INTEGER DEFAULT 0,"
                "  notes TEXT DEFAULT '',"
                "  created_at INTEGER NOT NULL,"
                "  updated_at INTEGER NOT NULL,"
                "  last_practiced_at INTEGER DEFAULT 0,"
                "  next_revision_at INTEGER DEFAULT 0,"
                "  revision_priority INTEGER DEFAULT 2,"
                "  tags TEXT DEFAULT '',"
                "  owner_id TEXT DEFAULT 'local_user'"
                ");"
                "INSERT INTO questions (id, title, topic, difficulty, status, created_at, updated_at, owner_id) "
                "VALUES ('Q-V1-1', 'V1 Preserved Question', 'Arrays', 'Easy', 'Todo', 100, 100, 'local_user');";
            char* err = nullptr;
            sqlite3_exec(rawDb, v1Schema, nullptr, nullptr, &err);
            if (err) sqlite3_free(err);
            sqlite3_close(rawDb);
        }

        // 2. Open with SqliteQuestionRepository, which executes automatic migration to Schema v3
        bool migrated = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            int version = repo.getSchemaVersion();
            auto preservedOpt = repo.findById("Q-V1-1");

            migrated = (version == 3 && preservedOpt.has_value() &&
                        preservedOpt->getTitle() == "V1 Preserved Question" &&
                        preservedOpt->getOwnerId() == "local_user");
            repo.close();
        }

        cleanupDb(dbPath);
        return migrated;
    });

    runTest("Migration Idempotence: Reopening Database Does Not Duplicate or Alter Schema v3", [&cleanupDb]() {
        std::string dbPath = "build/test_migrate_idempotent.db";
        cleanupDb(dbPath);

        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            repo.close();
        }

        bool ok = false;
        {
            codevault::persistence::SqliteQuestionRepository repo2(dbPath);
            int v2 = repo2.getSchemaVersion();

            codevault::persistence::SqliteUserRepository userRepo(dbPath);
            auto localUser = userRepo.findById("local_user");

            ok = (v2 == 3 && localUser.has_value() && localUser->getUsername() == "local_user");
            repo2.close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Isolation: FindById (User A cannot access User B's question)", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_find.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            userRepo->createUser(codevault::models::User("usr_a", "alice", "Alice"));
            userRepo->createUser(codevault::models::User("usr_b", "bob", "Bob"));

            codevault::models::Question qA("Q-A1", "Alice Secret", "Desc A", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "Google", codevault::models::Platform::LeetCode,
                                          "", codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, "usr_a");
            codevault::models::Question qB("Q-B1", "Bob Secret", "Desc B", codevault::models::Topic::Trees,
                                          codevault::models::Difficulty::Medium, "Amazon", codevault::models::Platform::LeetCode,
                                          "", codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, "usr_b");

            repo->saveForOwner(qA, "usr_a");
            repo->saveForOwner(qB, "usr_b");

            auto providerA = std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a");
            auto qsA = std::make_shared<codevault::services::QuestionService>(repo, nullptr, nullptr, providerA);

            auto providerB = std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_b");
            auto qsB = std::make_shared<codevault::services::QuestionService>(repo, nullptr, nullptr, providerB);

            // User A can find Q-A1 but NOT Q-B1
            bool aCanFindA = qsA->getQuestionById("Q-A1").has_value();
            bool aCannotFindB = !qsA->getQuestionById("Q-B1").has_value();

            // User B can find Q-B1 but NOT Q-A1
            bool bCanFindB = qsB->getQuestionById("Q-B1").has_value();
            bool bCannotFindA = !qsB->getQuestionById("Q-A1").has_value();

            ok = aCanFindA && aCannotFindB && bCanFindB && bCannotFindA;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Isolation: List & Count (User A only sees their own questions)", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_list.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            userRepo->createUser(codevault::models::User("usr_a", "alice", "Alice"));
            userRepo->createUser(codevault::models::User("usr_b", "bob", "Bob"));

            for (int i = 1; i <= 3; ++i) {
                codevault::models::Question q("QA-" + std::to_string(i), "Alice " + std::to_string(i), "",
                    codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "",
                    codevault::models::Platform::Custom, "", codevault::models::Status::Todo, false, "",
                    10, 10, 0, 0, 2, {}, "usr_a");
                repo->saveForOwner(q, "usr_a");
            }
            for (int i = 1; i <= 2; ++i) {
                codevault::models::Question q("QB-" + std::to_string(i), "Bob " + std::to_string(i), "",
                    codevault::models::Topic::Strings, codevault::models::Difficulty::Medium, "",
                    codevault::models::Platform::Custom, "", codevault::models::Status::Todo, false, "",
                    10, 10, 0, 0, 2, {}, "usr_b");
                repo->saveForOwner(q, "usr_b");
            }

            auto qsA = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a"));
            auto qsB = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_b"));

            auto allA = qsA->getAllQuestions();
            auto allB = qsB->getAllQuestions();

            ok = (allA.size() == 3 && qsA->getQuestionCount() == 3 &&
                  allB.size() == 2 && qsB->getQuestionCount() == 2);
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Isolation: Update (User A cannot overwrite User B's question)", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_update.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            codevault::models::Question qB("QB-1", "Bob Original", "", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {}, "usr_b");
            repo->saveForOwner(qB, "usr_b");

            auto qsA = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a"));

            // User A attempts to update Bob's question
            codevault::models::Question hacked("QB-1", "Hacked Title", "", codevault::models::Topic::Arrays,
                                               codevault::models::Difficulty::Hard, "", codevault::models::Platform::Custom,
                                               "", codevault::models::Status::Solved, true, "", 10, 20, 0, 0, 1, {}, "usr_a");

            auto res = qsA->updateQuestion(hacked);

            // Direct repository check: Bob's question must remain intact
            auto bobQ = repo->findByIdForOwner("QB-1", "usr_b");

            ok = (!res.isValid && bobQ.has_value() && bobQ->getTitle() == "Bob Original" &&
                  bobQ->getOwnerId() == "usr_b");
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Isolation: Delete (User A cannot delete User B's question)", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_delete.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            codevault::models::Question qB("QB-DEL", "Bob Deletable", "", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {}, "usr_b");
            repo->saveForOwner(qB, "usr_b");

            auto qsA = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a"));

            bool deletedByA = qsA->deleteQuestion("QB-DEL");
            bool stillExistsForB = repo->existsForOwner("QB-DEL", "usr_b");

            ok = (!deletedByA && stillExistsForB);
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Isolation: Search Autocomplete Trie (User A Trie isolates titles)", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_search.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            codevault::models::Question qA("QA-TRIE", "Binary Search Tree", "", codevault::models::Topic::Trees,
                                          codevault::models::Difficulty::Medium, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {}, "usr_a");
            codevault::models::Question qB("QB-TRIE", "Binary Search Array", "", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {}, "usr_b");
            repo->saveForOwner(qA, "usr_a");
            repo->saveForOwner(qB, "usr_b");

            auto qsA = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a"));
            auto qsB = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_b"));

            auto resultsA = qsA->searchQuestionsByTitlePrefix("Binary Search");
            auto resultsB = qsB->searchQuestionsByTitlePrefix("Binary Search");

            ok = (resultsA.size() == 1 && resultsA[0].getId() == "QA-TRIE" &&
                  resultsB.size() == 1 && resultsB[0].getId() == "QB-TRIE");
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Isolation: Topic and Criteria Filtering", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_filter.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            codevault::models::Question qA("QA-DP", "DP Problem A", "", codevault::models::Topic::DynamicProgramming,
                                          codevault::models::Difficulty::Hard, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {}, "usr_a");
            codevault::models::Question qB("QB-DP", "DP Problem B", "", codevault::models::Topic::DynamicProgramming,
                                          codevault::models::Difficulty::Hard, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {}, "usr_b");
            repo->saveForOwner(qA, "usr_a");
            repo->saveForOwner(qB, "usr_b");

            auto qsA = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a"));

            auto allA = qsA->getAllQuestions();
            size_t dpCountA = 0;
            for (const auto& q : allA) {
                if (q.getTopic() == codevault::models::Topic::DynamicProgramming) ++dpCountA;
            }

            ok = (dpCountA == 1);
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Isolation: Statistics & Dashboard Calculations", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_stats.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            // User A solved 1 problem
            codevault::models::Question qA("QA-SOLV", "Solved A", "", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Solved, true, "", 10, 10, 0, 0, 2, {}, "usr_a");
            // User B has 2 unsolved problems
            codevault::models::Question qB1("QB-UN1", "Unsolved B1", "", codevault::models::Topic::Arrays,
                                           codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                           "", codevault::models::Status::Unsolved, false, "", 10, 10, 0, 0, 2, {}, "usr_b");
            codevault::models::Question qB2("QB-UN2", "Unsolved B2", "", codevault::models::Topic::Arrays,
                                           codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                           "", codevault::models::Status::Unsolved, false, "", 10, 10, 0, 0, 2, {}, "usr_b");
            repo->saveForOwner(qA, "usr_a");
            repo->saveForOwner(qB1, "usr_b");
            repo->saveForOwner(qB2, "usr_b");

            auto qsA = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a"));
            auto statsA = std::make_shared<codevault::services::StatisticsService>(qsA, nullptr);

            auto qsB = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_b"));
            auto statsB = std::make_shared<codevault::services::StatisticsService>(qsB, nullptr);

            auto snapA = statsA->getDashboardSnapshot(1000);
            auto snapB = statsB->getDashboardSnapshot(1000);

            // User A: 1 total, 1 solved, 100% completion, 1 favorite
            bool aOk = (snapA.overall.totalQuestions == 1 &&
                        snapA.overall.solvedCount == 1 &&
                        snapA.overall.completionPercentage == 100.0 &&
                        snapA.overall.favoriteCount == 1);

            // User B: 2 total, 0 solved, 0% completion, 0 favorite
            bool bOk = (snapB.overall.totalQuestions == 2 &&
                        snapB.overall.solvedCount == 0 &&
                        snapB.overall.completionPercentage == 0.0 &&
                        snapB.overall.favoriteCount == 0);

            ok = aOk && bOk;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Isolation: Spaced Revision Queue Isolation", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_rev.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            // User A has question scheduled at t=500 (due at t=1000)
            codevault::models::Question qA("QA-REV", "Alice Revision", "", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::InProgress, false, "", 10, 10, 100, 500, 1, {}, "usr_a");
            // User B has question unscheduled
            codevault::models::Question qB("QB-REV", "Bob Question", "", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Unsolved, false, "", 10, 10, 0, 0, 2, {}, "usr_b");
            repo->saveForOwner(qA, "usr_a");
            repo->saveForOwner(qB, "usr_b");

            auto revA = std::make_shared<codevault::services::RevisionService>();
            auto qsA = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, revA, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a"));

            auto revB = std::make_shared<codevault::services::RevisionService>();
            auto qsB = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, revB, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_b"));

            auto dueA = revA->getDueQuestions(1000);
            auto dueB = revB->getDueQuestions(1000);

            ok = (dueA.size() == 1 && dueA[0].questionId == "QA-REV" && dueB.empty());
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Isolation: Practice Session Queue Rejects Other User Questions", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_practice.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            codevault::models::Question qB("QB-PRAC", "Bob Problem", "", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Unsolved, false, "", 10, 10, 0, 0, 2, {}, "usr_b");
            codevault::models::Question qA("QA-PRAC", "Alice Problem", "", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Unsolved, false, "", 10, 10, 0, 0, 2, {}, "usr_a");
            repo->saveForOwner(qB, "usr_b");
            repo->saveForOwner(qA, "usr_a");

            auto qsA = std::make_shared<codevault::services::QuestionService>(
                repo, nullptr, nullptr, std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a"));
            auto psA = std::make_shared<codevault::services::PracticeService>(qsA, nullptr);

            // Alice attempts to enqueue Bob's question QB-PRAC
            psA->startSession({"QB-PRAC"});
            bool bobBlockedFromAliceSession = !psA->hasQuestions();

            // Alice enqueues her own question QA-PRAC
            psA->startSession({"QA-PRAC"});
            bool aliceOwnQuestionEnqueued = psA->hasQuestions() && psA->getCurrentQuestion() == "QA-PRAC";

            ok = bobBlockedFromAliceSession && aliceOwnQuestionEnqueued;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Multi-User Dynamic Scoping: setCurrentUserProvider Switches State Live", [&cleanupDb]() {
        std::string dbPath = "build/test_iso_switch.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            codevault::models::Question qA("QA-SW", "Alice Switched", "", codevault::models::Topic::Arrays,
                                          codevault::models::Difficulty::Easy, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {}, "usr_a");
            codevault::models::Question qB("QB-SW", "Bob Switched", "", codevault::models::Topic::Strings,
                                          codevault::models::Difficulty::Hard, "", codevault::models::Platform::Custom,
                                          "", codevault::models::Status::Todo, false, "", 10, 10, 0, 0, 2, {}, "usr_b");
            repo->saveForOwner(qA, "usr_a");
            repo->saveForOwner(qB, "usr_b");

            auto provider = std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_a");
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, nullptr, provider);

            // Currently usr_a
            bool scopeA = (qs->getQuestionCount() == 1 && qs->questionExists("QA-SW") && !qs->questionExists("QB-SW"));

            // Switch to usr_b
            qs->setCurrentUserProvider(std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_b"));

            bool scopeB = (qs->getQuestionCount() == 1 && qs->questionExists("QB-SW") && !qs->questionExists("QA-SW"));

            ok = scopeA && scopeB;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    // =========================================================================
    // 24. Stage 9.3: Authentication & Session Foundation
    // =========================================================================
    std::cout << "\n--- 24. Stage 9.3: Authentication & Session Foundation ---\n";

    runTest("Crypto: Argon2id Password Hashing & Verification", []() {
        std::string rawPass = "SecretVault#2026";
        std::string hash1 = codevault::utils::crypto::hashPassword(rawPass);
        std::string hash2 = codevault::utils::crypto::hashPassword(rawPass);

        // 1. Argon2id encoded string format
        bool formatOk = (hash1.rfind("$argon2id$v=19$", 0) == 0);
        // 2. Salt uniqueness: two hashes of the same password produce distinct salt/hash outputs
        bool saltUnique = (hash1 != hash2);
        // 3. Verification succeeds with correct password
        bool verifyCorrect1 = codevault::utils::crypto::verifyPassword(rawPass, hash1);
        bool verifyCorrect2 = codevault::utils::crypto::verifyPassword(rawPass, hash2);
        // 4. Verification fails with incorrect password
        bool verifyWrong = !codevault::utils::crypto::verifyPassword("WrongPassword123!", hash1);
        // 5. Verification fails with empty password
        bool verifyEmpty = !codevault::utils::crypto::verifyPassword("", hash1);

        return formatOk && saltUnique && verifyCorrect1 && verifyCorrect2 && verifyWrong && verifyEmpty;
    });

    runTest("Crypto: Secure Token Generation & Token Hashing", []() {
        std::string token1 = codevault::utils::crypto::generateSecureToken(32);
        std::string token2 = codevault::utils::crypto::generateSecureToken(32);

        // 32 bytes = 64 hex characters
        bool lengthOk = (token1.length() == 64 && token2.length() == 64);
        // Tokens must be unpredictable and distinct
        bool distinct = (token1 != token2);

        // Token hash is deterministic
        std::string hashA = codevault::utils::crypto::hashToken(token1);
        std::string hashB = codevault::utils::crypto::hashToken(token1);
        std::string hashC = codevault::utils::crypto::hashToken(token2);

        bool hashDeterministic = (hashA == hashB && hashA != hashC && hashA.length() == 64);

        return lengthOk && distinct && hashDeterministic;
    });

    runTest("User Registration: Valid Profile & Password Storage", [&cleanupDb]() {
        std::string dbPath = "build/test_auth_reg.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo);

            auto result = auth.registerUser("alice_dev", "Alice Smith", "alice@codevault.dev", "SecurePass#123");
            if (result.success && result.user.has_value() && !result.sessionToken.empty()) {
                auto credsOpt = userRepo->getCredentials(result.user->getId());
                bool credsFound = credsOpt.has_value();
                bool hashStored = credsFound && !credsOpt->getPasswordHash().empty();
                bool plainNotStored = credsFound && credsOpt->getPasswordHash() != "SecurePass#123";
                bool verifyPass = credsFound && codevault::utils::crypto::verifyPassword("SecurePass#123", credsOpt->getPasswordHash());

                ok = (result.user->getUsername() == "alice_dev" &&
                      result.user->getDisplayName() == "Alice Smith" &&
                      credsFound && hashStored && plainNotStored && verifyPass);
            }
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("User Registration: Validation (Username, Password, and Unique Constraints)", [&cleanupDb]() {
        std::string dbPath = "build/test_auth_validation.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo);

            // Valid initial user
            auto validRes = auth.registerUser("bob_dev", "Bob Builder", "bob@builder.com", "Password123!");
            // 1. Duplicate username
            auto dupUserRes = auth.registerUser("bob_dev", "Bob Twin", "bob2@builder.com", "Password123!");
            // 2. Duplicate email
            auto dupEmailRes = auth.registerUser("bob_clone", "Bob Clone", "bob@builder.com", "Password123!");
            // 3. Short username (< 3 chars)
            auto shortUserRes = auth.registerUser("bo", "Short Bob", "", "Password123!");
            // 4. Short password (< 8 chars)
            auto shortPassRes = auth.registerUser("charlie_dev", "Charlie", "", "short");

            ok = (validRes.success &&
                  !dupUserRes.success && dupUserRes.errorMessage.find("Username already taken") != std::string::npos &&
                  !dupEmailRes.success && dupEmailRes.errorMessage.find("Email already registered") != std::string::npos &&
                  !shortUserRes.success &&
                  !shortPassRes.success);
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Login: Valid Credentials & Session Token Issuance", [&cleanupDb]() {
        std::string dbPath = "build/test_auth_login.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo);

            auth.registerUser("david_coder", "David Coder", "david@coder.io", "Pass@123456");

            // Login with username
            auto loginUserRes = auth.login("david_coder", "Pass@123456");
            // Login with email
            auto loginEmailRes = auth.login("david@coder.io", "Pass@123456");

            bool userLoginOk = loginUserRes.success && !loginUserRes.sessionToken.empty() && loginUserRes.user->getUsername() == "david_coder";
            bool emailLoginOk = loginEmailRes.success && !loginEmailRes.sessionToken.empty() && loginEmailRes.user->getUsername() == "david_coder";

            // Verify sessions created in DB
            auto authUser1 = auth.authenticateToken(loginUserRes.sessionToken);
            auto authUser2 = auth.authenticateToken(loginEmailRes.sessionToken);

            ok = (userLoginOk && emailLoginOk && authUser1.has_value() && authUser2.has_value() &&
                  authUser1->getId() == loginUserRes.user->getId());
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Login: Rejection of Invalid Passwords, Nonexistent Users & Inactive Accounts", [&cleanupDb]() {
        std::string dbPath = "build/test_auth_reject.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo);

            auth.registerUser("eva_dev", "Eva Dev", "eva@dev.org", "CorrectPassword#1");

            // 1. Wrong password
            auto wrongPass = auth.login("eva_dev", "IncorrectPass!");
            // 2. Nonexistent user
            auto nonExistent = auth.login("nobody_exists", "SomePass#123");
            // 3. Inactive account
            auto evaUser = userRepo->findByUsername("eva_dev");
            evaUser->setActive(false);
            userRepo->updateUser(*evaUser);
            auto inactiveLogin = auth.login("eva_dev", "CorrectPassword#1");

            // Generic error messages prevent account enumeration
            ok = (!wrongPass.success && wrongPass.errorMessage == "Invalid username or password" &&
                  !nonExistent.success && nonExistent.errorMessage == "Invalid username or password" &&
                  !inactiveLogin.success && inactiveLogin.errorMessage == "Account is disabled");
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Session: Server-Side Lookup & Last Seen Timestamp Updates", [&cleanupDb]() {
        std::string dbPath = "build/test_auth_session.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo);

            auto reg = auth.registerUser("frank_dev", "Frank", "frank@dev.net", "Secret#9999");
            std::string token = reg.sessionToken;

            std::string tokenHash = codevault::utils::crypto::hashToken(token);
            auto sessionBefore = sessionRepo->findByTokenHash(tokenHash);
            int64_t seenBefore = sessionBefore ? sessionBefore->getLastSeenAt() : 0;

            // Authenticate token updates last seen
            auto authedUser = auth.authenticateToken(token);
            auto sessionAfter = sessionRepo->findByTokenHash(tokenHash);
            int64_t seenAfter = sessionAfter ? sessionAfter->getLastSeenAt() : 0;

            ok = (authedUser.has_value() && authedUser->getUsername() == "frank_dev" &&
                  sessionAfter.has_value() && seenAfter >= seenBefore);
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Session: Invalidation via Expiration, Revocation & Logout", [&cleanupDb]() {
        std::string dbPath = "build/test_auth_invalidation.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo, 3600); // 1 hour

            auto reg = auth.registerUser("grace_dev", "Grace", "grace@dev.net", "Password#Grace1");
            std::string token = reg.sessionToken;

            // 1. Initial auth succeeds
            bool initialOk = auth.authenticateToken(token).has_value();

            // 2. Tampered token fails
            bool tamperedFails = !auth.authenticateToken(token + "tampered").has_value();

            // 3. Logout revokes session
            auth.logout(token);
            bool afterLogoutFails = !auth.authenticateToken(token).has_value();

            // 4. Verify session in DB has revoked_at set
            std::string tokenHash = codevault::utils::crypto::hashToken(token);
            auto sess = sessionRepo->findByTokenHash(tokenHash);
            bool revokedRecorded = (sess.has_value() && sess->getRevokedAt() > 0);

            // 5. Expired session is rejected
            std::string rawExpiredToken = codevault::utils::crypto::generateSecureToken(32);
            std::string expiredHash = codevault::utils::crypto::hashToken(rawExpiredToken);
            codevault::models::Session expiredSess("ses_expired", reg.user->getId(), expiredHash, 1000, 2000, 0, 1000);
            sessionRepo->createSession(expiredSess);
            bool expiredRejected = !auth.authenticateToken(rawExpiredToken).has_value();

            ok = initialOk && tamperedFails && afterLogoutFails && revokedRecorded && expiredRejected;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP Auth Endpoints: Register, Login, Me, Logout & Cookie Management", [&cleanupDb]() {
        std::string dbPath = "build/test_http_auth.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);

            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8911);

            // 1. Register
            codevault::app::HttpRequest regReq;
            regReq.method = "POST";
            regReq.path = "/api/auth/register";
            regReq.body = "{\"username\":\"helen_dev\",\"displayName\":\"Helen Troy\",\"email\":\"helen@troy.gr\",\"password\":\"Password#Helen1\"}";
            auto regRes = server.handleRequest(regReq);

            bool regOk = (regRes.statusCode == 201 &&
                          regRes.body.find("\"username\":\"helen_dev\"") != std::string::npos &&
                          !regRes.setCookies.empty() &&
                          regRes.setCookies[0].find("codevault_session=") != std::string::npos &&
                          regRes.setCookies[0].find("HttpOnly") != std::string::npos);

            // Extract session token from cookie
            std::string cookieVal = regRes.setCookies[0];
            size_t eqPos = cookieVal.find('=');
            size_t semiPos = cookieVal.find(';');
            std::string sessionToken = cookieVal.substr(eqPos + 1, semiPos - (eqPos + 1));

            // 2. GET /api/auth/me with session cookie
            codevault::app::HttpRequest meReq;
            meReq.method = "GET";
            meReq.path = "/api/auth/me";
            meReq.headers["Cookie"] = "codevault_session=" + sessionToken;
            auto meRes = server.handleRequest(meReq);

            bool meOk = (meRes.statusCode == 200 &&
                         meRes.body.find("\"username\":\"helen_dev\"") != std::string::npos &&
                         meRes.body.find("\"displayName\":\"Helen Troy\"") != std::string::npos);

            // 3. GET /api/auth/me without cookie -> 401
            codevault::app::HttpRequest meNoAuth;
            meNoAuth.method = "GET";
            meNoAuth.path = "/api/auth/me";
            auto meNoAuthRes = server.handleRequest(meNoAuth);
            bool meNoAuthOk = (meNoAuthRes.statusCode == 401);

            // 4. POST /api/auth/logout with cookie -> clears cookie
            codevault::app::HttpRequest logoutReq;
            logoutReq.method = "POST";
            logoutReq.path = "/api/auth/logout";
            logoutReq.headers["Cookie"] = "codevault_session=" + sessionToken;
            auto logoutRes = server.handleRequest(logoutReq);

            bool logoutOk = (logoutRes.statusCode == 200 &&
                             !logoutRes.setCookies.empty() &&
                             logoutRes.setCookies[0].find("Max-Age=0") != std::string::npos);

            // 5. Subsequent GET /api/auth/me -> 401
            auto meAfterLogout = server.handleRequest(meReq);
            bool afterLogoutRevoked = (meAfterLogout.statusCode == 401);

            ok = regOk && meOk && meNoAuthOk && logoutOk && afterLogoutRevoked;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP Route Protection: Unauthenticated Requests to Protected Endpoints Return 401", [&cleanupDb]() {
        std::string dbPath = "build/test_http_protect.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);

            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8912);

            // Protected endpoints without cookie must all return 401
            codevault::app::HttpRequest req;
            req.method = "GET"; req.path = "/api/questions";
            bool q401 = (server.handleRequest(req).statusCode == 401);

            req.method = "GET"; req.path = "/api/dashboard";
            bool d401 = (server.handleRequest(req).statusCode == 401);

            req.method = "GET"; req.path = "/api/revision/due";
            bool r401 = (server.handleRequest(req).statusCode == 401);

            req.method = "POST"; req.path = "/api/practice/start";
            bool p401 = (server.handleRequest(req).statusCode == 401);

            req.method = "GET"; req.path = "/api/history";
            bool h401 = (server.handleRequest(req).statusCode == 401);

            // Public endpoint /api/settings/diagnostics remains accessible
            req.method = "GET"; req.path = "/api/settings/diagnostics";
            auto diagRes = server.handleRequest(req);
            bool diag200 = (diagRes.statusCode == 200 && diagRes.body.find("\"authEnabled\":true") != std::string::npos);

            ok = q401 && d401 && r401 && p401 && h401 && diag200;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP Owner Isolation: Authenticated User A Cannot View, Mutate or Delete User B Questions", [&cleanupDb]() {
        std::string dbPath = "build/test_http_isolation.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);

            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8913);

            // Register User Alice and User Bob
            auto regAlice = auth->registerUser("alice_iso", "Alice", "alice@iso.com", "Password#Alice1");
            auto regBob = auth->registerUser("bob_iso", "Bob", "bob@iso.com", "Password#Bob123!");

            std::string cookieAlice = "codevault_session=" + regAlice.sessionToken;
            std::string cookieBob = "codevault_session=" + regBob.sessionToken;

            // Alice creates a question
            codevault::app::HttpRequest createAlice;
            createAlice.method = "POST";
            createAlice.path = "/api/questions";
            createAlice.headers["Cookie"] = cookieAlice;
            createAlice.body = "{\"id\":\"Q-ALICE-1\",\"title\":\"Alice Secret Problem\",\"topic\":\"Arrays\",\"difficulty\":\"Easy\"}";
            auto createAliceRes = server.handleRequest(createAlice);
            bool aliceCreated = (createAliceRes.statusCode == 200 || createAliceRes.statusCode == 201);

            // Bob creates a question
            codevault::app::HttpRequest createBob;
            createBob.method = "POST";
            createBob.path = "/api/questions";
            createBob.headers["Cookie"] = cookieBob;
            createBob.body = "{\"id\":\"Q-BOB-1\",\"title\":\"Bob Secret Problem\",\"topic\":\"Graphs\",\"difficulty\":\"Hard\"}";
            auto createBobRes = server.handleRequest(createBob);
            bool bobCreated = (createBobRes.statusCode == 200 || createBobRes.statusCode == 201);

            // Alice lists questions: sees ONLY Q-ALICE-1
            codevault::app::HttpRequest listAlice;
            listAlice.method = "GET";
            listAlice.path = "/api/questions";
            listAlice.headers["Cookie"] = cookieAlice;
            auto listAliceRes = server.handleRequest(listAlice);
            bool aliceOnlySeesOwn = (listAliceRes.body.find("Q-ALICE-1") != std::string::npos &&
                                     listAliceRes.body.find("Q-BOB-1") == std::string::npos);

            // Bob lists questions: sees ONLY Q-BOB-1
            codevault::app::HttpRequest listBob;
            listBob.method = "GET";
            listBob.path = "/api/questions";
            listBob.headers["Cookie"] = cookieBob;
            auto listBobRes = server.handleRequest(listBob);
            bool bobOnlySeesOwn = (listBobRes.body.find("Q-BOB-1") != std::string::npos &&
                                   listBobRes.body.find("Q-ALICE-1") == std::string::npos);

            // Bob attempts to GET Alice's question by ID -> 404 Not Found
            codevault::app::HttpRequest bobGetAliceQ;
            bobGetAliceQ.method = "GET";
            bobGetAliceQ.path = "/api/questions/Q-ALICE-1";
            bobGetAliceQ.headers["Cookie"] = cookieBob;
            auto bobGetAliceRes = server.handleRequest(bobGetAliceQ);
            bool bobGetBlocked = (bobGetAliceRes.statusCode == 404);

            // Bob attempts to UPDATE Alice's question -> 404 Not Found
            codevault::app::HttpRequest bobPutAliceQ;
            bobPutAliceQ.method = "PUT";
            bobPutAliceQ.path = "/api/questions/Q-ALICE-1";
            bobPutAliceQ.headers["Cookie"] = cookieBob;
            bobPutAliceQ.body = "{\"title\":\"Hacked Title\"}";
            auto bobPutAliceRes = server.handleRequest(bobPutAliceQ);
            bool bobPutBlocked = (bobPutAliceRes.statusCode == 404);

            // Bob attempts to DELETE Alice's question -> 404 Not Found
            codevault::app::HttpRequest bobDelAliceQ;
            bobDelAliceQ.method = "DELETE";
            bobDelAliceQ.path = "/api/questions/Q-ALICE-1";
            bobDelAliceQ.headers["Cookie"] = cookieBob;
            auto bobDelAliceRes = server.handleRequest(bobDelAliceQ);
            bool bobDelBlocked = (bobDelAliceRes.statusCode == 404);

            ok = aliceCreated && bobCreated && aliceOnlySeesOwn && bobOnlySeesOwn &&
                 bobGetBlocked && bobPutBlocked && bobDelBlocked;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Security Invariants: Password Hashes & Raw Tokens Never Exposed in API or Database", [&cleanupDb]() {
        std::string dbPath = "build/test_sec_invariants.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);

            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8914);

            // Register and Login
            auto regRes = auth->registerUser("ian_dev", "Ian Security", "ian@sec.io", "ComplexPass#999");
            std::string rawToken = regRes.sessionToken;

            // 1. Check /api/auth/me response
            codevault::app::HttpRequest meReq;
            meReq.method = "GET";
            meReq.path = "/api/auth/me";
            meReq.headers["Cookie"] = "codevault_session=" + rawToken;
            auto meRes = server.handleRequest(meReq);

            bool noHashInMe = (meRes.body.find("password") == std::string::npos &&
                               meRes.body.find("$argon2id") == std::string::npos);
            bool noTokenInMe = (meRes.body.find(rawToken) == std::string::npos);

            // 2. Direct inspection of sessions SQLite table: raw token must NOT be stored
            sqlite3* rawDb = nullptr;
            sqlite3_open(dbPath.c_str(), &rawDb);
            sqlite3_stmt* stmt = nullptr;
            sqlite3_prepare_v2(rawDb, "SELECT token_hash FROM sessions WHERE user_id = ?;", -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, regRes.user->getId().c_str(), -1, SQLITE_STATIC);

            bool foundRow = (sqlite3_step(stmt) == SQLITE_ROW);
            std::string storedTokenHash = foundRow ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)) : "";
            sqlite3_finalize(stmt);
            sqlite3_close(rawDb);

            bool tokenNotPlaintext = (storedTokenHash != rawToken && storedTokenHash.length() == 64);
            bool tokenHashMatches = (storedTokenHash == codevault::utils::crypto::hashToken(rawToken));

            // 3. Client spoofing owner_id in POST /api/questions is ignored, owner is strictly authenticated user
            codevault::app::HttpRequest spoofReq;
            spoofReq.method = "POST";
            spoofReq.path = "/api/questions";
            spoofReq.headers["Cookie"] = "codevault_session=" + rawToken;
            spoofReq.body = "{\"id\":\"Q-SPOOF\",\"title\":\"Spoofed Owner Problem\",\"topic\":\"Arrays\",\"difficulty\":\"Easy\",\"owner_id\":\"victim_user\"}";
            server.handleRequest(spoofReq);

            auto savedQ = repo->findById("Q-SPOOF");
            bool spoofPrevented = (savedQ.has_value() && savedQ->getOwnerId() == regRes.user->getId());

            ok = noHashInMe && noTokenInMe && foundRow && tokenNotPlaintext && tokenHashMatches && spoofPrevented;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Schema Migration v2 to v3: Preserves Existing Users, Questions, and local_user Ownership", [&cleanupDb]() {
        std::string dbPath = "build/test_migrate_v2_v3.db";
        cleanupDb(dbPath);

        // 1. Manually create a Schema v2 database (mimicking Stage 9.2 before Stage 9.3)
        {
            sqlite3* rawDb = nullptr;
            sqlite3_open(dbPath.c_str(), &rawDb);
            const char* v2Schema =
                "CREATE TABLE schema_version (version INTEGER PRIMARY KEY, applied_at INTEGER, description TEXT);"
                "INSERT INTO schema_version VALUES (1, 1000, 'Schema v1');"
                "INSERT INTO schema_version VALUES (2, 2000, 'Schema v2');"
                "CREATE TABLE users (id TEXT PRIMARY KEY, username TEXT UNIQUE, display_name TEXT, email TEXT, created_at INTEGER, updated_at INTEGER, active INTEGER);"
                "INSERT INTO users VALUES ('local_user', 'local_user', 'Local User', '', 1774000000, 1774000000, 1);"
                "CREATE TABLE questions ("
                "  id TEXT PRIMARY KEY NOT NULL,"
                "  title TEXT NOT NULL,"
                "  description TEXT DEFAULT '',"
                "  topic TEXT NOT NULL,"
                "  difficulty TEXT NOT NULL,"
                "  company TEXT DEFAULT '',"
                "  platform TEXT DEFAULT '',"
                "  source_url TEXT DEFAULT '',"
                "  status TEXT NOT NULL,"
                "  is_favorite INTEGER DEFAULT 0,"
                "  notes TEXT DEFAULT '',"
                "  created_at INTEGER NOT NULL,"
                "  updated_at INTEGER NOT NULL,"
                "  last_practiced_at INTEGER DEFAULT 0,"
                "  next_revision_at INTEGER DEFAULT 0,"
                "  revision_priority INTEGER DEFAULT 2,"
                "  tags TEXT DEFAULT '',"
                "  owner_id TEXT DEFAULT 'local_user' REFERENCES users(id)"
                ");"
                "INSERT INTO questions (id, title, topic, difficulty, status, created_at, updated_at, owner_id) "
                "VALUES ('Q-V2-PRESERVED', 'Stage 9.2 Preserved Problem', 'Arrays', 'Medium', 'Solved', 1500, 1500, 'local_user');";
            char* err = nullptr;
            sqlite3_exec(rawDb, v2Schema, nullptr, nullptr, &err);
            if (err) sqlite3_free(err);
            sqlite3_close(rawDb);
        }

        // 2. Open with SqliteQuestionRepository, which executes automatic migration to Schema v3
        bool migrated = false;
        {
            codevault::persistence::SqliteQuestionRepository repo(dbPath);
            int version = repo.getSchemaVersion();
            auto qOpt = repo.findById("Q-V2-PRESERVED");

            codevault::persistence::SqliteUserRepository userRepo(dbPath);
            auto localUser = userRepo.findById("local_user");

            codevault::persistence::SqliteSessionRepository sessionRepo(dbPath);
            // Verify new v3 tables exist by creating a session
            codevault::models::Session testSess("ses_mig_test", "local_user", "sample_hash", 100, 500, 0, 100);
            bool sessionTableWorks = sessionRepo.createSession(testSess);

            migrated = (version == 3 &&
                        qOpt.has_value() &&
                        qOpt->getTitle() == "Stage 9.2 Preserved Problem" &&
                        qOpt->getOwnerId() == "local_user" &&
                        localUser.has_value() &&
                        localUser->getUsername() == "local_user" &&
                        sessionTableWorks);
            repo.close();
        }

        cleanupDb(dbPath);
        return migrated;
    });

    // =========================================================================
    // 25. Stage 9.4: User Account Management, Active Sessions & Personal Data Export
    // =========================================================================
    std::cout << "\n--- 25. Stage 9.4: User Account Management, Active Sessions & Personal Data Export ---\n";

    runTest("Profile Update: Display Name, Email & Duplicate Email Rejection", [&cleanupDb]() {
        std::string dbPath = "build/test_profile_update.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo);

            auto u1 = auth.registerUser("user_one", "User One", "one@vault.io", "Password#123");
            auto u2 = auth.registerUser("user_two", "User Two", "two@vault.io", "Password#456");

            std::string err;
            // 1. Valid update
            bool updated = auth.updateProfile(u1.user->getId(), "Updated One", "new_one@vault.io", err);
            auto refreshed1 = userRepo->findById(u1.user->getId());

            // 2. Reject duplicate email (two@vault.io is used by u2)
            bool dupRejected = !auth.updateProfile(u1.user->getId(), "Updated One", "two@vault.io", err);

            // 3. Reject invalid email format
            bool invalidRejected = !auth.updateProfile(u1.user->getId(), "Updated One", "not-an-email", err);

            ok = updated && refreshed1.has_value() &&
                 refreshed1->getDisplayName() == "Updated One" &&
                 refreshed1->getEmail() == "new_one@vault.io" &&
                 dupRejected && invalidRejected;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Password Change: Verification with Argon2id & Constraints", [&cleanupDb]() {
        std::string dbPath = "build/test_pw_change.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo);

            auto reg = auth.registerUser("dave_dev", "Dave", "dave@vault.io", "InitialPassword#123");
            std::string userId = reg.user->getId();

            std::string err;
            // 1. Rejection of incorrect current password
            bool wrongCurrent = !auth.changePassword(userId, "WrongPassword#123", "NewSecretPass#999", err);
            // 2. Rejection of weak new password (< 8 chars)
            bool weakNew = !auth.changePassword(userId, "InitialPassword#123", "short", err);
            // 3. Valid password change
            bool changeSuccess = auth.changePassword(userId, "InitialPassword#123", "NewSecretPass#999", err);

            // 4. Verify login with old password fails
            auto oldLogin = auth.login("dave_dev", "InitialPassword#123");
            // 5. Verify login with new password succeeds
            auto newLogin = auth.login("dave_dev", "NewSecretPass#999");

            ok = wrongCurrent && weakNew && changeSuccess && !oldLogin.success && newLogin.success;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Active Sessions: Listing, Multi-Device Tracking & Expiration", [&cleanupDb]() {
        std::string dbPath = "build/test_sessions_mgmt.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo);

            auto reg = auth.registerUser("sarah_dev", "Sarah", "sarah@vault.io", "SarahPassword#123");
            std::string userId = reg.user->getId();

            // Simulate logins from 2 different devices/browsers
            auto login2 = auth.login("sarah_dev", "SarahPassword#123");
            auto login3 = auth.login("sarah_dev", "SarahPassword#123");

            auto activeSessions = auth.getActiveSessions(userId);
            // Sarah should have 3 active sessions (registration + 2 logins)
            bool countOk = (activeSessions.size() == 3);

            // Revoke specific session (login2)
            std::string tokenHash2 = codevault::utils::crypto::hashToken(login2.sessionToken);
            auto s2 = sessionRepo->findByTokenHash(tokenHash2);
            bool revoked = s2 && auth.revokeSession(userId, s2->getId());

            auto remainingActive = auth.getActiveSessions(userId);
            bool remainingOk = (remainingActive.size() == 2);

            // Attempting to authenticate with revoked token fails
            auto authRevoked = auth.authenticateToken(login2.sessionToken);
            // Attempting to authenticate with still-active token succeeds
            auto authActive = auth.authenticateToken(login3.sessionToken);

            ok = countOk && revoked && remainingOk && !authRevoked.has_value() && authActive.has_value();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Session Revocation: Revoke Other Sessions Preserves Current", [&cleanupDb]() {
        std::string dbPath = "build/test_revoke_others.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            codevault::services::AuthService auth(userRepo, sessionRepo);

            auto reg = auth.registerUser("elena_dev", "Elena", "elena@vault.io", "ElenaPassword#123");
            std::string userId = reg.user->getId();

            auto loginDev1 = auth.login("elena_dev", "ElenaPassword#123");
            auto loginDev2 = auth.login("elena_dev", "ElenaPassword#123");
            auto loginCurrent = auth.login("elena_dev", "ElenaPassword#123");

            // Revoke all sessions except loginCurrent
            bool revokedOthers = auth.revokeOtherSessions(userId, loginCurrent.sessionToken);

            auto sessions = auth.getActiveSessions(userId);
            bool onlyOneActive = (sessions.size() == 1);
            bool currentStillValid = auth.authenticateToken(loginCurrent.sessionToken).has_value();
            bool dev1Invalid = !auth.authenticateToken(loginDev1.sessionToken).has_value();
            bool dev2Invalid = !auth.authenticateToken(loginDev2.sessionToken).has_value();

            ok = revokedOthers && onlyOneActive && currentStillValid && dev1Invalid && dev2Invalid;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("Personal Data Export: JSON and CSV Owner-Scoped Formats", [&cleanupDb]() {
        std::string dbPath = "build/test_data_export.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);

            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8915);

            auto regA = auth->registerUser("alex", "Alex", "alex@vault.io", "Pass#Alex123");
            auto regB = auth->registerUser("bella", "Bella", "bella@vault.io", "Pass#Bella123");

            // Seed question for Alex
            codevault::models::Question qA("Q-EXPORT-1", "Alex Problem, with comma", "Description",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "CompanyA",
                codevault::models::Platform::LeetCode, "https://example.com", codevault::models::Status::Solved,
                true, "Notes with \"quotes\"", 100, 200, 0, 0, 3, {"array", "math"}, regA.user->getId());
            repo->saveForOwner(qA, regA.user->getId());

            // Seed question for Bella
            codevault::models::Question qB("Q-BELLA-1", "Bella Secret", "Secret",
                codevault::models::Topic::Graphs, codevault::models::Difficulty::Hard, "CompanyB",
                codevault::models::Platform::Codeforces, "", codevault::models::Status::Unsolved,
                false, "", 150, 250, 0, 0, 1, {"graph"}, regB.user->getId());
            repo->saveForOwner(qB, regB.user->getId());

            // 1. Alex requests JSON export
            codevault::app::HttpRequest jsonReq;
            jsonReq.method = "GET";
            jsonReq.path = "/api/user/export";
            jsonReq.headers["Cookie"] = "codevault_session=" + regA.sessionToken;
            auto jsonRes = server.handleRequest(jsonReq);

            bool jsonHasAlex = (jsonRes.body.find("Alex Problem") != std::string::npos);
            bool jsonNoBella = (jsonRes.body.find("Bella Secret") == std::string::npos);

            // 2. Alex requests CSV export
            codevault::app::HttpRequest csvReq;
            csvReq.method = "GET";
            csvReq.path = "/api/user/export";
            csvReq.queryParams["format"] = "csv";
            csvReq.headers["Cookie"] = "codevault_session=" + regA.sessionToken;
            auto csvRes = server.handleRequest(csvReq);

            bool csvHeaderOk = (csvRes.body.rfind("id,title,description", 0) == 0);
            bool csvHasAlex = (csvRes.body.find("\"Alex Problem, with comma\"") != std::string::npos);
            bool csvQuotesEscaped = (csvRes.body.find("\"\"quotes\"\"") != std::string::npos);
            bool csvNoBella = (csvRes.body.find("Bella Secret") == std::string::npos);
            bool csvContentType = (csvRes.contentType == "text/csv; charset=utf-8");

            ok = jsonHasAlex && jsonNoBella && csvHeaderOk && csvHasAlex && csvQuotesEscaped && csvNoBella && csvContentType;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP API: Profile Update, Password Change & Session Listing Routes", [&cleanupDb]() {
        std::string dbPath = "build/test_stage9_4_http.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);

            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8916);

            auto reg = auth->registerUser("mike_dev", "Mike Old", "mike@old.io", "OldPassword#123");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // 1. PUT /api/auth/profile
            codevault::app::HttpRequest profReq;
            profReq.method = "PUT";
            profReq.path = "/api/auth/profile";
            profReq.headers["Cookie"] = cookie;
            profReq.body = "{\"displayName\":\"Mike New\",\"email\":\"mike@new.io\"}";
            auto profRes = server.handleRequest(profReq);
            bool profOk = (profRes.statusCode == 200 && profRes.body.find("Mike New") != std::string::npos);

            // 2. POST /api/auth/change-password
            codevault::app::HttpRequest cpReq;
            cpReq.method = "POST";
            cpReq.path = "/api/auth/change-password";
            cpReq.headers["Cookie"] = cookie;
            cpReq.body = "{\"currentPassword\":\"OldPassword#123\",\"newPassword\":\"BrandNewPassword#999\"}";
            auto cpRes = server.handleRequest(cpReq);
            bool cpOk = (cpRes.statusCode == 200);

            // 3. GET /api/auth/sessions
            codevault::app::HttpRequest sessReq;
            sessReq.method = "GET";
            sessReq.path = "/api/auth/sessions";
            sessReq.headers["Cookie"] = cookie;
            auto sessRes = server.handleRequest(sessReq);
            bool sessOk = (sessRes.statusCode == 200 &&
                           sessRes.body.find("\"isCurrentSession\":true") != std::string::npos &&
                           sessRes.body.find("token_hash") == std::string::npos);

            ok = profOk && cpOk && sessOk;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    // =========================================================================
    // 26. Stage 9.5: Catalog Import, Advanced Export & Backend Services (Phase 1)
    // =========================================================================
    std::cout << "\n--- 26. Stage 9.5: Catalog Import, Advanced Export & Backend Services ---\n";

    runTest("ImportConflictStrategy: Parsing, String Conversion & Validation", []() {
        using codevault::models::ImportConflictStrategy;
        using codevault::models::stringToConflictStrategy;
        using codevault::models::conflictStrategyToString;

        ImportConflictStrategy s;
        bool b1 = stringToConflictStrategy("skip", s) && s == ImportConflictStrategy::Skip;
        bool b2 = stringToConflictStrategy("overwrite", s) && s == ImportConflictStrategy::Overwrite;
        bool b3 = stringToConflictStrategy("generate_new_id", s) && s == ImportConflictStrategy::GenerateNewId;
        bool b4 = !stringToConflictStrategy("unknown", s);
        bool b5 = !stringToConflictStrategy("", s);

        bool s1 = (conflictStrategyToString(ImportConflictStrategy::Skip) == "skip");
        bool s2 = (conflictStrategyToString(ImportConflictStrategy::Overwrite) == "overwrite");
        bool s3 = (conflictStrategyToString(ImportConflictStrategy::GenerateNewId) == "generate_new_id");

        return b1 && b2 && b3 && b4 && b5 && s1 && s2 && s3;
    });

    runTest("ImportService: Valid JSON Catalog Batch Ingestion & Entity Integrity", [&cleanupDb]() {
        std::string dbPath = "build/test_import_json.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto userProv = std::make_shared<codevault::services::StaticCurrentUserProvider>("alice_dev");
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev, userProv);
            codevault::services::ImportService importer(qs);

            std::string jsonPayload = R"json({
                "questions": [
                    {
                        "id": "Q-JSON-1",
                        "title": "Median of Two Sorted Arrays",
                        "description": "Find median in O(log(min(m, n)))",
                        "topic": "Arrays",
                        "difficulty": "Hard",
                        "platform": "LeetCode",
                        "company": "Google",
                        "source_url": "https://leetcode.com/problems/median-of-two-sorted-arrays/",
                        "status": "Solved",
                        "is_favorite": true,
                        "notes": "Binary search on partition boundary",
                        "revision_priority": 1,
                        "tags": ["array", "binary-search", "divide-and-conquer"]
                    },
                    {
                        "id": "Q-JSON-2",
                        "title": "Reverse Linked List",
                        "description": "Reverse singly linked list iteratively",
                        "topic": "LinkedLists",
                        "difficulty": "Easy",
                        "platform": "LeetCode",
                        "company": "Amazon",
                        "source_url": "https://leetcode.com/problems/reverse-linked-list/",
                        "status": "Solved",
                        "is_favorite": false,
                        "notes": "3 pointer iterative traversal",
                        "revision_priority": 2,
                        "tags": ["linked-list"]
                    }
                ]
            })json";

            auto res = importer.importFromJson(jsonPayload, codevault::models::ImportConflictStrategy::Skip);
            bool success = res.success && (res.importedCount == 2) && (res.totalProcessed == 2) && res.errors.empty();

            auto q1 = qs->getQuestionById("Q-JSON-1");
            auto q2 = qs->getQuestionById("Q-JSON-2");

            bool q1Ok = q1.has_value() && q1->getTitle() == "Median of Two Sorted Arrays" &&
                        q1->getDifficulty() == codevault::models::Difficulty::Hard &&
                        q1->getTopic() == codevault::models::Topic::Arrays &&
                        q1->isFavorite() && q1->getTags().size() == 3 &&
                        q1->getOwnerId() == "alice_dev";

            bool q2Ok = q2.has_value() && q2->getTitle() == "Reverse Linked List" &&
                        q2->getDifficulty() == codevault::models::Difficulty::Easy &&
                        q2->getOwnerId() == "alice_dev";

            ok = success && q1Ok && q2Ok;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("ImportService: Valid RFC 4180 CSV Import with Complex Quoted & Multiline Fields", [&cleanupDb]() {
        std::string dbPath = "build/test_import_csv.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto userProv = std::make_shared<codevault::services::StaticCurrentUserProvider>("bob_dev");
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev, userProv);
            codevault::services::ImportService importer(qs);

            std::string csvPayload =
                "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\r\n"
                "Q-CSV-1,\"Two Sum, Multi-Hash\",\"Description line 1\nDescription line 2\",Arrays,Easy,Meta,LeetCode,https://leetcode.com/q1,Solved,1,\"Approach with \"\"two pointers\"\" and commas, safe.\",100,200,0,0,1,array;hash-table,ignored_owner\r\n"
                "Q-CSV-2,Simple Binary Search,Basic BS algorithm,Arrays,Medium,Google,LeetCode,https://leetcode.com/q2,Unsolved,0,Standard notes,150,250,0,0,2,binary-search,ignored_owner\r\n";

            auto res = importer.importFromCsv(csvPayload, codevault::models::ImportConflictStrategy::Skip);
            bool success = res.success && (res.importedCount == 2) && res.errors.empty();

            auto q1 = qs->getQuestionById("Q-CSV-1");
            auto q2 = qs->getQuestionById("Q-CSV-2");

            bool q1Ok = q1.has_value() &&
                        q1->getTitle() == "Two Sum, Multi-Hash" &&
                        q1->getDescription() == "Description line 1\nDescription line 2" &&
                        q1->getNotes() == "Approach with \"two pointers\" and commas, safe." &&
                        q1->getTags().size() == 2 &&
                        q1->getOwnerId() == "bob_dev"; // stamped with authenticated owner

            bool q2Ok = q2.has_value() && q2->getTitle() == "Simple Binary Search" &&
                        q2->getOwnerId() == "bob_dev";

            ok = success && q1Ok && q2Ok;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("ImportService: Invalid JSON and CSV Formats & Missing Required Fields Error Handling", [&cleanupDb]() {
        std::string dbPath = "build/test_import_invalid.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto qs = std::make_shared<codevault::services::QuestionService>(repo);
            codevault::services::ImportService importer(qs);

            // 1. Malformed JSON
            auto resJson = importer.importFromJson("{invalid json");
            bool jsonErr = (!resJson.success && resJson.hasErrors());

            // 2. CSV with unclosed quote
            auto resCsvQuote = importer.importFromCsv("id,title\r\nQ-1,\"Unclosed quote");
            bool quoteErr = (!resCsvQuote.success && resCsvQuote.hasErrors());

            // 3. CSV missing required 'title' header
            auto resCsvHeader = importer.importFromCsv("id,difficulty,status\r\nQ-1,Easy,Solved\r\n");
            bool headerErr = (!resCsvHeader.success && resCsvHeader.hasErrors());

            // 4. Batch with empty question title
            std::string emptyTitleJson = R"({"questions": [{"title": "", "difficulty": "Easy"}]})";
            auto resEmptyTitle = importer.importFromJson(emptyTitleJson);
            bool emptyTitleErr = (!resEmptyTitle.success && resEmptyTitle.hasErrors());

            // 5. CSV empty string returns success with 0 processed
            auto resEmpty = importer.importFromCsv("");
            bool emptyOk = (resEmpty.success && resEmpty.totalProcessed == 0);

            ok = jsonErr && quoteErr && headerErr && emptyTitleErr && emptyOk;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("ImportService: Conflict Strategies (Skip, Overwrite, GenerateNewId)", [&cleanupDb]() {
        std::string dbPath = "build/test_conflict_strategies.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userProv = std::make_shared<codevault::services::StaticCurrentUserProvider>("carol_dev");
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, nullptr, userProv);
            codevault::services::ImportService importer(qs);

            // Seed initial question
            codevault::models::Question initialQ("Q-1001", "Original Title", "Initial Description",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "CompanyA",
                codevault::models::Platform::LeetCode, "", codevault::models::Status::Unsolved,
                false, "Initial Notes", 100, 100, 0, 0, 2, {}, "carol_dev");
            repo->saveForOwner(initialQ, "carol_dev");

            // Batch containing existing Q-1001 with modified title and brand-new Q-NEW
            std::vector<codevault::models::Question> batch = {
                codevault::models::Question("Q-1001", "Attempted New Title", "New Desc",
                    codevault::models::Topic::Strings, codevault::models::Difficulty::Hard, "Meta",
                    codevault::models::Platform::LeetCode, "", codevault::models::Status::Solved,
                    true, "Updated Notes", 200, 200, 0, 0, 1, {"str"}, "carol_dev"),
                codevault::models::Question("Q-BRAND-NEW", "Brand New Problem", "Desc",
                    codevault::models::Topic::Graphs, codevault::models::Difficulty::Medium, "Amazon",
                    codevault::models::Platform::LeetCode, "", codevault::models::Status::Unsolved,
                    false, "", 200, 200, 0, 0, 2, {}, "carol_dev")
            };

            // 1. Strategy: Skip
            auto resSkip = importer.importQuestions(batch, codevault::models::ImportConflictStrategy::Skip);
            bool skipOk = resSkip.success && (resSkip.skippedCount == 1) && (resSkip.importedCount == 1);
            auto qAfterSkip = qs->getQuestionById("Q-1001");
            bool skipTitleUntouched = (qAfterSkip.has_value() && qAfterSkip->getTitle() == "Original Title");

            // 2. Strategy: Overwrite
            auto resOverwrite = importer.importQuestions(batch, codevault::models::ImportConflictStrategy::Overwrite);
            bool overwriteOk = resOverwrite.success && (resOverwrite.updatedCount == 2);
            auto qAfterOverwrite = qs->getQuestionById("Q-1001");
            bool overwriteTitleUpdated = (qAfterOverwrite.has_value() && qAfterOverwrite->getTitle() == "Attempted New Title" &&
                                          qAfterOverwrite->getDifficulty() == codevault::models::Difficulty::Hard);

            // 3. Strategy: GenerateNewId
            auto resGenId = importer.importQuestions(batch, codevault::models::ImportConflictStrategy::GenerateNewId);
            bool genOk = resGenId.success && (resGenId.importedCount == 2);
            // Q-1001 retains its overwritten state, and a fresh ID (e.g. Q-1002) was generated for the payload question
            auto allQ = qs->getAllQuestions();
            bool totalCountOk = (allQ.size() == 4); // Q-1001, Q-BRAND-NEW, plus 2 from GenerateNewId

            ok = skipOk && skipTitleUntouched && overwriteOk && overwriteTitleUpdated && genOk && totalCountOk;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("ImportService: Mandatory Owner-ID Isolation & Client Spoofing Resistance", [&cleanupDb]() {
        std::string dbPath = "build/test_import_isolation.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userProv = std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_alice");
            auto qsAlice = std::make_shared<codevault::services::QuestionService>(repo, nullptr, nullptr, userProv);
            codevault::services::ImportService importer(qsAlice);

            // Alice imports a payload maliciously containing "owner_id": "usr_bob"
            std::string spoofPayload = R"({
                "questions": [
                    {
                        "id": "Q-SPOOF-1",
                        "title": "Alice Spoof Attempt",
                        "difficulty": "Easy",
                        "owner_id": "usr_bob"
                    }
                ]
            })";

            auto res = importer.importFromJson(spoofPayload, codevault::models::ImportConflictStrategy::Skip);
            bool importOk = res.success && (res.importedCount == 1);

            // Check in repo: question owner_id MUST be usr_alice
            auto qOpt = repo->findById("Q-SPOOF-1");
            bool ownerEnforced = qOpt.has_value() && (qOpt->getOwnerId() == "usr_alice");

            // Bob context cannot see this question
            userProv->setCurrentUserId("usr_bob");
            qsAlice->refreshUserScope();
            bool bobCannotSee = !qsAlice->getQuestionById("Q-SPOOF-1").has_value();

            ok = importOk && ownerEnforced && bobCannotSee;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("ImportService: Cross-User ID Conflict Safety (Cannot Overwrite Other Users)", [&cleanupDb]() {
        std::string dbPath = "build/test_cross_user_conflict.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userProv = std::make_shared<codevault::services::StaticCurrentUserProvider>("usr_alice");
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, nullptr, userProv);
            codevault::services::ImportService importer(qs);

            // 1. Alice creates Q-SECRET
            codevault::models::Question qAlice("Q-SECRET", "Alice Top Secret Problem", "Desc",
                codevault::models::Topic::DynamicProgramming, codevault::models::Difficulty::Hard, "Google",
                codevault::models::Platform::LeetCode, "", codevault::models::Status::Solved,
                true, "Secret note", 100, 100, 0, 0, 1, {}, "usr_alice");
            repo->saveForOwner(qAlice, "usr_alice");

            // 2. Switch context to Bob
            userProv->setCurrentUserId("usr_bob");
            qs->refreshUserScope();

            // Bob imports a question attempting to use Q-SECRET with strategy Overwrite
            codevault::models::Question qBob("Q-SECRET", "Bob Attacker Title", "Desc",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Meta",
                codevault::models::Platform::LeetCode, "", codevault::models::Status::Unsolved,
                false, "", 200, 200, 0, 0, 2, {}, "usr_bob");

            auto resOverwrite = importer.importQuestions({qBob}, codevault::models::ImportConflictStrategy::Overwrite);
            bool importSuccess = resOverwrite.success;

            // 3. Verify Alice's question was NEVER overwritten
            auto qAliceAfter = repo->findByIdForOwner("Q-SECRET", "usr_alice");
            bool aliceUntouched = qAliceAfter.has_value() &&
                                  qAliceAfter->getTitle() == "Alice Top Secret Problem" &&
                                  qAliceAfter->getOwnerId() == "usr_alice";

            // 4. Verify Bob got his question assigned a fresh safe ID
            auto bobQuestions = repo->findAllByOwner("usr_bob");
            bool bobGotIsolatedId = (bobQuestions.size() == 1) &&
                                    (bobQuestions[0].getId() != "Q-SECRET") &&
                                    (bobQuestions[0].getTitle() == "Bob Attacker Title") &&
                                    (bobQuestions[0].getOwnerId() == "usr_bob");

            ok = importSuccess && aliceUntouched && bobGotIsolatedId;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("ImportService: Atomic Batch Rollback on Database Failure", [&cleanupDb]() {
        std::string dbPath = "build/test_import_rollback.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userProv = std::make_shared<codevault::services::StaticCurrentUserProvider>("dan_dev");
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, nullptr, userProv);
            codevault::services::ImportService importer(qs);

            // Batch with 3 questions: Q1 valid, Q2 invalid priority (99), Q3 valid
            std::vector<codevault::models::Question> batch = {
                codevault::models::Question("Q-ATOMIC-1", "Valid Question 1", "Desc",
                    codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "",
                    codevault::models::Platform::Custom, "", codevault::models::Status::Unsolved,
                    false, "", 100, 100, 0, 0, 2, {}, "dan_dev"),
                codevault::models::Question("Q-ATOMIC-2", "Invalid Question Priority", "Desc",
                    codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "",
                    codevault::models::Platform::Custom, "", codevault::models::Status::Unsolved,
                    false, "", 100, 100, 0, 0, 99, {}, "dan_dev"), // Illegal priority 99
                codevault::models::Question("Q-ATOMIC-3", "Valid Question 3", "Desc",
                    codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "",
                    codevault::models::Platform::Custom, "", codevault::models::Status::Unsolved,
                    false, "", 100, 100, 0, 0, 2, {}, "dan_dev")
            };

            auto res = importer.importQuestions(batch, codevault::models::ImportConflictStrategy::Skip);
            bool failed = (!res.success && res.hasErrors());

            // Invariant: Q-ATOMIC-1 MUST NOT exist in database (zero partial imports)
            bool q1NotSaved = !repo->exists("Q-ATOMIC-1");
            bool q2NotSaved = !repo->exists("Q-ATOMIC-2");
            bool q3NotSaved = !repo->exists("Q-ATOMIC-3");
            bool totalCountZero = (repo->countForOwner("dan_dev") == 0);

            ok = failed && q1NotSaved && q2NotSaved && q3NotSaved && totalCountZero;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("ImportService: In-Memory DSA Synchronization (PrefixTrie & MinHeap)", [&cleanupDb]() {
        std::string dbPath = "build/test_import_dsa_sync.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto search = std::make_shared<codevault::services::SearchService>();
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto userProv = std::make_shared<codevault::services::StaticCurrentUserProvider>("eva_dev");
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, search, rev, userProv);
            codevault::services::ImportService importer(qs);

            std::string payload = R"({
                "questions": [
                    {
                        "id": "Q-SYNC-1",
                        "title": "Dijkstra Shortest Path",
                        "topic": "Graphs",
                        "difficulty": "Hard",
                        "status": "Solved",
                        "next_revision_at": 1800000000,
                        "revision_priority": 1,
                        "tags": ["graph", "shortest-path"]
                    },
                    {
                        "id": "Q-SYNC-2",
                        "title": "Dijkstra with Negative Weights Check",
                        "topic": "Graphs",
                        "difficulty": "Medium",
                        "status": "Unsolved",
                        "next_revision_at": 1850000000,
                        "revision_priority": 2
                    }
                ]
            })";

            auto res = importer.importFromJson(payload, codevault::models::ImportConflictStrategy::Skip);
            bool importOk = res.success && (res.importedCount == 2);

            // 1. Verify PrefixTrie autocomplete search immediately finds imported questions
            auto trieMatches = qs->searchQuestionsByTitlePrefix("Dijkstra");
            bool trieOk = (trieMatches.size() == 2);

            // 2. Verify MinHeap priority queue immediately reflects imported revision schedules
            auto upcoming = rev->getUpcomingRevisions(1700000000);
            bool heapOk = (upcoming.size() == 2) && (upcoming[0].questionId == "Q-SYNC-1") && (upcoming[0].priority == 1);

            ok = importOk && trieOk && heapOk;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("ExportService: Markdown Catalog Formatting & Deterministic Escaping", []() {
        std::vector<codevault::models::Question> questions = {
            codevault::models::Question("Q-101", "Binary Tree Zigzag | Level Order", "Traverse binary tree in zigzag order.",
                codevault::models::Topic::Trees, codevault::models::Difficulty::Medium, "Amazon",
                codevault::models::Platform::LeetCode, "https://leetcode.com/problems/zigzag", codevault::models::Status::Solved,
                true, "Use two stacks or deque.", 100, 200, 0, 0, 1, {"tree", "bfs"}, "user_1"),
            codevault::models::Question("Q-102", "Simple Math Problem", "",
                codevault::models::Topic::MathGeometry, codevault::models::Difficulty::Easy, "",
                codevault::models::Platform::Custom, "", codevault::models::Status::Unsolved,
                false, "", 150, 250, 0, 0, 3, {}, "user_1")
        };

        std::string md = codevault::services::ExportService::exportToMarkdown(questions, 1729000000);

        bool hasHeader = (md.find("# CodeVault Problem Catalog Export") != std::string::npos);
        bool hasMeta = (md.find("Total Problems: 2") != std::string::npos && md.find("1729000000") != std::string::npos);
        bool hasIndexTable = (md.find("| ID | Title | Topic | Difficulty | Status | Platform |") != std::string::npos);
        bool pipeEscapedInTable = (md.find("Binary Tree Zigzag &#124; Level Order") != std::string::npos);
        bool hasDetails = (md.find("### Q-101 — Binary Tree Zigzag | Level Order") != std::string::npos);
        bool hasTags = (md.find("- **Tags**: tree, bfs") != std::string::npos);
        bool hasNotes = (md.find("Use two stacks or deque.") != std::string::npos);
        bool emptyNotesHandled = (md.find("*(No notes provided)*") != std::string::npos);

        return hasHeader && hasMeta && hasIndexTable && pipeEscapedInTable && hasDetails && hasTags && hasNotes && emptyNotesHandled;
    });

    runTest("ExportService: Anki TSV Flashcard Deck Generation & Escaping", []() {
        std::vector<codevault::models::Question> questions = {
            codevault::models::Question("Q-201", "Two Sum with\tTab", "Given an array of integers.\nFind two numbers.",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Google",
                codevault::models::Platform::LeetCode, "https://leetcode.com/p/two-sum", codevault::models::Status::Solved,
                true, "One-pass\thash map\r\nO(N) time.", 100, 200, 0, 0, 1, {"two sum", "hash-map"}, "user_1"),
            codevault::models::Question("Q-202", "Empty Problem", "",
                codevault::models::Topic::Other, codevault::models::Difficulty::Medium, "",
                codevault::models::Platform::Custom, "", codevault::models::Status::Unsolved,
                false, "", 100, 100, 0, 0, 2, {}, "user_1")
        };

        std::string tsv = codevault::services::ExportService::exportToAnkiTsv(questions);

        std::stringstream ss(tsv);
        std::string line;
        int lineCount = 0;
        bool allLinesTwoTabs = true;
        bool newlinesConvertedToBr = true;

        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            lineCount++;

            // Count tabs: exactly 2 tabs per line (3 columns: Front, Back, Tags)
            int tabCount = 0;
            for (char c : line) {
                if (c == '\t') tabCount++;
            }
            if (tabCount != 2) {
                allLinesTwoTabs = false;
            }

            // Verify \r or \n were converted to <br>
            if (line.find("\r") != std::string::npos) {
                newlinesConvertedToBr = false;
            }
        }

        bool lineCountOk = (lineCount == 2);
        bool hasBrInOutput = (tsv.find("<br>") != std::string::npos);
        bool tagsContainHyphen = (tsv.find("two-sum") != std::string::npos); // space in tag replaced with hyphen

        return lineCountOk && allLinesTwoTabs && newlinesConvertedToBr && hasBrInOutput && tagsContainHyphen;
    });

    // =========================================================================
    // 27. Stage 9.5: HTTP API Integration & Route Protection (Phase 2)
    // =========================================================================
    std::cout << "\n--- 27. Stage 9.5: HTTP API Integration & Route Protection ---\n";

    runTest("HTTP API: Route Protection & 401 Rejection for Unauthenticated Import & Export", [&cleanupDb]() {
        std::string dbPath = "build/test_stage9_5_http_auth.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8931);

            // 1. Unauthenticated POST /api/user/import -> 401
            codevault::app::HttpRequest reqImport;
            reqImport.method = "POST";
            reqImport.path = "/api/user/import";
            reqImport.headers["Content-Type"] = "application/json";
            reqImport.body = R"json({"questions":[]})json";
            auto resImport = server.handleRequest(reqImport);
            bool unauthImport = (resImport.statusCode == 401);

            // 2. Unauthenticated GET /api/user/export/markdown -> 401
            codevault::app::HttpRequest reqMd;
            reqMd.method = "GET";
            reqMd.path = "/api/user/export/markdown";
            auto resMd = server.handleRequest(reqMd);
            bool unauthMd = (resMd.statusCode == 401);

            // 3. Unauthenticated GET /api/user/export/anki -> 401
            codevault::app::HttpRequest reqAnki;
            reqAnki.method = "GET";
            reqAnki.path = "/api/user/export/anki";
            auto resAnki = server.handleRequest(reqAnki);
            bool unauthAnki = (resAnki.statusCode == 401);

            // 4. Invalid session token -> 401
            codevault::app::HttpRequest reqBadCookie;
            reqBadCookie.method = "GET";
            reqBadCookie.path = "/api/user/export/markdown";
            reqBadCookie.headers["Cookie"] = "codevault_session=tampered_token_xyz";
            auto resBadCookie = server.handleRequest(reqBadCookie);
            bool badCookieRejected = (resBadCookie.statusCode == 401);

            // 5. Method not allowed for authenticated user
            auto reg = auth->registerUser("auth_user", "Auth User", "auth@test.io", "StrongPass#123");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            codevault::app::HttpRequest reqWrongMethod;
            reqWrongMethod.method = "POST";
            reqWrongMethod.path = "/api/user/export/markdown";
            reqWrongMethod.headers["Cookie"] = cookie;
            auto resWrongMethod = server.handleRequest(reqWrongMethod);
            bool wrongMethod405 = (resWrongMethod.statusCode == 405);

            codevault::app::HttpRequest reqWrongMethodImport;
            reqWrongMethodImport.method = "GET";
            reqWrongMethodImport.path = "/api/user/import";
            reqWrongMethodImport.headers["Cookie"] = cookie;
            auto resWrongMethodImport = server.handleRequest(reqWrongMethodImport);
            bool wrongMethodImport405 = (resWrongMethodImport.statusCode == 405);

            ok = unauthImport && unauthMd && unauthAnki && badCookieRejected && wrongMethod405 && wrongMethodImport405;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP API: JSON Catalog Ingestion & Content-Type Dispatching", [&cleanupDb]() {
        std::string dbPath = "build/test_stage9_5_http_json.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8932);

            auto reg = auth->registerUser("json_user", "JSON User", "json@test.io", "StrongPass#123");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            std::string jsonBody = R"json({
                "questions": [
                    {
                        "id": "Q-HTTP-J1",
                        "title": "Container With Most Water",
                        "description": "Two pointer boundary optimization",
                        "topic": "Arrays",
                        "difficulty": "Medium",
                        "platform": "LeetCode",
                        "company": "Amazon",
                        "status": "Solved",
                        "is_favorite": true,
                        "notes": "Two pointer inward crawl",
                        "revision_priority": 1,
                        "tags": ["array", "two-pointers"]
                    },
                    {
                        "id": "Q-HTTP-J2",
                        "title": "Subtree of Another Tree",
                        "description": "Tree hashing or traversal",
                        "topic": "Trees",
                        "difficulty": "Easy",
                        "platform": "LeetCode",
                        "company": "Google",
                        "status": "Unsolved",
                        "is_favorite": false,
                        "notes": "IsIdentical helper",
                        "revision_priority": 2,
                        "tags": ["tree", "dfs"]
                    }
                ]
            })json";

            codevault::app::HttpRequest req;
            req.method = "POST";
            req.path = "/api/user/import";
            req.headers["Cookie"] = cookie;
            req.headers["Content-Type"] = "application/json; charset=utf-8";
            req.body = jsonBody;

            auto res = server.handleRequest(req);
            bool status200 = (res.statusCode == 200);
            bool resSuccess = (res.body.find("\"success\":true") != std::string::npos);
            bool resCountOk = (res.body.find("\"importedCount\":2") != std::string::npos &&
                               res.body.find("\"totalProcessed\":2") != std::string::npos);

            // Invariant: Stamped with authenticated user's ID
            auto q1 = repo->findByIdForOwner("Q-HTTP-J1", reg.user->getId());
            auto q2 = repo->findByIdForOwner("Q-HTTP-J2", reg.user->getId());
            bool q1Ok = q1.has_value() && q1->getTitle() == "Container With Most Water" && q1->isFavorite();
            bool q2Ok = q2.has_value() && q2->getDifficulty() == codevault::models::Difficulty::Easy;

            ok = status200 && resSuccess && resCountOk && q1Ok && q2Ok;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP API: RFC 4180 CSV Import & Multiline Preservation", [&cleanupDb]() {
        std::string dbPath = "build/test_stage9_5_http_csv.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8933);

            auto reg = auth->registerUser("csv_user", "CSV User", "csv@test.io", "StrongPass#123");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            std::string csvBody =
                "id,title,description,topic,difficulty,company,platform,source_url,status,is_favorite,notes,created_at,updated_at,last_practiced_at,next_revision_at,revision_priority,tags,owner_id\r\n"
                "Q-HTTP-C1,\"3Sum, Multi-Target\",\"Line 1 of description\nLine 2 of description\",Arrays,Medium,Meta,LeetCode,https://leetcode.com,Solved,1,\"Sort first, then \"\"two pointer\"\" sweep\",100,100,0,0,1,array;two-pointers,ignored_spoof\r\n";

            codevault::app::HttpRequest req;
            req.method = "POST";
            req.path = "/api/user/import";
            req.headers["Cookie"] = cookie;
            req.headers["Content-Type"] = "text/csv";
            req.body = csvBody;

            auto res = server.handleRequest(req);
            bool status200 = (res.statusCode == 200);
            bool resSuccess = (res.body.find("\"success\":true") != std::string::npos);
            bool resCountOk = (res.body.find("\"importedCount\":1") != std::string::npos);

            auto q = repo->findByIdForOwner("Q-HTTP-C1", reg.user->getId());
            bool qOk = q.has_value() &&
                       q->getTitle() == "3Sum, Multi-Target" &&
                       q->getDescription() == "Line 1 of description\nLine 2 of description" &&
                       q->getNotes() == "Sort first, then \"two pointer\" sweep" &&
                       q->getOwnerId() == reg.user->getId();

            ok = status200 && resSuccess && resCountOk && qOk;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP API: Conflict Strategy Negotiation (Skip, Overwrite, GenerateNewId & Invalid Rejection)", [&cleanupDb]() {
        std::string dbPath = "build/test_stage9_5_http_conflict.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8934);

            auto reg = auth->registerUser("strat_user", "Strat User", "strat@test.io", "StrongPass#123");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // Seed initial question
            codevault::models::Question initQ("Q-STRAT-1", "Original Strat Title", "Desc",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "",
                codevault::models::Platform::Custom, "", codevault::models::Status::Unsolved,
                false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(initQ, reg.user->getId());

            std::string payloadJson = R"json({
                "questions": [
                    {
                        "id": "Q-STRAT-1",
                        "title": "Modified Strat Title",
                        "difficulty": "Hard"
                    }
                ]
            })json";

            // 1. Conflict Strategy: skip -> original untouched, skippedCount = 1
            codevault::app::HttpRequest reqSkip;
            reqSkip.method = "POST";
            reqSkip.path = "/api/user/import";
            reqSkip.queryParams["conflict_strategy"] = "skip";
            reqSkip.headers["Cookie"] = cookie;
            reqSkip.headers["Content-Type"] = "application/json";
            reqSkip.body = payloadJson;
            auto resSkip = server.handleRequest(reqSkip);
            bool skipOk = (resSkip.statusCode == 200 && resSkip.body.find("\"skippedCount\":1") != std::string::npos);
            auto qAfterSkip = repo->findByIdForOwner("Q-STRAT-1", reg.user->getId());
            bool skipTitleUntouched = (qAfterSkip.has_value() && qAfterSkip->getTitle() == "Original Strat Title");

            // 2. Conflict Strategy: overwrite -> updatedCount = 1, title updated
            codevault::app::HttpRequest reqOverwrite;
            reqOverwrite.method = "POST";
            reqOverwrite.path = "/api/user/import";
            reqOverwrite.queryParams["conflict_strategy"] = "overwrite";
            reqOverwrite.headers["Cookie"] = cookie;
            reqOverwrite.headers["Content-Type"] = "application/json";
            reqOverwrite.body = payloadJson;
            auto resOverwrite = server.handleRequest(reqOverwrite);
            bool overwriteOk = (resOverwrite.statusCode == 200 && resOverwrite.body.find("\"updatedCount\":1") != std::string::npos);
            auto qAfterOverwrite = repo->findByIdForOwner("Q-STRAT-1", reg.user->getId());
            bool overwriteTitleUpdated = (qAfterOverwrite.has_value() && qAfterOverwrite->getTitle() == "Modified Strat Title");

            // 3. Conflict Strategy: generate_new_id -> importedCount = 1, new ID allocated
            codevault::app::HttpRequest reqGenId;
            reqGenId.method = "POST";
            reqGenId.path = "/api/user/import";
            reqGenId.queryParams["conflict_strategy"] = "generate_new_id";
            reqGenId.headers["Cookie"] = cookie;
            reqGenId.headers["Content-Type"] = "application/json";
            reqGenId.body = payloadJson;
            auto resGenId = server.handleRequest(reqGenId);
            bool genIdOk = (resGenId.statusCode == 200 && resGenId.body.find("\"importedCount\":1") != std::string::npos);
            bool totalQuestionsNowTwo = (repo->countForOwner(reg.user->getId()) == 2);

            // 4. Invalid conflict strategy -> 400 Bad Request
            codevault::app::HttpRequest reqInvalidStrat;
            reqInvalidStrat.method = "POST";
            reqInvalidStrat.path = "/api/user/import";
            reqInvalidStrat.queryParams["conflict_strategy"] = "nonexistent_mode";
            reqInvalidStrat.headers["Cookie"] = cookie;
            reqInvalidStrat.headers["Content-Type"] = "application/json";
            reqInvalidStrat.body = payloadJson;
            auto resInvalidStrat = server.handleRequest(reqInvalidStrat);
            bool invalidStratRejected = (resInvalidStrat.statusCode == 400 && resInvalidStrat.body.find("Invalid conflict strategy") != std::string::npos);

            ok = skipOk && skipTitleUntouched && overwriteOk && overwriteTitleUpdated && genIdOk && totalQuestionsNowTwo && invalidStratRejected;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP API: Strict Owner Isolation & Cross-User Spoof Resistance", [&cleanupDb]() {
        std::string dbPath = "build/test_stage9_5_http_isolation.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8935);

            auto regAlice = auth->registerUser("alice_http", "Alice", "alice@http.io", "Password#123");
            auto regBob = auth->registerUser("bob_http", "Bob", "bob@http.io", "Password#123");

            // Alice seeds Q-SECRET
            codevault::models::Question qAlice("Q-SECRET", "Alice Secret Question", "Confidential",
                codevault::models::Topic::DynamicProgramming, codevault::models::Difficulty::Hard, "",
                codevault::models::Platform::LeetCode, "", codevault::models::Status::Solved,
                true, "Secret Alice Notes", 100, 100, 0, 0, 1, {"dp"}, regAlice.user->getId());
            repo->saveForOwner(qAlice, regAlice.user->getId());

            // 1. Bob attempts to import payload with owner_id = alice's ID
            std::string bobSpoofJson = R"json({
                "questions": [
                    {
                        "id": "Q-BOB-1",
                        "title": "Bob Injected Question",
                        "difficulty": "Easy",
                        "owner_id": ")json" + regAlice.user->getId() + R"json("
                    }
                ]
            })json";

            codevault::app::HttpRequest reqBobSpoof;
            reqBobSpoof.method = "POST";
            reqBobSpoof.path = "/api/user/import";
            reqBobSpoof.headers["Cookie"] = "codevault_session=" + regBob.sessionToken;
            reqBobSpoof.headers["Content-Type"] = "application/json";
            reqBobSpoof.body = bobSpoofJson;
            auto resBobSpoof = server.handleRequest(reqBobSpoof);
            bool spoofImportOk = (resBobSpoof.statusCode == 200);

            // Invariant: Q-BOB-1 belongs to Bob, NOT Alice
            auto qBobSaved = repo->findById("Q-BOB-1");
            bool stampedBob = qBobSaved.has_value() && (qBobSaved->getOwnerId() == regBob.user->getId());

            // 2. Bob attempts to overwrite Alice's Q-SECRET
            std::string bobStealJson = R"json({
                "questions": [
                    {
                        "id": "Q-SECRET",
                        "title": "Bob Malicious Overwrite",
                        "difficulty": "Easy"
                    }
                ]
            })json";

            codevault::app::HttpRequest reqBobSteal;
            reqBobSteal.method = "POST";
            reqBobSteal.path = "/api/user/import";
            reqBobSteal.queryParams["conflict_strategy"] = "overwrite";
            reqBobSteal.headers["Cookie"] = "codevault_session=" + regBob.sessionToken;
            reqBobSteal.headers["Content-Type"] = "application/json";
            reqBobSteal.body = bobStealJson;
            auto resBobSteal = server.handleRequest(reqBobSteal);
            bool stealCallOk = (resBobSteal.statusCode == 200);

            // Invariant: Alice's question was NEVER modified
            auto qAliceAfter = repo->findByIdForOwner("Q-SECRET", regAlice.user->getId());
            bool aliceUntouched = qAliceAfter.has_value() &&
                                  qAliceAfter->getTitle() == "Alice Secret Question" &&
                                  qAliceAfter->getOwnerId() == regAlice.user->getId();

            // 3. Export Isolation: Alice cannot see Bob questions; Bob cannot see Alice questions
            codevault::app::HttpRequest reqAliceExp;
            reqAliceExp.method = "GET";
            reqAliceExp.path = "/api/user/export/markdown";
            reqAliceExp.headers["Cookie"] = "codevault_session=" + regAlice.sessionToken;
            auto resAliceExp = server.handleRequest(reqAliceExp);
            bool aliceExportHasAlice = (resAliceExp.body.find("Alice Secret Question") != std::string::npos);
            bool aliceExportNoBob = (resAliceExp.body.find("Bob Injected Question") == std::string::npos &&
                                     resAliceExp.body.find("Bob Malicious Overwrite") == std::string::npos);

            codevault::app::HttpRequest reqBobExp;
            reqBobExp.method = "GET";
            reqBobExp.path = "/api/user/export/markdown";
            reqBobExp.headers["Cookie"] = "codevault_session=" + regBob.sessionToken;
            auto resBobExp = server.handleRequest(reqBobExp);
            bool bobExportNoAlice = (resBobExp.body.find("Alice Secret Question") == std::string::npos);

            ok = spoofImportOk && stampedBob && stealCallOk && aliceUntouched &&
                 aliceExportHasAlice && aliceExportNoBob && bobExportNoAlice;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP API: Markdown Catalog Export (Content-Type, Headers & Scoped Body)", [&cleanupDb]() {
        std::string dbPath = "build/test_stage9_5_http_md_export.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8936);

            auto reg = auth->registerUser("md_user", "Markdown User", "md@test.io", "StrongPass#123");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // Seed question
            codevault::models::Question q("Q-MD-1", "Serialize and Deserialize Binary Tree", "Binary tree serialization",
                codevault::models::Topic::Trees, codevault::models::Difficulty::Hard, "Amazon",
                codevault::models::Platform::LeetCode, "https://leetcode.com", codevault::models::Status::Solved,
                true, "Level order BFS serialization", 100, 100, 0, 0, 1, {"tree", "bfs"}, reg.user->getId());
            repo->saveForOwner(q, reg.user->getId());

            codevault::app::HttpRequest req;
            req.method = "GET";
            req.path = "/api/user/export/markdown";
            req.headers["Cookie"] = cookie;

            auto res = server.handleRequest(req);
            bool status200 = (res.statusCode == 200);
            bool contentTypeOk = (res.contentType == "text/markdown; charset=utf-8");
            bool dispositionOk = (res.customHeaders["Content-Disposition"] == "attachment; filename=\"codevault-questions.md\"");
            bool hasHeader = (res.body.find("# CodeVault Problem Catalog Export") != std::string::npos);
            bool hasTable = (res.body.find("| ID | Title | Topic | Difficulty | Status | Platform |") != std::string::npos);
            bool hasQuestion = (res.body.find("Serialize and Deserialize Binary Tree") != std::string::npos);

            ok = status200 && contentTypeOk && dispositionOk && hasHeader && hasTable && hasQuestion;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP API: Anki TSV Flashcard Export (Content-Type, Headers & TSV Structure)", [&cleanupDb]() {
        std::string dbPath = "build/test_stage9_5_http_anki_export.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8937);

            auto reg = auth->registerUser("anki_user", "Anki User", "anki@test.io", "StrongPass#123");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // Seed question with embedded tab and newline
            codevault::models::Question q("Q-ANKI-1", "Valid Anagram\tTabbed", "Given two strings s and t.\nReturn true if t is an anagram.",
                codevault::models::Topic::Strings, codevault::models::Difficulty::Easy, "Facebook",
                codevault::models::Platform::LeetCode, "https://leetcode.com", codevault::models::Status::Solved,
                true, "Frequency array\r\nor hash map.", 100, 100, 0, 0, 1, {"string", "hash-table"}, reg.user->getId());
            repo->saveForOwner(q, reg.user->getId());

            codevault::app::HttpRequest req;
            req.method = "GET";
            req.path = "/api/user/export/anki";
            req.headers["Cookie"] = cookie;

            auto res = server.handleRequest(req);
            bool status200 = (res.statusCode == 200);
            bool contentTypeOk = (res.contentType == "text/tab-separated-values; charset=utf-8");
            bool dispositionOk = (res.customHeaders["Content-Disposition"] == "attachment; filename=\"codevault-questions-anki.tsv\"");

            // Verify TSV structure: exactly 2 tabs (3 columns: Front, Back, Tags)
            int tabCount = 0;
            for (char c : res.body) {
                if (c == '\t') tabCount++;
            }
            bool twoTabs = (tabCount == 2);
            bool brPreserved = (res.body.find("<br>") != std::string::npos);
            bool noRawCr = (res.body.find("\r") == std::string::npos);

            ok = status200 && contentTypeOk && dispositionOk && twoTabs && brPreserved && noRawCr;
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP API: Malformed Payloads, Unsupported Formats & Atomic Rollback", [&cleanupDb]() {
        std::string dbPath = "build/test_stage9_5_http_errors.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8938);

            auto reg = auth->registerUser("err_user", "Error User", "err@test.io", "StrongPass#123");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // 1. Malformed JSON -> 400 Bad Request
            codevault::app::HttpRequest reqBadJson;
            reqBadJson.method = "POST";
            reqBadJson.path = "/api/user/import";
            reqBadJson.headers["Cookie"] = cookie;
            reqBadJson.headers["Content-Type"] = "application/json";
            reqBadJson.body = "{ invalid json structure";
            auto resBadJson = server.handleRequest(reqBadJson);
            bool badJsonOk = (resBadJson.statusCode == 400 && resBadJson.body.find("\"success\":false") != std::string::npos);

            // 2. Malformed CSV (unclosed quote) -> 400 Bad Request
            codevault::app::HttpRequest reqBadCsv;
            reqBadCsv.method = "POST";
            reqBadCsv.path = "/api/user/import";
            reqBadCsv.headers["Cookie"] = cookie;
            reqBadCsv.headers["Content-Type"] = "text/csv";
            reqBadCsv.body = "id,title\r\nQ-1,\"Unterminated quote without closing";
            auto resBadCsv = server.handleRequest(reqBadCsv);
            bool badCsvOk = (resBadCsv.statusCode == 400 && resBadCsv.body.find("\"success\":false") != std::string::npos);

            // 3. Unsupported format -> 400 Bad Request
            codevault::app::HttpRequest reqBadFormat;
            reqBadFormat.method = "POST";
            reqBadFormat.path = "/api/user/import";
            reqBadFormat.headers["Cookie"] = cookie;
            reqBadFormat.headers["Content-Type"] = "application/xml";
            reqBadFormat.body = "<xml></xml>";
            auto resBadFormat = server.handleRequest(reqBadFormat);
            bool badFormatOk = (resBadFormat.statusCode == 400 && resBadFormat.body.find("Unsupported or missing format") != std::string::npos);

            // 4. Batch with validation failure -> Atomic rollback (0 saved questions)
            std::string fatalBatchJson = R"json({
                "questions": [
                    {
                        "id": "Q-ROLL-1",
                        "title": "First Valid Question",
                        "difficulty": "Easy"
                    },
                    {
                        "id": "Q-ROLL-2",
                        "title": "",
                        "difficulty": "Easy"
                    }
                ]
            })json";

            codevault::app::HttpRequest reqFatalBatch;
            reqFatalBatch.method = "POST";
            reqFatalBatch.path = "/api/user/import";
            reqFatalBatch.headers["Cookie"] = cookie;
            reqFatalBatch.headers["Content-Type"] = "application/json";
            reqFatalBatch.body = fatalBatchJson;
            auto resFatalBatch = server.handleRequest(reqFatalBatch);
            bool fatalBatch400 = (resFatalBatch.statusCode == 400 && resFatalBatch.body.find("\"success\":false") != std::string::npos);

            // Invariant: Q-ROLL-1 was rolled back and does NOT exist in SQLite
            bool q1NotSaved = !repo->exists("Q-ROLL-1");
            bool totalZero = (repo->countForOwner(reg.user->getId()) == 0);

            ok = badJsonOk && badCsvOk && badFormatOk && fatalBatch400 && q1NotSaved && totalZero;
        }

        cleanupDb(dbPath);
        return ok;
    });

    // =========================================================================
    // 28. Stage 10: Advanced Practice Workflow & Practice Queue (Phase 1)
    // =========================================================================
    std::cout << "\n--- 28. Stage 10: Advanced Practice Workflow & Practice Queue (Phase 1) ---\n";

    runTest("dsa::Queue: toVector, contains, and remove (Head, Middle, Tail, Missing)", []() {
        codevault::dsa::Queue<std::string> q;
        q.enqueue("A");
        q.enqueue("B");
        q.enqueue("C");
        q.enqueue("D");

        // 1. toVector non-destructive inspection
        auto vec = q.toVector();
        bool vecOk = (vec.size() == 4 && vec[0] == "A" && vec[1] == "B" && vec[2] == "C" && vec[3] == "D");
        bool sizeBefore = (q.size() == 4);

        // 2. contains check
        bool hasB = q.contains("B");
        bool hasZ = !q.contains("Z");

        // 3. remove middle element "B"
        bool remB = q.remove("B");
        auto vecAfterB = q.toVector();
        bool remBOk = remB && (q.size() == 3) && (vecAfterB.size() == 3) &&
                      (vecAfterB[0] == "A" && vecAfterB[1] == "C" && vecAfterB[2] == "D") && !q.contains("B");

        // 4. remove head element "A"
        bool remA = q.remove("A");
        bool frontIsC = (q.front() == "C");
        bool remAOk = remA && (q.size() == 2) && frontIsC && (q.toVector()[0] == "C");

        // 5. remove tail element "D"
        bool remD = q.remove("D");
        bool remDOk = remD && (q.size() == 1) && (q.front() == "C") && (q.toVector()[0] == "C");

        // 6. remove missing element "Z"
        bool remZ = !q.remove("Z");
        bool remZOk = remZ && (q.size() == 1);

        // 7. remove final element "C" leaving queue empty
        bool remC = q.remove("C");
        bool emptyOk = remC && q.empty() && (q.size() == 0);

        // 8. remove on empty queue
        bool remEmpty = !q.remove("X");

        // 9. re-enqueue after clear works properly
        q.enqueue("NewFront");
        bool reEnqueueOk = (q.size() == 1 && q.front() == "NewFront");

        return vecOk && sizeBefore && hasB && hasZ && remBOk && remAOk && remDOk && remZOk && emptyOk && remEmpty && reEnqueueOk;
    });

    runTest("PracticeService: Queue Introspection, Unlinking & Multi-Item Management", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_ps_queue.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);

            auto reg = auth->registerUser("queue_coder", "Queue Coder", "qc@test.io", "Pass#12345");
            qs->setCurrentUserProvider(std::make_shared<codevault::services::StaticCurrentUserProvider>(reg.user->getId()));

            // Seed questions Q1, Q2, Q3, Q4
            for (int i = 1; i <= 4; ++i) {
                std::string qid = "Q-PRAC-" + std::to_string(i);
                codevault::models::Question q(qid, "Problem " + std::to_string(i), "", codevault::models::Topic::Arrays,
                    codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                    codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
                repo->saveForOwner(q, reg.user->getId());
            }

            // Start session with Q1, Q2, Q3, Q4
            ps->startSession({"Q-PRAC-1", "Q-PRAC-2", "Q-PRAC-3", "Q-PRAC-4"});

            bool initCount = (ps->getRemainingCount() == 4);
            auto ids = ps->getQueueQuestionIds();
            bool idsOk = (ids.size() == 4 && ids[0] == "Q-PRAC-1" && ids[3] == "Q-PRAC-4");

            auto fullQuestions = ps->getQueueQuestions();
            bool fullOk = (fullQuestions.size() == 4 && fullQuestions[0].getId() == "Q-PRAC-1");

            bool isQueued2 = ps->isQuestionQueued("Q-PRAC-2");
            bool isQueued99 = !ps->isQuestionQueued("Q-PRAC-99");

            // Remove middle question Q-PRAC-2
            bool removedMiddle = ps->removeQuestionFromQueue("Q-PRAC-2");
            bool middleGone = !ps->isQuestionQueued("Q-PRAC-2") && (ps->getRemainingCount() == 3);
            auto idsAfterMiddle = ps->getQueueQuestionIds();
            bool orderPreserved = (idsAfterMiddle.size() == 3 && idsAfterMiddle[0] == "Q-PRAC-1" &&
                                   idsAfterMiddle[1] == "Q-PRAC-3" && idsAfterMiddle[2] == "Q-PRAC-4");

            // Remove current/head question Q-PRAC-1
            bool removedHead = ps->removeQuestionFromQueue("Q-PRAC-1");
            bool headAdvanced = (ps->getCurrentQuestion() == "Q-PRAC-3") && (ps->getRemainingCount() == 2);

            // Removing nonexistent question returns false
            bool removedMissing = !ps->removeQuestionFromQueue("Q-PRAC-99");

            ok = initCount && idsOk && fullOk && isQueued2 && isQueued99 &&
                 removedMiddle && middleGone && orderPreserved && removedHead && headAdvanced && removedMissing;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("PracticeService: startSessionWithFilter (Topic, Difficulty, and Revision Due Filters)", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_filter_session.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);

            auto reg = auth->registerUser("filter_user", "Filter User", "fu@test.io", "Pass#12345");
            qs->setCurrentUserProvider(std::make_shared<codevault::services::StaticCurrentUserProvider>(reg.user->getId()));

            int64_t now = 1700000000;
            codevault::models::Question q1("Q-ARR-EASY", "Two Sum", "", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            codevault::models::Question q2("Q-ARR-MED", "3Sum", "", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            codevault::models::Question q3("Q-DP-HARD", "Edit Distance", "", codevault::models::Topic::DynamicProgramming,
                codevault::models::Difficulty::Hard, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            codevault::models::Question q4("Q-REV-DUE", "Longest Palindrome", "", codevault::models::Topic::Strings,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Solved, false, "", 100, 100, now - 5000, now - 1000, 1, {}, reg.user->getId());

            repo->saveForOwner(q1, reg.user->getId());
            repo->saveForOwner(q2, reg.user->getId());
            repo->saveForOwner(q3, reg.user->getId());
            repo->saveForOwner(q4, reg.user->getId());
            rev->loadFromQuestions(qs->getAllQuestions());

            // 1. Filter by Topic: Arrays (both Easy and Medium)
            codevault::models::QuestionFilter fArrays;
            fArrays.topic = codevault::models::Topic::Arrays;
            ps->startSessionWithFilter(fArrays);
            bool arrSessionOk = (ps->getRemainingCount() == 2 && ps->isQuestionQueued("Q-ARR-EASY") && ps->isQuestionQueued("Q-ARR-MED"));

            // 2. Filter by Topic + Difficulty: Arrays + Medium (only Q-ARR-MED)
            codevault::models::QuestionFilter fArrMed;
            fArrMed.topic = codevault::models::Topic::Arrays;
            fArrMed.difficulty = codevault::models::Difficulty::Medium;
            ps->startSessionWithFilter(fArrMed);
            bool arrMedSessionOk = (ps->getRemainingCount() == 1 && ps->getCurrentQuestion() == "Q-ARR-MED");

            // 3. Filter with dueOnly = true: only Q-REV-DUE is due
            codevault::models::QuestionFilter fEmpty;
            ps->startSessionWithFilter(fEmpty, true);
            bool dueSessionOk = (ps->getRemainingCount() == 1 && ps->getCurrentQuestion() == "Q-REV-DUE");

            // 4. Filter with no matches: empty queue
            codevault::models::QuestionFilter fNoMatch;
            fNoMatch.topic = codevault::models::Topic::Graphs;
            ps->startSessionWithFilter(fNoMatch);
            bool emptySessionOk = !ps->hasQuestions() && (ps->getRemainingCount() == 0);

            ok = arrSessionOk && arrMedSessionOk && dueSessionOk && emptySessionOk;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("PracticeService: Single-Question Practice Verdict (Solved, NeedsReview, Skipped, and Unlinking)", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_verdicts.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);

            auto reg = auth->registerUser("verdict_user", "Verdict User", "vu@test.io", "Pass#12345");
            qs->setCurrentUserProvider(std::make_shared<codevault::services::StaticCurrentUserProvider>(reg.user->getId()));

            int64_t now = 1700000000;
            codevault::models::Question q1("Q-VER-1", "Problem One", "", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            codevault::models::Question q2("Q-VER-2", "Problem Two", "", codevault::models::Topic::Strings,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            codevault::models::Question q3("Q-VER-3", "Problem Three", "", codevault::models::Topic::Trees,
                codevault::models::Difficulty::Hard, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());

            repo->saveForOwner(q1, reg.user->getId());
            repo->saveForOwner(q2, reg.user->getId());
            repo->saveForOwner(q3, reg.user->getId());
            rev->loadFromQuestions(qs->getAllQuestions());

            // Enqueue Q-VER-1 and Q-VER-2
            ps->startSession({"Q-VER-1", "Q-VER-2"});

            // 1. Mark Solved for Q-VER-1 (front of queue)
            auto res1 = ps->recordPracticeAttemptForQuestion("Q-VER-1", codevault::models::PracticeVerdict::Solved, now + 100);
            bool res1Ok = res1.has_value() && res1->newStatus == codevault::models::Status::Solved &&
                          res1->nextRevisionAt > (now + 100);
            // Verify Q-VER-1 status is now Solved, last_practiced_at is set, and unlinked from queue
            auto q1After = repo->findById("Q-VER-1");
            bool q1SavedOk = (q1After->getStatus() == codevault::models::Status::Solved) &&
                             (q1After->getLastPracticedAt() == (now + 100)) &&
                             (!ps->isQuestionQueued("Q-VER-1")) &&
                             (ps->getCurrentQuestion() == "Q-VER-2") &&
                             (ps->getRemainingCount() == 1);

            // 2. Mark NeedsReview for Q-VER-2
            auto res2 = ps->recordPracticeAttemptForQuestion("Q-VER-2", codevault::models::PracticeVerdict::NeedsReview, now + 200);
            bool res2Ok = res2.has_value() && res2->newStatus == codevault::models::Status::InProgress &&
                          res2->nextRevisionAt == ((now + 200) + 86400); // 1-day interval
            auto q2After = repo->findById("Q-VER-2");
            bool q2SavedOk = (q2After->getStatus() == codevault::models::Status::InProgress) &&
                             (q2After->getLastPracticedAt() == (now + 200)) &&
                             (!ps->hasQuestions()); // Queue is now empty

            // 3. Mark Skipped on Q-VER-3 (which was NEVER in the active practice queue)
            auto res3 = ps->recordPracticeAttemptForQuestion("Q-VER-3", codevault::models::PracticeVerdict::Skipped, now + 300);
            bool res3Ok = res3.has_value();
            auto q3After = repo->findById("Q-VER-3");
            // Status remains Todo, last_practiced_at is preserved
            bool q3SavedOk = (q3After->getStatus() == codevault::models::Status::Todo);

            // 4. Invalid question ID returns std::nullopt
            auto resNonexistent = ps->recordPracticeAttemptForQuestion("Q-NONEXISTENT", codevault::models::PracticeVerdict::Solved, now);
            bool nonExistentBlocked = !resNonexistent.has_value();

            ok = res1Ok && q1SavedOk && res2Ok && q2SavedOk && res3Ok && q3SavedOk && nonExistentBlocked;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("PracticeService: Deterministic \"Practice Next\" Cascade & Rule Verification", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_practice_next.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);

            auto reg = auth->registerUser("next_user", "Next User", "nu@test.io", "Pass#12345");
            qs->setCurrentUserProvider(std::make_shared<codevault::services::StaticCurrentUserProvider>(reg.user->getId()));

            int64_t now = 1700000000;

            codevault::models::Question qTodo("Q-TODO", "Binary Search", "", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            codevault::models::Question qReview("Q-REVIEW", "Course Schedule", "", codevault::models::Topic::Graphs,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::InProgress, false, "", 100, 100, now - 5000, now + 10000, 2, {}, reg.user->getId());
            codevault::models::Question qDue("Q-DUE", "LRU Cache", "", codevault::models::Topic::StacksQueues,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Solved, false, "", 100, 100, now - 10000, now - 1000, 1, {}, reg.user->getId());
            codevault::models::Question qSolved("Q-SOLVED", "Invert Binary Tree", "", codevault::models::Topic::Trees,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Solved, false, "", 100, 100, now - 2000, now + 100000, 3, {}, reg.user->getId());

            repo->saveForOwner(qTodo, reg.user->getId());
            repo->saveForOwner(qReview, reg.user->getId());
            repo->saveForOwner(qDue, reg.user->getId());
            repo->saveForOwner(qSolved, reg.user->getId());
            rev->loadFromQuestions(qs->getAllQuestions());

            // Case A: Active session queue takes priority over everything
            ps->startSession({"Q-TODO"});
            auto nextA = ps->getPracticeNext({}, now);
            bool caseAOk = nextA.hasQuestion && nextA.question->getId() == "Q-TODO" &&
                           nextA.recommendationReason.find("practice queue") != std::string::npos;
            ps->clearSession();

            // Case B: No active queue -> Overdue revision from MinHeap takes priority
            auto nextB = ps->getPracticeNext({}, now);
            bool caseBOk = nextB.hasQuestion && nextB.question->getId() == "Q-DUE" &&
                           nextB.recommendationReason.find("spaced revision") != std::string::npos;

            // Case C: Criteria exclude due revisions -> Unsolved/never-practiced question surfaces
            codevault::models::PracticeNextCriteria critNoDue;
            critNoDue.includeDueRevisions = false;
            auto nextC = ps->getPracticeNext(critNoDue, now);
            bool caseCOk = nextC.hasQuestion && nextC.question->getId() == "Q-TODO" &&
                           nextC.recommendationReason.find("Unsolved problem") != std::string::npos;

            // Case D: Narrow by topic: Graphs -> selects Q-REVIEW
            codevault::models::PracticeNextCriteria critGraphs;
            critGraphs.topic = codevault::models::Topic::Graphs;
            auto nextD = ps->getPracticeNext(critGraphs, now);
            bool caseDOk = nextD.hasQuestion && nextD.question->getId() == "Q-REVIEW";

            // Case E: Narrow by difficulty: Hard (no matching questions)
            codevault::models::PracticeNextCriteria critHard;
            critHard.difficulty = codevault::models::Difficulty::Hard;
            auto nextE = ps->getPracticeNext(critHard, now);
            bool caseEOk = !nextE.hasQuestion && !nextE.question.has_value() &&
                           nextE.recommendationReason.find("No eligible problem") != std::string::npos;

            ok = caseAOk && caseBOk && caseCOk && caseDOk && caseEOk;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("PracticeService: Multi-User Boundary Enforcement Across Queue and Recommendations", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_ps_iso.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();

            auto regAlice = auth->registerUser("alice_prac", "Alice", "alice@p.io", "Pass#12345");
            auto regBob = auth->registerUser("bob_prac", "Bob", "bob@p.io", "Pass#12345");

            // Seed Alice questions
            codevault::models::Question qAlice("Q-ALICE-P", "Alice Problem", "", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, regAlice.user->getId());
            repo->saveForOwner(qAlice, regAlice.user->getId());

            // Seed Bob questions
            codevault::models::Question qBob("Q-BOB-P", "Bob Problem", "", codevault::models::Topic::Strings,
                codevault::models::Difficulty::Hard, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, regBob.user->getId());
            repo->saveForOwner(qBob, regBob.user->getId());

            // Alice context
            auto providerAlice = std::make_shared<codevault::services::StaticCurrentUserProvider>(regAlice.user->getId());
            auto qsAlice = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev, providerAlice);
            auto psAlice = std::make_shared<codevault::services::PracticeService>(qsAlice, rev);

            // Bob context
            auto providerBob = std::make_shared<codevault::services::StaticCurrentUserProvider>(regBob.user->getId());
            auto qsBob = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev, providerBob);
            auto psBob = std::make_shared<codevault::services::PracticeService>(qsBob, rev);

            // 1. Practice Next isolation: Alice only recommended Alice problem, Bob only Bob problem
            auto nextAlice = psAlice->getPracticeNext();
            auto nextBob = psBob->getPracticeNext();
            bool nextIso = (nextAlice.hasQuestion && nextAlice.question->getId() == "Q-ALICE-P") &&
                           (nextBob.hasQuestion && nextBob.question->getId() == "Q-BOB-P");

            // 2. Queue Isolation: Alice cannot enqueue Bob's problem
            psAlice->startSession({"Q-BOB-P"});
            bool aliceBlockedBob = !psAlice->hasQuestions() && (psAlice->getRemainingCount() == 0);

            // 3. Verdict Isolation: Alice cannot record attempt on Bob's problem
            auto attemptBobByAlice = psAlice->recordPracticeAttemptForQuestion("Q-BOB-P", codevault::models::PracticeVerdict::Solved, 1000);
            bool verdictBlocked = !attemptBobByAlice.has_value();

            // Bob's problem remains Todo in DB
            auto bobQAfter = repo->findById("Q-BOB-P");
            bool bobUntouched = (bobQAfter.has_value() && bobQAfter->getStatus() == codevault::models::Status::Todo &&
                                 bobQAfter->getLastPracticedAt() == 0);

            ok = nextIso && aliceBlockedBob && verdictBlocked && bobUntouched;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    // =========================================================================
    // 29. Stage 10: Advanced Practice Workflow & Practice HTTP API (Phase 2)
    // =========================================================================
    std::cout << "\n--- 29. Stage 10: Advanced Practice Workflow & Practice HTTP API (Phase 2) ---\n";

    runTest("HTTP Practice API: Route Protection & 401 Rejection for Unauthenticated Endpoints", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_http_auth.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8940);

            // 1. GET /api/practice/next without auth
            codevault::app::HttpRequest reqNext;
            reqNext.method = "GET";
            reqNext.path = "/api/practice/next";
            auto resNext = server.handleRequest(reqNext);
            bool next401 = (resNext.statusCode == 401);

            // 2. POST /api/practice/session without auth
            codevault::app::HttpRequest reqSession;
            reqSession.method = "POST";
            reqSession.path = "/api/practice/session";
            reqSession.body = "{\"topic\":\"Arrays\"}";
            auto resSession = server.handleRequest(reqSession);
            bool session401 = (resSession.statusCode == 401);

            // 3. GET /api/practice/queue without auth
            codevault::app::HttpRequest reqQueue;
            reqQueue.method = "GET";
            reqQueue.path = "/api/practice/queue";
            auto resQueue = server.handleRequest(reqQueue);
            bool queue401 = (resQueue.statusCode == 401);

            // 4. DELETE /api/practice/queue/:id without auth
            codevault::app::HttpRequest reqRemQueue;
            reqRemQueue.method = "DELETE";
            reqRemQueue.path = "/api/practice/queue/Q-ANY";
            auto resRemQueue = server.handleRequest(reqRemQueue);
            bool remQueue401 = (resRemQueue.statusCode == 401);

            // 5. POST /api/practice/:id/result without auth
            codevault::app::HttpRequest reqResult;
            reqResult.method = "POST";
            reqResult.path = "/api/practice/Q-ANY/result";
            reqResult.body = "{\"verdict\":\"Solved\"}";
            auto resResult = server.handleRequest(reqResult);
            bool result401 = (resResult.statusCode == 401);

            ok = next401 && session401 && queue401 && remQueue401 && result401;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP Practice API: GET /api/practice/next (Deterministic Recommendation & Cascade Rationale)", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_http_next.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8941);

            auto reg = auth->registerUser("prac_user", "Prac User", "prac@test.io", "Pass#12345");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // 1. When user has no questions, returns 200 with hasQuestion=false
            codevault::app::HttpRequest reqEmpty;
            reqEmpty.method = "GET";
            reqEmpty.path = "/api/practice/next";
            reqEmpty.headers["Cookie"] = cookie;
            auto resEmpty = server.handleRequest(reqEmpty);
            bool emptyOk = (resEmpty.statusCode == 200);
            auto emptyJson = codevault::utils::json::parse(resEmpty.body);
            bool emptyHasQ = !emptyJson.getBool("hasQuestion");

            // 2. Seed a problem and request next
            codevault::models::Question q("Q-NEXT-1", "Two Sum", "Two sum problem",
                codevault::models::Topic::Arrays, codevault::models::Difficulty::Easy, "Amazon",
                codevault::models::Platform::LeetCode, "https://leetcode.com", codevault::models::Status::Todo,
                false, "", 100, 100, 0, 0, 2, {"hashmap"}, reg.user->getId());
            repo->saveForOwner(q, reg.user->getId());

            codevault::app::HttpRequest req;
            req.method = "GET";
            req.path = "/api/practice/next";
            req.headers["Cookie"] = cookie;
            auto res = server.handleRequest(req);
            bool status200 = (res.statusCode == 200);
            auto root = codevault::utils::json::parse(res.body);
            bool hasQ = root.getBool("hasQuestion");
            std::string reason = root.getString("recommendationReason");
            const auto* qNode = root.get("question");
            bool qIdMatches = (qNode != nullptr && qNode->getString("id") == "Q-NEXT-1");
            bool reasonValid = !reason.empty();

            ok = emptyOk && emptyHasQ && status200 && hasQ && qIdMatches && reasonValid;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP Practice API: GET /api/practice/next (Topic, Difficulty & Due Filter Negotiation)", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_http_next_filter.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8942);

            auto reg = auth->registerUser("filter_user", "Filter User", "filter@test.io", "Pass#12345");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // Seed questions
            codevault::models::Question qArr("Q-ARR", "Array Problem", "", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(qArr, reg.user->getId());

            codevault::models::Question qTree("Q-TREE", "Tree Problem", "", codevault::models::Topic::Trees,
                codevault::models::Difficulty::Hard, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(qTree, reg.user->getId());

            // 1. Topic filter: Trees
            codevault::app::HttpRequest reqTopic;
            reqTopic.method = "GET";
            reqTopic.path = "/api/practice/next?topic=Trees";
            reqTopic.headers["Cookie"] = cookie;
            auto resTopic = server.handleRequest(reqTopic);
            auto rootTopic = codevault::utils::json::parse(resTopic.body);
            bool topicOk = (resTopic.statusCode == 200 && rootTopic.getBool("hasQuestion") &&
                            rootTopic.get("question") && rootTopic.get("question")->getString("id") == "Q-TREE");

            // 2. Difficulty filter: Easy
            codevault::app::HttpRequest reqDiff;
            reqDiff.method = "GET";
            reqDiff.path = "/api/practice/next?difficulty=Easy";
            reqDiff.headers["Cookie"] = cookie;
            auto resDiff = server.handleRequest(reqDiff);
            auto rootDiff = codevault::utils::json::parse(resDiff.body);
            bool diffOk = (resDiff.statusCode == 200 && rootDiff.getBool("hasQuestion") &&
                           rootDiff.get("question") && rootDiff.get("question")->getString("id") == "Q-ARR");

            // 3. Filter matching nothing
            codevault::app::HttpRequest reqNone;
            reqNone.method = "GET";
            reqNone.path = "/api/practice/next?topic=Graphs";
            reqNone.headers["Cookie"] = cookie;
            auto resNone = server.handleRequest(reqNone);
            auto rootNone = codevault::utils::json::parse(resNone.body);
            bool noneOk = (resNone.statusCode == 200 && !rootNone.getBool("hasQuestion"));

            ok = topicOk && diffOk && noneOk;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP Practice API: POST /api/practice/session (Criteria Filtering & FIFO Queue Initialization)", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_http_session.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8943);

            auto reg = auth->registerUser("session_user", "Session User", "session@test.io", "Pass#12345");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // Seed questions
            codevault::models::Question q1("Q-DP-1", "DP 1", "", codevault::models::Topic::DynamicProgramming,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(q1, reg.user->getId());

            codevault::models::Question q2("Q-DP-2", "DP 2", "", codevault::models::Topic::DynamicProgramming,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(q2, reg.user->getId());

            codevault::models::Question q3("Q-STR-1", "String 1", "", codevault::models::Topic::Strings,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(q3, reg.user->getId());

            // 1. Start filtered session with DP Medium
            codevault::app::HttpRequest reqSession;
            reqSession.method = "POST";
            reqSession.path = "/api/practice/session";
            reqSession.headers["Cookie"] = cookie;
            reqSession.body = "{\"topic\":\"DynamicProgramming\",\"difficulty\":\"Medium\"}";
            auto resSession = server.handleRequest(reqSession);

            bool status200 = (resSession.statusCode == 200);
            auto prog = codevault::utils::json::parse(resSession.body);
            bool progOk = (prog.getInt("total") == 2 && prog.getInt("remaining") == 2);

            // Verify queue endpoint reflects the session
            codevault::app::HttpRequest reqQueue;
            reqQueue.method = "GET";
            reqQueue.path = "/api/practice/queue";
            reqQueue.headers["Cookie"] = cookie;
            auto resQueue = server.handleRequest(reqQueue);
            auto queueJson = codevault::utils::json::parse(resQueue.body);
            bool queueCountOk = (queueJson.getInt("count") == 2);

            // 2. Start session matching nothing
            codevault::app::HttpRequest reqEmpty;
            reqEmpty.method = "POST";
            reqEmpty.path = "/api/practice/session";
            reqEmpty.headers["Cookie"] = cookie;
            reqEmpty.body = "{\"topic\":\"Tries\"}";
            auto resEmpty = server.handleRequest(reqEmpty);
            auto emptyProg = codevault::utils::json::parse(resEmpty.body);
            bool emptyProgOk = (resEmpty.statusCode == 200 && emptyProg.getInt("total") == 0 && emptyProg.getInt("remaining") == 0);

            ok = status200 && progOk && queueCountOk && emptyProgOk;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP Practice API: GET /api/practice/queue & DELETE /api/practice/queue/:id", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_http_queue.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8944);

            auto reg = auth->registerUser("queue_user", "Queue User", "queue@test.io", "Pass#12345");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // Seed questions
            codevault::models::Question q1("Q-FIFO-1", "FIFO 1", "", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(q1, reg.user->getId());

            codevault::models::Question q2("Q-FIFO-2", "FIFO 2", "", codevault::models::Topic::Strings,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(q2, reg.user->getId());

            codevault::models::Question q3("Q-FIFO-3", "FIFO 3", "", codevault::models::Topic::Trees,
                codevault::models::Difficulty::Hard, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(q3, reg.user->getId());

            // Initialize practice session
            qs->setCurrentUserProvider(std::make_shared<codevault::services::AuthenticatedCurrentUserProvider>(*reg.user));
            ps->startSession({"Q-FIFO-1", "Q-FIFO-2", "Q-FIFO-3"});

            // 1. GET /api/practice/queue
            codevault::app::HttpRequest reqGet;
            reqGet.method = "GET";
            reqGet.path = "/api/practice/queue";
            reqGet.headers["Cookie"] = cookie;
            auto resGet = server.handleRequest(reqGet);
            bool get200 = (resGet.statusCode == 200);
            auto rootGet = codevault::utils::json::parse(resGet.body);
            bool countIs3 = (rootGet.getInt("count") == 3);

            // 2. DELETE head question Q-FIFO-1
            codevault::app::HttpRequest reqDelHead;
            reqDelHead.method = "DELETE";
            reqDelHead.path = "/api/practice/queue/Q-FIFO-1";
            reqDelHead.headers["Cookie"] = cookie;
            auto resDelHead = server.handleRequest(reqDelHead);
            bool delHead200 = (resDelHead.statusCode == 200 && resDelHead.body.find("\"removed\":true") != std::string::npos);

            // 3. Verify queue now has count 2 and front is Q-FIFO-2
            auto resGetAfter = server.handleRequest(reqGet);
            auto rootAfter = codevault::utils::json::parse(resGetAfter.body);
            bool countIs2 = (rootAfter.getInt("count") == 2);

            // 4. DELETE already removed item returns 404
            auto resDelAgain = server.handleRequest(reqDelHead);
            bool delAgain404 = (resDelAgain.statusCode == 404);

            // 5. DELETE nonexistent item returns 404
            codevault::app::HttpRequest reqDelGhost;
            reqDelGhost.method = "DELETE";
            reqDelGhost.path = "/api/practice/queue/Q-NONEXISTENT";
            reqDelGhost.headers["Cookie"] = cookie;
            auto resDelGhost = server.handleRequest(reqDelGhost);
            bool delGhost404 = (resDelGhost.statusCode == 404);

            ok = get200 && countIs3 && delHead200 && countIs2 && delAgain404 && delGhost404;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP Practice API: POST /api/practice/:questionId/result (Verdicts, Queue Unlinking & Revision State)", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_http_result.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8945);

            auto reg = auth->registerUser("verdict_user", "Verdict User", "verdict@test.io", "Pass#12345");
            std::string cookie = "codevault_session=" + reg.sessionToken;

            // Seed questions
            codevault::models::Question q1("Q-RES-1", "Verdict Problem 1", "", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(q1, reg.user->getId());

            codevault::models::Question q2("Q-RES-2", "Verdict Problem 2", "", codevault::models::Topic::Strings,
                codevault::models::Difficulty::Medium, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, reg.user->getId());
            repo->saveForOwner(q2, reg.user->getId());

            qs->setCurrentUserProvider(std::make_shared<codevault::services::AuthenticatedCurrentUserProvider>(*reg.user));
            ps->startSession({"Q-RES-1", "Q-RES-2"});

            // 1. Post verdict: Solved for Q-RES-1
            codevault::app::HttpRequest reqSolved;
            reqSolved.method = "POST";
            reqSolved.path = "/api/practice/Q-RES-1/result";
            reqSolved.headers["Cookie"] = cookie;
            reqSolved.body = "{\"verdict\":\"Solved\"}";
            auto resSolved = server.handleRequest(reqSolved);

            bool solved200 = (resSolved.statusCode == 200);
            auto rootSolved = codevault::utils::json::parse(resSolved.body);
            bool solvedSuccess = rootSolved.getBool("success");
            const auto* qNode = rootSolved.get("question");
            bool qStatusSolved = (qNode != nullptr && qNode->getString("status") == "Solved");
            bool lastPracUpdated = (qNode != nullptr && qNode->getInt("last_practiced_at") > 0);
            const auto* schedNode = rootSolved.get("schedule");
            bool schedPresent = (schedNode != nullptr && schedNode->type != codevault::utils::json::JsonValue::Type::Null);

            // Verify Q-RES-1 was unlinked from queue (queue size becomes 1)
            bool unlinkedOk = (ps->getQueueQuestions().size() == 1);

            // 2. Post verdict: NeedsReview for Q-RES-2
            codevault::app::HttpRequest reqReview;
            reqReview.method = "POST";
            reqReview.path = "/api/practice/Q-RES-2/result";
            reqReview.headers["Cookie"] = cookie;
            reqReview.body = "{\"verdict\":\"NeedsReview\"}";
            auto resReview = server.handleRequest(reqReview);
            bool review200 = (resReview.statusCode == 200);
            auto rootReview = codevault::utils::json::parse(resReview.body);
            const auto* qNode2 = rootReview.get("question");
            bool qStatusInProgress = (qNode2 != nullptr && qNode2->getString("status") == "InProgress");

            // 3. Post invalid verdict returns 400
            codevault::app::HttpRequest reqBadVerdict;
            reqBadVerdict.method = "POST";
            reqBadVerdict.path = "/api/practice/Q-RES-1/result";
            reqBadVerdict.headers["Cookie"] = cookie;
            reqBadVerdict.body = "{\"verdict\":\"SuperSolved\"}";
            auto resBadVerdict = server.handleRequest(reqBadVerdict);
            bool badVerdict400 = (resBadVerdict.statusCode == 400);

            // 4. Post verdict for nonexistent question returns 404
            codevault::app::HttpRequest reqGhost;
            reqGhost.method = "POST";
            reqGhost.path = "/api/practice/Q-NONEXISTENT/result";
            reqGhost.headers["Cookie"] = cookie;
            reqGhost.body = "{\"verdict\":\"Solved\"}";
            auto resGhost = server.handleRequest(reqGhost);
            bool ghost404 = (resGhost.statusCode == 404);

            ok = solved200 && solvedSuccess && qStatusSolved && lastPracUpdated && schedPresent &&
                 unlinkedOk && review200 && qStatusInProgress && badVerdict400 && ghost404;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    runTest("HTTP Practice API: Multi-User Isolation & Cross-User Security Enforcement", [&cleanupDb]() {
        std::string dbPath = "build/test_stage10_http_iso.db";
        cleanupDb(dbPath);

        bool ok = false;
        {
            auto repo = std::make_shared<codevault::persistence::SqliteQuestionRepository>(dbPath);
            auto userRepo = std::make_shared<codevault::persistence::SqliteUserRepository>(dbPath);
            auto sessionRepo = std::make_shared<codevault::persistence::SqliteSessionRepository>(dbPath);
            auto auth = std::make_shared<codevault::services::AuthService>(userRepo, sessionRepo);
            auto rev = std::make_shared<codevault::services::RevisionService>();
            auto qs = std::make_shared<codevault::services::QuestionService>(repo, nullptr, rev);
            auto ps = std::make_shared<codevault::services::PracticeService>(qs, rev);
            auto hist = std::make_shared<codevault::services::RecentHistoryService>();
            auto stats = std::make_shared<codevault::services::StatisticsService>(qs, rev);

            codevault::app::HttpServer server(qs, hist, qs->getSearchService(), rev, ps, stats, auth, 8946);

            auto regAlice = auth->registerUser("alice_api", "Alice", "alice@api.io", "Pass#12345");
            auto regBob = auth->registerUser("bob_api", "Bob", "bob@api.io", "Pass#12345");

            std::string cookieAlice = "codevault_session=" + regAlice.sessionToken;
            std::string cookieBob = "codevault_session=" + regBob.sessionToken;

            // Seed Alice questions
            codevault::models::Question qAlice("Q-ALICE-API", "Alice API Problem", "", codevault::models::Topic::Arrays,
                codevault::models::Difficulty::Easy, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, regAlice.user->getId());
            repo->saveForOwner(qAlice, regAlice.user->getId());

            // Seed Bob questions
            codevault::models::Question qBob("Q-BOB-API", "Bob API Problem", "", codevault::models::Topic::Strings,
                codevault::models::Difficulty::Hard, "", codevault::models::Platform::LeetCode, "",
                codevault::models::Status::Todo, false, "", 100, 100, 0, 0, 2, {}, regBob.user->getId());
            repo->saveForOwner(qBob, regBob.user->getId());

            // Bob enqueues his question in a practice session
            qs->setCurrentUserProvider(std::make_shared<codevault::services::AuthenticatedCurrentUserProvider>(*regBob.user));
            ps->startSession({"Q-BOB-API"});

            // 1. Alice queries her queue: should NOT contain Bob's question
            codevault::app::HttpRequest reqAliceQueue;
            reqAliceQueue.method = "GET";
            reqAliceQueue.path = "/api/practice/queue";
            reqAliceQueue.headers["Cookie"] = cookieAlice;
            auto resAliceQueue = server.handleRequest(reqAliceQueue);
            bool queueIsoOk = (resAliceQueue.statusCode == 200 &&
                               resAliceQueue.body.find("Q-BOB-API") == std::string::npos);

            // 2. Alice attempts to DELETE Bob's queued question: rejected with 404
            codevault::app::HttpRequest reqAliceDelBob;
            reqAliceDelBob.method = "DELETE";
            reqAliceDelBob.path = "/api/practice/queue/Q-BOB-API";
            reqAliceDelBob.headers["Cookie"] = cookieAlice;
            auto resAliceDelBob = server.handleRequest(reqAliceDelBob);
            bool delBobBlocked = (resAliceDelBob.statusCode == 404);

            // 3. Alice attempts to submit a practice result on Bob's question: rejected with 404
            codevault::app::HttpRequest reqAliceResultBob;
            reqAliceResultBob.method = "POST";
            reqAliceResultBob.path = "/api/practice/Q-BOB-API/result";
            reqAliceResultBob.headers["Cookie"] = cookieAlice;
            reqAliceResultBob.body = "{\"verdict\":\"Solved\"}";
            auto resAliceResultBob = server.handleRequest(reqAliceResultBob);
            bool resultBobBlocked = (resAliceResultBob.statusCode == 404);

            // Invariant: Bob's question remains Todo in database
            auto bobQInDb = repo->findById("Q-BOB-API");
            bool bobStillTodo = (bobQInDb.has_value() && bobQInDb->getStatus() == codevault::models::Status::Todo &&
                                 bobQInDb->getLastPracticedAt() == 0);

            // 4. Practice Next isolation: Alice's recommendation only considers Alice's problem
            codevault::app::HttpRequest reqAliceNext;
            reqAliceNext.method = "GET";
            reqAliceNext.path = "/api/practice/next";
            reqAliceNext.headers["Cookie"] = cookieAlice;
            auto resAliceNext = server.handleRequest(reqAliceNext);
            auto rootAliceNext = codevault::utils::json::parse(resAliceNext.body);
            bool nextIsoOk = (resAliceNext.statusCode == 200 && rootAliceNext.getBool("hasQuestion") &&
                              rootAliceNext.get("question") &&
                              rootAliceNext.get("question")->getString("id") == "Q-ALICE-API");

            ok = queueIsoOk && delBobBlocked && resultBobBlocked && bobStillTodo && nextIsoOk;
            repo->close();
        }

        cleanupDb(dbPath);
        return ok;
    });

    // =========================================================================
    // Summary
    // =========================================================================
    std::cout << "\n==================================================\n";
    std::cout << "Unit Test Results: " << g_passedTests << " / " << g_totalTests << " passed.\n";
    std::cout << "==================================================\n";

    return (g_passedTests == g_totalTests) ? 0 : 1;
}
