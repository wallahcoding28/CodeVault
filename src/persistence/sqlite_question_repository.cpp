#include "persistence/sqlite_question_repository.hpp"
#include "sqlite3.h"

#include <chrono>
#include <filesystem>
#include <sstream>

namespace codevault::persistence {

using LockGuard = std::lock_guard<std::recursive_mutex>;
using UniqueLock = std::unique_lock<std::recursive_mutex>;

namespace {

// RAII helper to ensure sqlite3_stmt is always finalized
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

std::string serializeTags(const std::vector<std::string>& tags) {
    std::string result;
    for (size_t i = 0; i < tags.size(); ++i) {
        result += tags[i];
        if (i + 1 < tags.size()) {
            result += ';';
        }
    }
    return result;
}

std::vector<std::string> deserializeTags(const std::string& tagsStr) {
    std::vector<std::string> tags;
    std::stringstream ss(tagsStr);
    std::string token;
    while (std::getline(ss, token, ';')) {
        if (!token.empty()) {
            tags.push_back(token);
        }
    }
    return tags;
}

std::string getColumnText(sqlite3_stmt* stmt, int col) {
    const unsigned char* text = sqlite3_column_text(stmt, col);
    if (!text) {
        return "";
    }
    return reinterpret_cast<const char*>(text);
}

} // namespace

SqliteQuestionRepository::SqliteQuestionRepository(std::string dbPath)
    : dbPath_(std::move(dbPath)) {
    initialize();
}

SqliteQuestionRepository::~SqliteQuestionRepository() {
    close();
}

SqliteQuestionRepository::SqliteQuestionRepository(SqliteQuestionRepository&& other) noexcept {
    LockGuard lock(other.dbMutex_);
    dbPath_ = std::move(other.dbPath_);
    db_ = other.db_;
    other.db_ = nullptr;
}

SqliteQuestionRepository& SqliteQuestionRepository::operator=(SqliteQuestionRepository&& other) noexcept {
    if (this != &other) {
        close();
        UniqueLock lockThis(dbMutex_, std::defer_lock);
        UniqueLock lockOther(other.dbMutex_, std::defer_lock);
        std::lock(lockThis, lockOther);

        dbPath_ = std::move(other.dbPath_);
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

bool SqliteQuestionRepository::initialize() {
    LockGuard lock(dbMutex_);
    if (db_) {
        return true;
    }

    if (!openDatabase()) {
        return false;
    }

    if (!executePragmas()) {
        close();
        return false;
    }

    if (!createSchema()) {
        close();
        return false;
    }

    return true;
}

bool SqliteQuestionRepository::openDatabase() {
    try {
        std::filesystem::path p(dbPath_);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (...) {
        // Let sqlite3_open report error if path is invalid
    }

    int rc = sqlite3_open_v2(
        dbPath_.c_str(),
        &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (rc != SQLITE_OK || !db_) {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        return false;
    }

    return true;
}

bool SqliteQuestionRepository::executePragmas() {
    if (!db_) return false;

    // Enable WAL for performance and concurrent readers/writers
    char* errMsg = nullptr;
    const char* pragmaSql =
        "PRAGMA journal_mode = WAL;"
        "PRAGMA synchronous = NORMAL;"
        "PRAGMA foreign_keys = ON;";

    int rc = sqlite3_exec(db_, pragmaSql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) {
            sqlite3_free(errMsg);
        }
        return false;
    }

    sqlite3_busy_timeout(db_, 5000);
    return true;
}

bool SqliteQuestionRepository::createSchema() {
    if (!db_) return false;

    const char* ddl =
        "CREATE TABLE IF NOT EXISTS schema_version ("
        "  version INTEGER PRIMARY KEY,"
        "  applied_at INTEGER NOT NULL,"
        "  description TEXT NOT NULL"
        ");"
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
        "CREATE TABLE IF NOT EXISTS questions ("
        "  id TEXT PRIMARY KEY NOT NULL,"
        "  title TEXT NOT NULL,"
        "  description TEXT NOT NULL DEFAULT '',"
        "  topic TEXT NOT NULL,"
        "  difficulty TEXT NOT NULL,"
        "  company TEXT NOT NULL DEFAULT '',"
        "  platform TEXT NOT NULL DEFAULT 'Custom',"
        "  source_url TEXT NOT NULL DEFAULT '',"
        "  status TEXT NOT NULL DEFAULT 'Todo',"
        "  is_favorite INTEGER NOT NULL DEFAULT 0,"
        "  notes TEXT NOT NULL DEFAULT '',"
        "  created_at INTEGER NOT NULL DEFAULT 0,"
        "  updated_at INTEGER NOT NULL DEFAULT 0,"
        "  last_practiced_at INTEGER NOT NULL DEFAULT 0,"
        "  next_revision_at INTEGER NOT NULL DEFAULT 0,"
        "  revision_priority INTEGER NOT NULL DEFAULT 2,"
        "  tags TEXT NOT NULL DEFAULT '',"
        "  owner_id TEXT NOT NULL DEFAULT 'local_user' REFERENCES users(id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_questions_topic ON questions(topic);"
        "CREATE INDEX IF NOT EXISTS idx_questions_difficulty ON questions(difficulty);"
        "CREATE INDEX IF NOT EXISTS idx_questions_status ON questions(status);"
        "CREATE INDEX IF NOT EXISTS idx_questions_company ON questions(company);"
        "CREATE INDEX IF NOT EXISTS idx_questions_next_rev ON questions(next_revision_at);"
        "CREATE INDEX IF NOT EXISTS idx_questions_owner ON questions(owner_id);"
        "CREATE TABLE IF NOT EXISTS user_credentials ("
        "  user_id TEXT PRIMARY KEY NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
        "  password_hash TEXT NOT NULL,"
        "  created_at INTEGER NOT NULL DEFAULT 0,"
        "  updated_at INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE TABLE IF NOT EXISTS sessions ("
        "  id TEXT PRIMARY KEY NOT NULL,"
        "  user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
        "  token_hash TEXT NOT NULL UNIQUE,"
        "  created_at INTEGER NOT NULL,"
        "  expires_at INTEGER NOT NULL,"
        "  revoked_at INTEGER DEFAULT NULL,"
        "  last_seen_at INTEGER NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_sessions_token_hash ON sessions(token_hash);"
        "CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id);";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, ddl, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) {
            sqlite3_free(errMsg);
        }
        return false;
    }

    int currentVer = getSchemaVersion();
    if (currentVer == 0) {
        recordSchemaVersion(1, "Initial CodeVault SQLite schema with questions table and domain indexes");
        recordSchemaVersion(2, "Multi-user data foundation: users table, indexes, and local_user seed");
        recordSchemaVersion(3, "Authentication & session foundation: user_credentials and sessions tables");
    } else if (currentVer == 1) {
        sqlite3_exec(db_, "UPDATE questions SET owner_id = 'local_user' WHERE owner_id IS NULL OR owner_id = '';", nullptr, nullptr, nullptr);
        recordSchemaVersion(2, "Multi-user data foundation: users table, indexes, and local_user seed");
        recordSchemaVersion(3, "Authentication & session foundation: user_credentials and sessions tables");
    } else if (currentVer == 2) {
        recordSchemaVersion(3, "Authentication & session foundation: user_credentials and sessions tables");
    }

    return true;
}

bool SqliteQuestionRepository::recordSchemaVersion(int version, const std::string& description) {
    if (!db_) return false;

    const char* sql = "INSERT OR REPLACE INTO schema_version (version, applied_at, description) VALUES (?, ?, ?);";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    sqlite3_bind_int(stmt, 1, version);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);

    return sqlite3_step(stmt) == SQLITE_DONE;
}

int SqliteQuestionRepository::getSchemaVersion() const {
    LockGuard lock(dbMutex_);
    if (!db_) return 0;

    const char* sql = "SELECT MAX(version) FROM schema_version;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return sqlite3_column_int(stmt, 0);
    }
    return 0;
}

