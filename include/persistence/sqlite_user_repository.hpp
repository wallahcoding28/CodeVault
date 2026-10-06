#pragma once

#include "persistence/i_user_repository.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

namespace codevault::persistence {

/**
 * @brief SQLite-backed implementation of IUserRepository.
 *
 * Provides transactional, ACID-compliant persistence for User domain models,
 * with index acceleration on username and email.
 */
class SqliteUserRepository : public IUserRepository {
public:
    explicit SqliteUserRepository(std::string dbPath);
    ~SqliteUserRepository() override;

    SqliteUserRepository(const SqliteUserRepository&) = delete;
    SqliteUserRepository& operator=(const SqliteUserRepository&) = delete;
    SqliteUserRepository(SqliteUserRepository&&) noexcept;
    SqliteUserRepository& operator=(SqliteUserRepository&&) noexcept;

    // --- IUserRepository Interface ---
    bool createUser(const models::User& user) override;
    std::optional<models::User> findById(const std::string& id) const override;
    std::optional<models::User> findByUsername(const std::string& username) const override;
    std::optional<models::User> findByEmail(const std::string& email) const override;
    bool updateUser(const models::User& user) override;
    bool exists(const std::string& id) const override;
    std::vector<models::User> findAll() const override;
    size_t count() const override;

    // --- User Management ---
    bool deactivateUser(const std::string& id);

    // --- Credential Operations ---
    bool saveCredentials(const models::UserCredentials& credentials) override;
    std::optional<models::UserCredentials> getCredentials(const std::string& userId) const override;
    bool deleteCredentials(const std::string& userId) override;

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

    models::User rowToUser(sqlite3_stmt* stmt) const;
};

} // namespace codevault::persistence
