#pragma once

#include "persistence/sqlite_question_repository.hpp"

#include <string>
#include <vector>

namespace codevault::persistence {

struct MigrationReport {
    bool success{false};
    size_t csvRecordsRead{0};
    size_t validRecordsFound{0};
    size_t malformedRowsCount{0};
    size_t importedCount{0};
    size_t skippedExistingCount{0};
    size_t finalDatabaseCount{0};
    std::string csvPath;
    std::string backupPath;
    std::string dbPath;
    std::string message;
    std::vector<std::string> warnings;
};

/**
 * @brief Service responsible for safely migrating flat CSV datasets to SQLite.
 */
class MigrationService {
public:
    /**
     * @brief Create a backup of the source CSV file before migration.
     * @param csvPath Path to original CSV file.
     * @param backupPath Destination path for backup (defaults to csvPath + ".bak").
     * @return true if backup succeeded or already existed, false on error.
     */
    static bool createCsvBackup(const std::string& csvPath, const std::string& backupPath = "");

    /**
     * @brief Migrate questions from a CSV file into a SQLite repository.
     *
     * Performs atomic transactional ingestion, validates record counts, preserves fields,
     * and ensures idempotence (does not duplicate records if already migrated).
     *
     * @param csvPath Path to source CSV.
     * @param repository Target SQLite repository.
     * @param forceReimport If true, updates existing records; if false, skips existing IDs.
     * @return MigrationReport Detailed report of the migration outcome.
     */
    static MigrationReport migrateCsvToSqlite(
        const std::string& csvPath,
        SqliteQuestionRepository& repository,
        bool forceReimport = false
    );
};

} // namespace codevault::persistence