int64_t SqliteQuestionRepository::getDatabaseSizeBytes() const {
    try {
        if (std::filesystem::exists(dbPath_)) {
            return static_cast<int64_t>(std::filesystem::file_size(dbPath_));
        }
    } catch (...) {}
    return 0;
}

void SqliteQuestionRepository::close() {
    LockGuard lock(dbMutex_);
    if (db_) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
}

bool SqliteQuestionRepository::isOpen() const noexcept {
    LockGuard lock(dbMutex_);
    return db_ != nullptr;
}

bool SqliteQuestionRepository::beginTransaction() {
    LockGuard lock(dbMutex_);
    return beginTransactionInternal();
}

bool SqliteQuestionRepository::commitTransaction() {
    LockGuard lock(dbMutex_);
    return commitTransactionInternal();
}

bool SqliteQuestionRepository::rollbackTransaction() {
    LockGuard lock(dbMutex_);
    return rollbackTransactionInternal();
}

bool SqliteQuestionRepository::beginTransactionInternal() {
    if (!db_) return false;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    return rc == SQLITE_OK;
}

bool SqliteQuestionRepository::commitTransactionInternal() {
    if (!db_) return false;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    return rc == SQLITE_OK;
}

bool SqliteQuestionRepository::rollbackTransactionInternal() {
    if (!db_) return false;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    return rc == SQLITE_OK;
}

