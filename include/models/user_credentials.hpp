#pragma once

#include <cstdint>
#include <string>

namespace codevault::models {

/**
 * @brief Authentication credentials for a user.
 * Kept completely decoupled from the public User profile model.
 * Never exposed through API responses or serialized to clients.
 */
class UserCredentials {
public:
    UserCredentials() = default;

    UserCredentials(std::string userId,
                    std::string passwordHash,
                    int64_t createdAt = 0,
                    int64_t updatedAt = 0)
        : userId_(std::move(userId)),
          passwordHash_(std::move(passwordHash)),
          createdAt_(createdAt),
          updatedAt_(updatedAt) {}

    const std::string& getUserId() const noexcept { return userId_; }
    const std::string& getPasswordHash() const noexcept { return passwordHash_; }
    int64_t getCreatedAt() const noexcept { return createdAt_; }
    int64_t getUpdatedAt() const noexcept { return updatedAt_; }

    void setUserId(std::string userId) { userId_ = std::move(userId); }
    void setPasswordHash(std::string passwordHash) { passwordHash_ = std::move(passwordHash); }
    void setCreatedAt(int64_t createdAt) { createdAt_ = createdAt; }
    void setUpdatedAt(int64_t updatedAt) { updatedAt_ = updatedAt; }

    bool isValid() const noexcept {
        return !userId_.empty() && !passwordHash_.empty();
    }

private:
    std::string userId_;
    std::string passwordHash_;
    int64_t createdAt_{0};
    int64_t updatedAt_{0};
};

} // namespace codevault::models
