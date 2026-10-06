#pragma once

#include "persistence/i_session_repository.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

namespace codevault::persistence {

/**
 * @brief SQLite-backed implementation of ISessionRepository.
 *
 * Persists session tokens safely using token hashes.
 * Enforces foreign key constraints to users table.
 */
class SqliteSessionRepository : public ISessionRepository {
public:
    explicit SqliteSessionRepository(std::string dbPath);
    ~SqliteSessionRepository() override;

    SqliteSessionRepository(const SqliteSessionRepository&) = delete;
    SqliteSessionRepository& operator=(const SqliteSessionRepository&) = delete;
    SqliteSessionRepository(SqliteSessionRepository&&) noexcept;
    SqliteSessionRepository& operator=(SqliteSessionRepository&&) noexcept;

    // --- ISessionRepository Interface ---
    bool createSession(const models::Session& session) override;
    std::optional<models::Session> findByTokenHash(const std::string& tokenHash) const override;
    std::optional<models::Session> findById(const std::string& sessionId) const override;
    bool revokeSession(const std::string& sessionId, int64_t revokedAt) override;
    bool revokeAllForUser(const std::string& userId, int64_t revokedAt) override;
    bool revokeAllForUserExcept(const std::string& userId, const std::string& exceptSessionId, int64_t revokedAt) override;
    std::vector<models::Session> findActiveSessionsForUser(const std::string& userId, int64_t now) const override;
    bool updateLastSeen(const std::string& sessionId, int64_t lastSeenAt) override;
    size_t deleteExpiredSessions(int64_t now) override;

    // --- Lifecycle ---
    bool initialize();
    void close();
    bool isOpen() const noexcept;
    const std::string& getDbPath() const noexcept { return dbPath_; }

private:
    std::string dbPath_;
    sqlite3* db_{nullptr};
    mutable std::recursive_mutex dbMutex_;

    bool openDatabase();
    bool executePragmas();
    bool createSchema();

    models::Session rowToSession(sqlite3_stmt* stmt) const;
};

} // namespace codevault::persistence
