#pragma once

#include <cstdint>
#include <string>

namespace codevault::models {

/**
 * @brief Server-side session entity.
 * Stores only a cryptographic hash of the session token.
 * Raw session tokens are never stored in the database.
 */
class Session {
public:
    Session() = default;

    Session(std::string id,
            std::string userId,
            std::string tokenHash,
            int64_t createdAt,
            int64_t expiresAt,
            int64_t revokedAt = 0,
            int64_t lastSeenAt = 0)
        : id_(std::move(id)),
          userId_(std::move(userId)),
          tokenHash_(std::move(tokenHash)),
          createdAt_(createdAt),
          expiresAt_(expiresAt),
          revokedAt_(revokedAt),
          lastSeenAt_(lastSeenAt) {}

    const std::string& getId() const noexcept { return id_; }
    const std::string& getUserId() const noexcept { return userId_; }
    const std::string& getTokenHash() const noexcept { return tokenHash_; }
    int64_t getCreatedAt() const noexcept { return createdAt_; }
    int64_t getExpiresAt() const noexcept { return expiresAt_; }
    int64_t getRevokedAt() const noexcept { return revokedAt_; }
    int64_t getLastSeenAt() const noexcept { return lastSeenAt_; }

    void setId(std::string id) { id_ = std::move(id); }
    void setUserId(std::string userId) { userId_ = std::move(userId); }
    void setTokenHash(std::string tokenHash) { tokenHash_ = std::move(tokenHash); }
    void setCreatedAt(int64_t createdAt) { createdAt_ = createdAt; }
    void setExpiresAt(int64_t expiresAt) { expiresAt_ = expiresAt; }
    void setRevokedAt(int64_t revokedAt) { revokedAt_ = revokedAt; }
    void setLastSeenAt(int64_t lastSeenAt) { lastSeenAt_ = lastSeenAt; }

    bool isValid(int64_t currentTime) const noexcept {
        return !id_.empty() && !userId_.empty() && !tokenHash_.empty() &&
               revokedAt_ == 0 && expiresAt_ > currentTime;
    }

private:
    std::string id_;
    std::string userId_;
    std::string tokenHash_;
    int64_t createdAt_{0};
    int64_t expiresAt_{0};
    int64_t revokedAt_{0};
    int64_t lastSeenAt_{0};
};

} // namespace codevault::models
