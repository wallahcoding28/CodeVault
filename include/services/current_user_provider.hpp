#pragma once

#include "models/user.hpp"

#include <memory>
#include <optional>
#include <string>

namespace codevault::services {

/**
 * @brief Abstract provider supplying the identity of the currently active user.
 *
 * Decouples services, repositories, and controllers from specific authentication mechanisms
 * (e.g. static local identity, session tokens, JWTs, HTTP headers).
 */
class ICurrentUserProvider {
public:
    virtual ~ICurrentUserProvider() = default;

    /**
     * @brief Get the unique identifier of the currently active user.
     */
    virtual std::string getCurrentUserId() const = 0;

    /**
     * @brief Optional: get the full User object if resolved.
     */
    virtual std::optional<models::User> getCurrentUser() const {
        return std::nullopt;
    }
};

/**
 * @brief Default static user provider returning a fixed user identity (e.g. "local_user").
 * Provides deterministic user context for local single-user mode and test fixtures.
 */
class StaticCurrentUserProvider : public ICurrentUserProvider {
public:
    explicit StaticCurrentUserProvider(std::string userId = "local_user")
        : userId_(std::move(userId)) {}

    std::string getCurrentUserId() const override {
        return userId_;
    }

    void setCurrentUserId(std::string userId) {
        userId_ = std::move(userId);
    }

private:
    std::string userId_;
};

/**
 * @brief Current user provider backed by an authenticated User domain object.
 */
class AuthenticatedCurrentUserProvider : public ICurrentUserProvider {
public:
    explicit AuthenticatedCurrentUserProvider(models::User user)
        : user_(std::move(user)) {}

    std::string getCurrentUserId() const override {
        return user_.getId();
    }

    std::optional<models::User> getCurrentUser() const override {
        return user_;
    }

private:
    models::User user_;
};

} // namespace codevault::services
