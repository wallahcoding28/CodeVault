#include "persistence/sqlite_session_repository.hpp"
#include "sqlite3.h"

#include <chrono>
#include <filesystem>

namespace codevault::persistence {

using LockGuard = std::lock_guard<std::recursive_mutex>;
using UniqueLock = std::unique_lock<std::recursive_mutex>;

namespace {

struct ScopedStmt {
    sqlite3_stmt* stmt{nullptr};
    ~ScopedStmt() {
        if (stmt) {
            sqlite3_finalize(stmt);
        }
    }
    sqlite3_stmt** operator&() { return &stmt; }
    operator sqlite3_stmt*() const { return stmt; }
};

std::string getColumnText(sqlite3_stmt* stmt, int col) {
    const unsigned char* text = sqlite3_column_text(stmt, col);
    return text ? reinterpret_cast<const char*>(text) : "";
}

} // namespace

SqliteSessionRepository::SqliteSessionRepository(std::string dbPath)
    : dbPath_(std::move(dbPath)) {
    initialize();
}

SqliteSessionRepository::~SqliteSessionRepository() {
    close();
}

SqliteSessionRepository::SqliteSessionRepository(SqliteSessionRepository&& other) noexcept {
    LockGuard lock(other.dbMutex_);
    dbPath_ = std::move(other.dbPath_);
    db_ = other.db_;
    other.db_ = nullptr;
}

SqliteSessionRepository& SqliteSessionRepository::operator=(SqliteSessionRepository&& other) noexcept {
    if (this != &other) {
        UniqueLock lockThis(dbMutex_, std::defer_lock);
        UniqueLock lockOther(other.dbMutex_, std::defer_lock);
        std::lock(lockThis, lockOther);

        close();
        dbPath_ = std::move(other.dbPath_);
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

void SqliteSessionRepository::close() {
    LockGuard lock(dbMutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SqliteSessionRepository::isOpen() const noexcept {
    LockGuard lock(dbMutex_);
    return db_ != nullptr;
}

bool SqliteSessionRepository::initialize() {
    LockGuard lock(dbMutex_);
    if (db_) return true;

    if (!openDatabase()) return false;
    if (!executePragmas()) return false;
    if (!createSchema()) return false;

    return true;
}

bool SqliteSessionRepository::openDatabase() {
    if (dbPath_.empty()) return false;

    std::filesystem::path p(dbPath_);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    int rc = sqlite3_open_v2(dbPath_.c_str(), &db_, flags, nullptr);
    if (rc != SQLITE_OK) {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        return false;
    }
    return true;
}

bool SqliteSessionRepository::executePragmas() {
    if (!db_) return false;

    char* errMsg = nullptr;
    const char* pragmaSql =
        "PRAGMA journal_mode = WAL;"
        "PRAGMA synchronous = NORMAL;"
        "PRAGMA foreign_keys = ON;";

    int rc = sqlite3_exec(db_, pragmaSql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    sqlite3_busy_timeout(db_, 5000);
    return true;
}

bool SqliteSessionRepository::createSchema() {
    if (!db_) return false;

    const char* ddl =
        "CREATE TABLE IF NOT EXISTS sessions ("
        "  id TEXT PRIMARY KEY NOT NULL,"
        "  user_id TEXT NOT NULL,"
        "  token_hash TEXT NOT NULL,"
        "  created_at INTEGER NOT NULL,"
        "  expires_at INTEGER NOT NULL,"
        "  revoked_at INTEGER NOT NULL DEFAULT 0,"
        "  last_seen_at INTEGER NOT NULL DEFAULT 0,"
        "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_sessions_token_hash ON sessions(token_hash);"
        "CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id);";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, ddl, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    return true;
}

models::Session SqliteSessionRepository::rowToSession(sqlite3_stmt* stmt) const {
    std::string id = getColumnText(stmt, 0);
    std::string userId = getColumnText(stmt, 1);
    std::string tokenHash = getColumnText(stmt, 2);
    int64_t createdAt = sqlite3_column_int64(stmt, 3);
    int64_t expiresAt = sqlite3_column_int64(stmt, 4);
    int64_t revokedAt = sqlite3_column_int64(stmt, 5);
    int64_t lastSeenAt = sqlite3_column_int64(stmt, 6);

    return models::Session(id, userId, tokenHash, createdAt, expiresAt, revokedAt, lastSeenAt);
}

bool SqliteSessionRepository::createSession(const models::Session& session) {
    if (session.getId().empty() || session.getUserId().empty() || session.getTokenHash().empty()) {
        return false;
    }

    LockGuard lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "INSERT INTO sessions (id, user_id, token_hash, created_at, expires_at, revoked_at, last_seen_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, session.getId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, session.getUserId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, session.getTokenHash().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, session.getCreatedAt());
    sqlite3_bind_int64(stmt, 5, session.getExpiresAt());
    sqlite3_bind_int64(stmt, 6, session.getRevokedAt());
    sqlite3_bind_int64(stmt, 7, session.getLastSeenAt());

    return sqlite3_step(stmt) == SQLITE_DONE;
}

std::optional<models::Session> SqliteSessionRepository::findByTokenHash(const std::string& tokenHash) const {
    if (tokenHash.empty()) return std::nullopt;

    LockGuard lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql =
        "SELECT id, user_id, token_hash, created_at, expires_at, revoked_at, last_seen_at "
        "FROM sessions WHERE token_hash = ? LIMIT 1;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, tokenHash.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return rowToSession(stmt);
    }

    return std::nullopt;
}

std::optional<models::Session> SqliteSessionRepository::findById(const std::string& sessionId) const {
    if (sessionId.empty()) return std::nullopt;

    LockGuard lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql =
        "SELECT id, user_id, token_hash, created_at, expires_at, revoked_at, last_seen_at "
        "FROM sessions WHERE id = ? LIMIT 1;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return rowToSession(stmt);
    }

    return std::nullopt;
}

bool SqliteSessionRepository::revokeSession(const std::string& sessionId, int64_t revokedAt) {
    if (sessionId.empty()) return false;

    LockGuard lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "UPDATE sessions SET revoked_at = ? WHERE id = ?;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, revokedAt);
    sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        return false;
    }

    return sqlite3_changes(db_) > 0;
}

bool SqliteSessionRepository::revokeAllForUser(const std::string& userId, int64_t revokedAt) {
    if (userId.empty()) return false;

    LockGuard lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "UPDATE sessions SET revoked_at = ? WHERE user_id = ? AND revoked_at = 0;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, revokedAt);
    sqlite3_bind_text(stmt, 2, userId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        return false;
    }

    return sqlite3_changes(db_) > 0;
}

bool SqliteSessionRepository::revokeAllForUserExcept(const std::string& userId, const std::string& exceptSessionId, int64_t revokedAt) {
    if (userId.empty()) return false;

    LockGuard lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "UPDATE sessions SET revoked_at = ? WHERE user_id = ? AND id != ? AND revoked_at = 0;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, revokedAt);
    sqlite3_bind_text(stmt, 2, userId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, exceptSessionId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        return false;
    }

    return sqlite3_changes(db_) > 0;
}

std::vector<models::Session> SqliteSessionRepository::findActiveSessionsForUser(const std::string& userId, int64_t now) const {
    std::vector<models::Session> sessions;
    if (userId.empty()) return sessions;

    LockGuard lock(dbMutex_);
    if (!db_) return sessions;

    const char* sql =
        "SELECT id, user_id, token_hash, created_at, expires_at, revoked_at, last_seen_at "
        "FROM sessions WHERE user_id = ? AND revoked_at = 0 AND expires_at > ? "
        "ORDER BY last_seen_at DESC, created_at DESC;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return sessions;
    }

    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, now);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        sessions.push_back(rowToSession(stmt));
    }

    return sessions;
}

bool SqliteSessionRepository::updateLastSeen(const std::string& sessionId, int64_t lastSeenAt) {
    if (sessionId.empty()) return false;

    LockGuard lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "UPDATE sessions SET last_seen_at = ? WHERE id = ?;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, lastSeenAt);
    sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);

    return sqlite3_step(stmt) == SQLITE_DONE;
}

size_t SqliteSessionRepository::deleteExpiredSessions(int64_t now) {
    LockGuard lock(dbMutex_);
    if (!db_) return 0;

    const char* sql = "DELETE FROM sessions WHERE expires_at <= ? OR (revoked_at > 0 AND revoked_at <= ?);";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_int64(stmt, 2, now);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        return 0;
    }

    return static_cast<size_t>(sqlite3_changes(db_));
}

} // namespace codevault::persistence