bool SqliteQuestionRepository::executeTransaction(const std::function<bool()>& action) {
    LockGuard lock(dbMutex_);
    if (!beginTransactionInternal()) {
        return false;
    }

    bool success = false;
    try {
        success = action();
    } catch (...) {
        rollbackTransactionInternal();
        throw;
    }

    if (success) {
        return commitTransactionInternal();
    } else {
        rollbackTransactionInternal();
        return false;
    }
}

models::Question SqliteQuestionRepository::rowToQuestion(sqlite3_stmt* stmt) const {
    std::string id = getColumnText(stmt, 0);
    std::string title = getColumnText(stmt, 1);
    std::string description = getColumnText(stmt, 2);
    auto topic = models::stringToTopic(getColumnText(stmt, 3));
    auto difficulty = models::stringToDifficulty(getColumnText(stmt, 4));
    std::string company = getColumnText(stmt, 5);
    auto platform = models::stringToPlatform(getColumnText(stmt, 6));
    std::string sourceUrl = getColumnText(stmt, 7);
    auto status = models::stringToStatus(getColumnText(stmt, 8));
    bool isFavorite = sqlite3_column_int(stmt, 9) != 0;
    std::string notes = getColumnText(stmt, 10);
    int64_t createdAt = sqlite3_column_int64(stmt, 11);
    int64_t updatedAt = sqlite3_column_int64(stmt, 12);
    int64_t lastPracticedAt = sqlite3_column_int64(stmt, 13);
    int64_t nextRevisionAt = sqlite3_column_int64(stmt, 14);
    int32_t revisionPriority = sqlite3_column_int(stmt, 15);
    std::vector<std::string> tags = deserializeTags(getColumnText(stmt, 16));
    std::string ownerId = getColumnText(stmt, 17);

    return models::Question(
        id, title, description, topic, difficulty, company,
        platform, sourceUrl, status, isFavorite, notes,
        createdAt, updatedAt, lastPracticedAt, nextRevisionAt,
        revisionPriority, tags, ownerId
    );
}

std::vector<models::Question> SqliteQuestionRepository::findAll() const {
    LockGuard lock(dbMutex_);
    std::vector<models::Question> questions;
    if (!db_) return questions;

    const char* sql =
        "SELECT id, title, description, topic, difficulty, company, platform,"
        "       source_url, status, is_favorite, notes, created_at, updated_at,"
        "       last_practiced_at, next_revision_at, revision_priority, tags, owner_id "
        "FROM questions ORDER BY rowid ASC;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return questions;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        questions.push_back(rowToQuestion(stmt));
    }

    return questions;
}

