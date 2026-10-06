#pragma once

#include "models/import_result.hpp"
#include "models/question.hpp"
#include "services/question_service.hpp"

#include <memory>
#include <string>
#include <vector>

namespace codevault::services {

/**
 * @brief Application service handling bulk import of question catalogs (JSON and CSV)
 *        with configurable conflict strategies, atomic transactions, owner isolation,
 *        and DSA index synchronization.
 */
class ImportService {
public:
    explicit ImportService(std::shared_ptr<QuestionService> questionService);

    /**
     * @brief Import questions from a JSON string payload.
     * @param jsonContent JSON string (either an array of questions or an object with "questions" array).
     * @param strategy Conflict strategy for existing IDs (Skip, Overwrite, GenerateNewId).
     * @return ImportResult with success flag, counters, and error diagnostics.
     */
    models::ImportResult importFromJson(
        const std::string& jsonContent,
        models::ImportConflictStrategy strategy = models::ImportConflictStrategy::Skip);

    /**
     * @brief Import questions from an RFC 4180 compliant CSV string payload.
     * @param csvContent CSV string with standard CodeVault header.
     * @param strategy Conflict strategy for existing IDs (Skip, Overwrite, GenerateNewId).
     * @return ImportResult with success flag, counters, and error diagnostics.
     */
    models::ImportResult importFromCsv(
        const std::string& csvContent,
        models::ImportConflictStrategy strategy = models::ImportConflictStrategy::Skip);

    /**
     * @brief Import an in-memory batch of questions.
     * @param questions Vector of parsed Question domain models.
     * @param strategy Conflict strategy for existing IDs.
     * @return ImportResult with success flag, counters, and error diagnostics.
     */
    models::ImportResult importQuestions(
        const std::vector<models::Question>& questions,
        models::ImportConflictStrategy strategy = models::ImportConflictStrategy::Skip);

    /**
     * @brief Utility parser for RFC 4180 CSV strings into raw row vectors.
     *        Handles multiline quoted text, escaped quotes, and commas inside quotes.
     */
    static std::vector<std::vector<std::string>> parseRfc4180Csv(
        const std::string& input,
        std::string& outError);

private:
    std::shared_ptr<QuestionService> questionService_;

    // Helper to generate next unique sequential ID with batch offset awareness
    std::string generateUniqueId(int64_t& inMemoryMaxNum) const;
};

} // namespace codevault::services
