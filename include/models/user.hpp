#pragma once

#include <cstdint>
#include <string>

namespace codevault::models {

/**
 * @brief Domain entity representing a user in CodeVault.
 *
 * Designed for multi-user data scoping and ownership isolation.
 * Contains core profile metadata without coupling to specific auth mechanisms.
 */
class User {
public:
    User() = default;

    User(std::string id,
         std::string username,
         std::string displayName,
         std::string email = "",
         int64_t createdAt = 0,
         int64_t updatedAt = 0,
         bool active = true)
        : id_(std::move(id)),
          username_(std::move(username)),
          displayName_(std::move(displayName)),
          email_(std::move(email)),
          createdAt_(createdAt),
          updatedAt_(updatedAt),
          active_(active) {}

    // Getters
    const std::string& getId() const noexcept { return id_; }
    const std::string& getUsername() const noexcept { return username_; }
    const std::string& getDisplayName() const noexcept { return displayName_; }
    const std::string& getEmail() const noexcept { return email_; }
    int64_t getCreatedAt() const noexcept { return createdAt_; }
    int64_t getUpdatedAt() const noexcept { return updatedAt_; }
    bool isActive() const noexcept { return active_; }

    // Setters
    void setId(std::string id) { id_ = std::move(id); }
    void setUsername(std::string username) { username_ = std::move(username); }
    void setDisplayName(std::string displayName) { displayName_ = std::move(displayName); }
    void setEmail(std::string email) { email_ = std::move(email); }
    void setCreatedAt(int64_t createdAt) { createdAt_ = createdAt; }
    void setUpdatedAt(int64_t updatedAt) { updatedAt_ = updatedAt; }
    void setActive(bool active) { active_ = active; }

    // Validation
    bool isValid() const noexcept {
        return !id_.empty() && !username_.empty() && !displayName_.empty();
    }

private:
    std::string id_;
    std::string username_;
    std::string displayName_;
    std::string email_;
    int64_t createdAt_{0};
    int64_t updatedAt_{0};
    bool active_{true};
};

} // namespace codevault::models
