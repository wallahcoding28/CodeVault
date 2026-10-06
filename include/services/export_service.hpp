#pragma once

#include "models/question.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace codevault::services {

/**
 * @brief Service providing deterministic export of question catalogs into multiple formats:
 *        JSON, RFC 4180 CSV, GitHub-friendly Markdown, and Anki-compatible TSV flashcards.
 *        Strictly uses real domain fields with zero invented content.
 */
class ExportService {
public:
    ExportService() = default;

    /**
     * @brief Export questions to a formatted JSON string.
     * @param questions Vector of questions to export.
     * @param exportedAtEpoch Timestamp to embed in metadata (0 uses system time).
     */
    static std::string exportToJson(const std::vector<models::Question>& questions, int64_t exportedAtEpoch = 0);

    /**
     * @brief Export questions to RFC 4180 compliant CSV string with 18 standard headers.
     * @param questions Vector of questions to export.
     */
    static std::string exportToCsv(const std::vector<models::Question>& questions);

    /**
     * @brief Export questions to a deterministic GitHub-flavored Markdown problem journal.
     * @param questions Vector of questions to export.
     * @param exportedAtEpoch Timestamp to embed in metadata (0 uses system time).
     */
    static std::string exportToMarkdown(const std::vector<models::Question>& questions, int64_t exportedAtEpoch = 0);

    /**
     * @brief Export questions to Anki-compatible tab-separated values (TSV) deck.
     *        Columns: Front (Title & Metadata) \t Back (Description, Notes & URL) \t Tags
     *        Guarantees single-line rows with newline and tab escaping to prevent column shift.
     * @param questions Vector of questions to export.
     */
    static std::string exportToAnkiTsv(const std::vector<models::Question>& questions);
};

} // namespace codevault::services
