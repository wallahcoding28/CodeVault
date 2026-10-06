#include "persistence/sqlite_user_repository.hpp"
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

SqliteUserRepository::SqliteUserRepository(std::string dbPath)
    : dbPath_(std::move(dbPath)) {
    initialize();
}

SqliteUserRepository::~SqliteUserRepository() {
    close();
}

SqliteUserRepository::SqliteUserRepository(SqliteUserRepository&& other) noexcept {
    LockGuard lock(other.dbMutex_);
    dbPath_ = std::move(other.dbPath_);
    db_ = other.db_;
    other.db_ = nullptr;
}

SqliteUserRepository& SqliteUserRepository::operator=(SqliteUserRepository&& other) noexcept {
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

void SqliteUserRepository::close() {
    LockGuard lock(dbMutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SqliteUserRepository::isOpen() const noexcept {
    LockGuard lock(dbMutex_);
    return db_ != nullptr;
}

bool SqliteUserRepository::initialize() {
    LockGuard lock(dbMutex_);
    if (db_) return true;

    if (!openDatabase()) return false;
    if (!executePragmas()) return false;
    if (!createSchema()) return false;

    return true;
}

bool SqliteUserRepository::openDatabase() {
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

bool SqliteUserRepository::executePragmas() {
    if (!db_) return false;

    const char* pragmas =
        "PRAGMA journal_mode = WAL;"
        "PRAGMA synchronous = NORMAL;"
        "PRAGMA foreign_keys = ON;"
        "PRAGMA busy_timeout = 5000;";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, pragmas, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool SqliteUserRepository::createSchema() {
    if (!db_) return false;

    const char* ddl =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id TEXT PRIMARY KEY NOT NULL,"
        "  username TEXT UNIQUE NOT NULL,"
        "  display_name TEXT NOT NULL,"
        "  email TEXT NOT NULL DEFAULT '',"
        "  created_at INTEGER NOT NULL DEFAULT 0,"
        "  updated_at INTEGER NOT NULL DEFAULT 0,"
        "  active INTEGER NOT NULL DEFAULT 1"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);"
        "CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);"
        "INSERT OR IGNORE INTO users (id, username, display_name, email, created_at, updated_at, active) "
        "VALUES ('local_user', 'local_user', 'Local User', '', 1774000000, 1774000000, 1);"
        "CREATE TABLE IF NOT EXISTS user_credentials ("
        "  user_id TEXT PRIMARY KEY NOT NULL,"
        "  password_hash TEXT NOT NULL,"
        "  created_at INTEGER NOT NULL,"
        "  updated_at INTEGER NOT NULL,"
        "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, ddl, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    return true;
}

models::User SqliteUserRepository::rowToUser(sqlite3_stmt* stmt) const {
    std::string id = getColumnText(stmt, 0);
    std::string username = getColumnText(stmt, 1);
    std::string displayName = getColumnText(stmt, 2);
    std::string email = getColumnText(stmt, 3);
    int64_t createdAt = sqlite3_column_int64(stmt, 4);
    int64_t updatedAt = sqlite3_column_int64(stmt, 5);
    bool active = sqlite3_column_int(stmt, 6) != 0;

    return models::User(id, username, displayName, email, createdAt, updatedAt, active);
}

bool SqliteUserRepository::createUser(const models::User& user) {
    if (!user.isValid()) return false;

    LockGuard lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "INSERT INTO users (id, username, display_name, email, created_at, updated_at, active) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t createdAt = user.getCreatedAt() > 0 ? user.getCreatedAt() : now;
    int64_t updatedAt = user.getUpdatedAt() > 0 ? user.getUpdatedAt() : now;

    sqlite3_bind_text(stmt, 1, user.getId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.getUsername().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.getDisplayName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, user.getEmail().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, createdAt);
    sqlite3_bind_int64(stmt, 6, updatedAt);
    sqlite3_bind_int(stmt, 7, user.isActive() ? 1 : 0);

    return sqlite3_step(stmt) == SQLITE_DONE;
}

std::optional<models::User> SqliteUserRepository::findById(const std::string& id) const {
    LockGuard lock(dbMutex_);
    if (!db_ || id.empty()) return std::nullopt;

    const char* sql =
        "SELECT id, username, display_name, email, created_at, updated_at, active "
        "FROM users WHERE id = ?;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return rowToUser(stmt);
    }

    return std::nullopt;
}

std::optional<models::User> SqliteUserRepository::findByUsername(const std::string& username) const {
    LockGuard lock(dbMutex_);
    if (!db_ || username.empty()) return std::nullopt;

    const char* sql =
        "SELECT id, username, display_name, email, created_at, updated_at, active "
        "FROM users WHERE username = ?;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return rowToUser(stmt);
    }

    return std::nullopt;
}

std::optional<models::User> SqliteUserRepository::findByEmail(const std::string& email) const {
    LockGuard lock(dbMutex_);
    if (!db_ || email.empty()) return std::nullopt;

    const char* sql =
        "SELECT id, username, display_name, email, created_at, updated_at, active "
        "FROM users WHERE email = ?;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return rowToUser(stmt);
    }

    return std::nullopt;
}

bool SqliteUserRepository::updateUser(const models::User& user) {
    if (!user.isValid()) return false;

    LockGuard lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "UPDATE users SET username = ?, display_name = ?, email = ?, updated_at = ?, active = ? "
        "WHERE id = ?;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t updatedAt = user.getUpdatedAt() > 0 ? user.getUpdatedAt() : now;

    sqlite3_bind_text(stmt, 1, user.getUsername().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.getDisplayName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.getEmail().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, updatedAt);
    sqlite3_bind_int(stmt, 5, user.isActive() ? 1 : 0);
    sqlite3_bind_text(stmt, 6, user.getId().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        return false;
    }

    return sqlite3_changes(db_) > 0;
}

bool SqliteUserRepository::exists(const std::string& id) const {
    LockGuard lock(dbMutex_);
    if (!db_ || id.empty()) return false;

    const char* sql = "SELECT 1 FROM users WHERE id = ? LIMIT 1;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(stmt) == SQLITE_ROW;
}

std::vector<models::User> SqliteUserRepository::findAll() const {
    LockGuard lock(dbMutex_);
    std::vector<models::User> users;
    if (!db_) return users;

    const char* sql =
        "SELECT id, username, display_name, email, created_at, updated_at, active "
        "FROM users ORDER BY created_at ASC;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return users;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        users.push_back(rowToUser(stmt));
    }

    return users;
}

