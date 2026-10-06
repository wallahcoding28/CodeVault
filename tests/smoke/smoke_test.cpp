#include "models/question.hpp"
#include "persistence/file_question_repository.hpp"
#include "services/question_service.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    std::string samplePath = "data/sample/questions.csv";
    if (argc > 1) {
        samplePath = argv[1];
    }

    std::cout << "========================================\n";
    std::cout << "     CodeVault Stage 1 Smoke Test       \n";
    std::cout << "========================================\n\n";

    int passed = 0;
    int total = 0;

    auto runTest = [&](const std::string& name, auto testFunc) {
        ++total;
        std::cout << "[TEST " << total << "] " << name << " ... ";
        try {
            if (testFunc()) {
                std::cout << "PASSED\n";
                ++passed;
            } else {
                std::cout << "FAILED\n";
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (Exception: " << e.what() << ")\n";
        }
    };

    // Test 1: Model creation and validation
    runTest("Question Entity Invariants", [&]() {
        codevault::models::Question q(
            "T-1", "Test Question", "Description",
            codevault::models::Topic::Arrays,
            codevault::models::Difficulty::Easy,
            "Google",
            codevault::models::Platform::LeetCode,
            "https://leetcode.com/problems/test",
            codevault::models::Status::Todo,
            true, "Some notes",
            100, 100, 0, 200, 1,
            {"tag1", "tag2"}
        );
        return q.isValid() &&
               q.getId() == "T-1" &&
               q.getTitle() == "Test Question" &&
               q.isFavorite() &&
               q.getTags().size() == 2;
    });

    // Test 2: Repository loading from CSV
    auto repo = std::make_shared<codevault::persistence::FileQuestionRepository>(samplePath);
    runTest("Repository CSV Ingestion", [&]() {
        size_t count = repo->count();
        std::cout << "(Loaded " << count << " records) ";
        return count >= 8; // We expect at least the 9 sample questions
    });

    // Test 3: Lookup specific sample question (Two Sum)
    runTest("Lookup 'Two Sum' by ID", [&]() {
        auto qOpt = repo->findById("Q-1001");
        if (!qOpt.has_value()) return false;
        const auto& q = qOpt.value();
        return q.getTitle() == "Two Sum" &&
               q.getTopic() == codevault::models::Topic::Arrays &&
               q.getDifficulty() == codevault::models::Difficulty::Easy &&
               q.getStatus() == codevault::models::Status::Solved;
    });

    // Test 4: QuestionService orchestration & metrics
    auto service = std::make_shared<codevault::services::QuestionService>(repo);
    runTest("QuestionService Aggregation", [&]() {
        size_t totalCount = service->getQuestionCount();
        size_t easyCount = service->countByDifficulty(codevault::models::Difficulty::Easy);
        size_t mediumCount = service->countByDifficulty(codevault::models::Difficulty::Medium);
        size_t solvedCount = service->countByStatus(codevault::models::Status::Solved);

        std::cout << "(Total=" << totalCount
                  << ", Easy=" << easyCount
                  << ", Medium=" << mediumCount
                  << ", Solved=" << solvedCount << ") ";

        return totalCount >= 8 && easyCount > 0 && mediumCount > 0 && solvedCount > 0;
    });

    // Test 5: Dynamic addition and deletion via service
    runTest("Service Save & Delete Lifecycle", [&]() {
        codevault::models::Question temp(
            "Q-TEMP", "Temporary Smoke Problem", "Desc",
            codevault::models::Topic::Heaps,
            codevault::models::Difficulty::Hard,
            "Netflix",
            codevault::models::Platform::Custom,
            "",
            codevault::models::Status::Todo,
            false, "Temp notes",
            100, 100, 0, 0, 2, {"heap"}
        );

        size_t beforeCount = service->getQuestionCount();
        bool saved = service->saveQuestion(temp);
        if (!saved || service->getQuestionCount() != beforeCount + 1) return false;

        auto retrieved = service->getQuestionById("Q-TEMP");
        if (!retrieved.has_value() || retrieved->getTitle() != "Temporary Smoke Problem") return false;

        bool deleted = service->deleteQuestion("Q-TEMP");
        if (!deleted || service->getQuestionCount() != beforeCount) return false;

        return true;
    });

    std::cout << "\n----------------------------------------\n";
    std::cout << "Results: " << passed << " / " << total << " tests passed.\n";
    std::cout << "----------------------------------------\n";

    return (passed == total) ? 0 : 1;
}