std::optional<models::Question> SqliteQuestionRepository::findById(const std::string& id) const {
    LockGuard lock(dbMutex_);
    if (!db_ || id.empty()) return std::nullopt;

    const char* sql =
        "SELECT id, title, description, topic, difficulty, company, platform,"
        "       source_url, status, is_favorite, notes, created_at, updated_at,"
        "       last_practiced_at, next_revision_at, revision_priority, tags, owner_id "
        "FROM questions WHERE id = ?;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return rowToQuestion(stmt);
    }

    return std::nullopt;
}

bool SqliteQuestionRepository::exists(const std::string& id) const {
    LockGuard lock(dbMutex_);
    if (!db_ || id.empty()) return false;

    const char* sql = "SELECT 1 FROM questions WHERE id = ? LIMIT 1;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    return sqlite3_step(stmt) == SQLITE_ROW;
}

bool SqliteQuestionRepository::save(const models::Question& question) {
    if (!question.isValid()) {
        return false;
    }

    LockGuard lock(dbMutex_);
    if (!db_) return false;

    std::string owner = question.getOwnerId().empty() ? "local_user" : question.getOwnerId();
    // Ensure owner exists in users table so foreign key constraint is satisfied
    const char* userSql = "INSERT OR IGNORE INTO users (id, username, display_name, active) VALUES (?, ?, ?, 1);";
    ScopedStmt uStmt;
    if (sqlite3_prepare_v2(db_, userSql, -1, &uStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(uStmt, 1, owner.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(uStmt, 2, owner.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(uStmt, 3, owner.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(uStmt);
    }

    const char* sql =
        "INSERT INTO questions ("
        "  id, title, description, topic, difficulty, company, platform,"
        "  source_url, status, is_favorite, notes, created_at, updated_at,"
        "  last_practiced_at, next_revision_at, revision_priority, tags, owner_id"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        "ON CONFLICT(id) DO UPDATE SET "
        "  title = excluded.title,"
        "  description = excluded.description,"
        "  topic = excluded.topic,"
        "  difficulty = excluded.difficulty,"
        "  company = excluded.company,"
        "  platform = excluded.platform,"
        "  source_url = excluded.source_url,"
        "  status = excluded.status,"
        "  is_favorite = excluded.is_favorite,"
        "  notes = excluded.notes,"
        "  created_at = excluded.created_at,"
        "  updated_at = excluded.updated_at,"
        "  last_practiced_at = excluded.last_practiced_at,"
        "  next_revision_at = excluded.next_revision_at,"
        "  revision_priority = excluded.revision_priority,"
        "  tags = excluded.tags,"
        "  owner_id = excluded.owner_id;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    std::string topicStr = models::topicToString(question.getTopic());
    std::string diffStr = models::difficultyToString(question.getDifficulty());
    std::string platStr = models::platformToString(question.getPlatform());
    std::string statStr = models::statusToString(question.getStatus());
    std::string tagsStr = serializeTags(question.getTags());

    sqlite3_bind_text(stmt, 1, question.getId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, question.getTitle().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, question.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, topicStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, diffStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, question.getCompany().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, platStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, question.getSourceUrl().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, statStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, question.isFavorite() ? 1 : 0);
    sqlite3_bind_text(stmt, 11, question.getNotes().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 12, question.getCreatedAt());
    sqlite3_bind_int64(stmt, 13, question.getUpdatedAt());
    sqlite3_bind_int64(stmt, 14, question.getLastPracticedAt());
    sqlite3_bind_int64(stmt, 15, question.getNextRevisionAt());
    sqlite3_bind_int(stmt, 16, question.getRevisionPriority());
    sqlite3_bind_text(stmt, 17, tagsStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 18, question.getOwnerId().c_str(), -1, SQLITE_TRANSIENT);

    return sqlite3_step(stmt) == SQLITE_DONE;
}

bool SqliteQuestionRepository::remove(const std::string& id) {
    LockGuard lock(dbMutex_);
    if (!db_ || id.empty()) return false;

    const char* sql = "DELETE FROM questions WHERE id = ?;";
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

size_t SqliteQuestionRepository::count() const {
    LockGuard lock(dbMutex_);
    if (!db_) return 0;

    const char* sql = "SELECT COUNT(*) FROM questions;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return static_cast<size_t>(sqlite3_column_int64(stmt, 0));
    }

    return 0;
}

std::vector<models::Question> SqliteQuestionRepository::findAllByOwner(const std::string& ownerId) const {
    LockGuard lock(dbMutex_);
    std::vector<models::Question> questions;
    if (!db_ || ownerId.empty()) return questions;

    const char* sql =
        "SELECT id, title, description, topic, difficulty, company, platform,"
        "       source_url, status, is_favorite, notes, created_at, updated_at,"
        "       last_practiced_at, next_revision_at, revision_priority, tags, owner_id "
        "FROM questions WHERE owner_id = ? ORDER BY rowid ASC;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return questions;
    }

    sqlite3_bind_text(stmt, 1, ownerId.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        questions.push_back(rowToQuestion(stmt));
    }

    return questions;
}

std::optional<models::Question> SqliteQuestionRepository::findByIdForOwner(const std::string& id, const std::string& ownerId) const {
    LockGuard lock(dbMutex_);
    if (!db_ || id.empty() || ownerId.empty()) return std::nullopt;

    const char* sql =
        "SELECT id, title, description, topic, difficulty, company, platform,"
        "       source_url, status, is_favorite, notes, created_at, updated_at,"
        "       last_practiced_at, next_revision_at, revision_priority, tags, owner_id "
        "FROM questions WHERE id = ? AND owner_id = ?;";

    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return rowToQuestion(stmt);
    }

    return std::nullopt;
}

bool SqliteQuestionRepository::existsForOwner(const std::string& id, const std::string& ownerId) const {
    LockGuard lock(dbMutex_);
    if (!db_ || id.empty() || ownerId.empty()) return false;

    const char* sql = "SELECT 1 FROM questions WHERE id = ? AND owner_id = ? LIMIT 1;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerId.c_str(), -1, SQLITE_TRANSIENT);

    return sqlite3_step(stmt) == SQLITE_ROW;
}

bool SqliteQuestionRepository::saveForOwner(const models::Question& question, const std::string& ownerId) {
    if (!question.isValid() || ownerId.empty()) {
        return false;
    }

    LockGuard lock(dbMutex_);
    if (!db_) return false;

    // Check if question exists: if it exists, ensure it is owned by ownerId
    if (exists(question.getId()) && !existsForOwner(question.getId(), ownerId)) {
        return false; // Cross-user overwrite blocked!
    }

    models::Question scopedQ = question;
    scopedQ.setOwnerId(ownerId);
    return save(scopedQ);
}

bool SqliteQuestionRepository::removeForOwner(const std::string& id, const std::string& ownerId) {
    LockGuard lock(dbMutex_);
    if (!db_ || id.empty() || ownerId.empty()) return false;

    const char* sql = "DELETE FROM questions WHERE id = ? AND owner_id = ?;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, ownerId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        return false;
    }

    return sqlite3_changes(db_) > 0;
}

size_t SqliteQuestionRepository::countForOwner(const std::string& ownerId) const {
    LockGuard lock(dbMutex_);
    if (!db_ || ownerId.empty()) return 0;

    const char* sql = "SELECT COUNT(*) FROM questions WHERE owner_id = ?;";
    ScopedStmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, ownerId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return static_cast<size_t>(sqlite3_column_int64(stmt, 0));
    }

    return 0;
}

} // namespace codevault::persistence