size_t SqliteUserRepository::count() const {
    LockGuard lock(dbMutex_);
    if (!db_) return 0;

    const char* sql = "SELECT COUNT(*) FROM users;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return static_cast<size_t>(sqlite3_column_int64(stmt, 0));
    }

    return 0;
}

bool SqliteUserRepository::deactivateUser(const std::string& id) {
    LockGuard lock(dbMutex_);
    if (!db_ || id.empty()) return false;

    const char* sql = "UPDATE users SET active = 0 WHERE id = ?;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        return false;
    }

    return sqlite3_changes(db_) > 0;
}

bool SqliteUserRepository::saveCredentials(const models::UserCredentials& credentials) {
    if (!credentials.isValid()) return false;
    LockGuard lock(dbMutex_);
    if (!db_) return false;

    const char* sql =
        "INSERT INTO user_credentials (user_id, password_hash, created_at, updated_at) "
        "VALUES (?, ?, ?, ?) "
        "ON CONFLICT(user_id) DO UPDATE SET "
        "  password_hash = excluded.password_hash,"
        "  updated_at = excluded.updated_at;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int64_t now = credentials.getUpdatedAt() > 0 ? credentials.getUpdatedAt() :
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t created = credentials.getCreatedAt() > 0 ? credentials.getCreatedAt() : now;

    sqlite3_bind_text(stmt, 1, credentials.getUserId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, credentials.getPasswordHash().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, created);
    sqlite3_bind_int64(stmt, 4, now);

    return sqlite3_step(stmt) == SQLITE_DONE;
}

std::optional<models::UserCredentials> SqliteUserRepository::getCredentials(const std::string& userId) const {
    if (userId.empty()) return std::nullopt;
    LockGuard lock(dbMutex_);
    if (!db_) return std::nullopt;

    const char* sql = "SELECT user_id, password_hash, created_at, updated_at FROM user_credentials WHERE user_id = ?;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string uid = getColumnText(stmt, 0);
        std::string hash = getColumnText(stmt, 1);
        int64_t created = sqlite3_column_int64(stmt, 2);
        int64_t updated = sqlite3_column_int64(stmt, 3);
        return models::UserCredentials(uid, hash, created, updated);
    }

    return std::nullopt;
}

bool SqliteUserRepository::deleteCredentials(const std::string& userId) {
    if (userId.empty()) return false;
    LockGuard lock(dbMutex_);
    if (!db_) return false;

    const char* sql = "DELETE FROM user_credentials WHERE user_id = ?;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(stmt) == SQLITE_DONE;
}

} // namespace codevault::persistence
