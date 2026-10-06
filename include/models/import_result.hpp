#pragma once

#include <string>
#include <vector>

namespace codevault::models {

/**
 * @brief Strategy for resolving ID conflicts during catalog import.
 */
enum class ImportConflictStrategy {
    Skip,
    Overwrite,
    GenerateNewId
};

/**
 * @brief Convert conflict strategy enum to string representation.
 */
inline std::string conflictStrategyToString(ImportConflictStrategy strategy) {
    switch (strategy) {
        case ImportConflictStrategy::Skip: return "skip";
        case ImportConflictStrategy::Overwrite: return "overwrite";
        case ImportConflictStrategy::GenerateNewId: return "generate_new_id";
    }
    return "skip";
}

/**
 * @brief Parse conflict strategy from string. Returns true if valid, false otherwise.
 */
inline bool stringToConflictStrategy(const std::string& str, ImportConflictStrategy& out) {
    if (str == "skip" || str == "Skip") {
        out = ImportConflictStrategy::Skip;
        return true;
    }
    if (str == "overwrite" || str == "Overwrite") {
        out = ImportConflictStrategy::Overwrite;
        return true;
    }
    if (str == "generate_new_id" || str == "generateNewId" || str == "GenerateNewId") {
        out = ImportConflictStrategy::GenerateNewId;
        return true;
    }
    return false;
}

/**
 * @brief Represents the outcome of a catalog batch import operation.
 */
struct ImportResult {
    bool success{false};
    size_t importedCount{0};
    size_t updatedCount{0};
    size_t skippedCount{0};
    size_t totalProcessed{0};
    std::vector<std::string> errors;

    bool hasErrors() const noexcept { return !errors.empty(); }
};

} // namespace codevault::models
