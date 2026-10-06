#pragma once

#include "persistence/iquestion_repository.hpp"

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Forward declaration of sqlite3 structures to keep header clean
struct sqlite3;
struct sqlite3_stmt;

namespace codevault::persistence {

/**
 * @brief SQLite-backed implementation of IQuestionRepository.
 *
 * Provides transactional, ACID-compliant persistence for CodeVault domain models,
 * replacing the flat CSV repository while maintaining identical domain contracts.
 */
class SqliteQuestionRepository : public IQuestionRepository {
public:
    /**
     * @brief Construct repository associated with a specific SQLite database path.
     * @param dbPath Path to the .db file (e.g. "data/codevault.db"). If directory does not exist,
     *               it will be created automatically upon initialization.
     */
    explicit SqliteQuestionRepository(std::string dbPath);

    /**
     * @brief Destructor. Safely finalizes statements and closes database connection.
     */
    ~SqliteQuestionRepository() override;

    // Non-copyable, movable
    SqliteQuestionRepository(const SqliteQuestionRepository&) = delete;
    SqliteQuestionRepository& operator=(const SqliteQuestionRepository&) = delete;
    SqliteQuestionRepository(SqliteQuestionRepository&&) noexcept;
    SqliteQuestionRepository& operator=(SqliteQuestionRepository&&) noexcept;

    // --- IQuestionRepository Interface ---
    std::vector<models::Question> findAll() const override;
    std::optional<models::Question> findById(const std::string& id) const override;
    bool exists(const std::string& id) const override;
    bool save(const models::Question& question) override;
    bool remove(const std::string& id) override;
    size_t count() const override;

    // --- Owner-Scoped Operations ---
    std::vector<models::Question> findAllByOwner(const std::string& ownerId) const override;
    std::optional<models::Question> findByIdForOwner(const std::string& id, const std::string& ownerId) const override;
    bool existsForOwner(const std::string& id, const std::string& ownerId) const override;
    bool saveForOwner(const models::Question& question, const std::string& ownerId) override;
    bool removeForOwner(const std::string& id, const std::string& ownerId) override;
    size_t countForOwner(const std::string& ownerId) const override;

    // --- Database Lifecycle & Diagnostics ---
    /**
     * @brief Initialize database connection, pragmas, schema tables, and indexes.
     * @return true on success, false otherwise.
     */
    bool initialize();

    /**
     * @brief Close database connection safely.
     */
    void close();

    /**
     * @brief Check if database is currently open and valid.
     */
    bool isOpen() const noexcept;

    /**
     * @brief Path to the SQLite database file.
     */
    const std::string& getDbPath() const noexcept { return dbPath_; }

    /**
     * @brief Retrieve current applied schema version from `schema_version` table.
     */
    int getSchemaVersion() const;

    /**
     * @brief Retrieve database file size in bytes from disk.
     */
    int64_t getDatabaseSizeBytes() const;

    // --- Transaction Management ---
    /**
     * @brief Begin an explicit SQLite transaction.
     */
    bool beginTransaction();

    /**
     * @brief Commit active SQLite transaction.
     */
    bool commitTransaction();

    /**
     * @brief Rollback active SQLite transaction.
     */
    bool rollbackTransaction();

    /**
     * @brief Execute a multi-step operation within an atomic transaction.
     *        Automatically commits if action returns true, rolls back if false or on exception.
     */
    bool executeTransaction(const std::function<bool()>& action);

private:
    std::string dbPath_;
    sqlite3* db_{nullptr};
    mutable std::recursive_mutex dbMutex_;

    // Internal initialization helpers (called with lock held or in constructor)
    bool openDatabase();
    bool executePragmas();
    bool createSchema();
    bool recordSchemaVersion(int version, const std::string& description);

    // Row mapping helpers
    models::Question rowToQuestion(sqlite3_stmt* stmt) const;

    // Internal transaction helpers (assumes dbMutex_ held)
    bool beginTransactionInternal();
    bool commitTransactionInternal();
    bool rollbackTransactionInternal();
};

} // namespace codevault::persistence
