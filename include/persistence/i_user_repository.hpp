#pragma once

#include "models/user.hpp"
#include "models/user_credentials.hpp"

#include <optional>
#include <string>
#include <vector>

namespace codevault::persistence {

/**
 * @brief Abstract interface defining user persistence operations.
 *
 * Decouples user entity storage from concrete database engines.
 */
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual bool createUser(const models::User& user) = 0;
    virtual std::optional<models::User> findById(const std::string& id) const = 0;
    virtual std::optional<models::User> findByUsername(const std::string& username) const = 0;
    virtual std::optional<models::User> findByEmail(const std::string& email) const = 0;
    virtual bool updateUser(const models::User& user) = 0;
    virtual bool exists(const std::string& id) const = 0;
    virtual std::vector<models::User> findAll() const = 0;
    virtual size_t count() const = 0;

    // --- Credential Operations ---
    virtual bool saveCredentials(const models::UserCredentials& credentials) = 0;
    virtual std::optional<models::UserCredentials> getCredentials(const std::string& userId) const = 0;
    virtual bool deleteCredentials(const std::string& userId) = 0;
};

} // namespace codevault::persistence
