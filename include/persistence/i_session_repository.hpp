#pragma once

#include "models/session.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace codevault::persistence {

/**
 * @brief Abstract interface defining session persistence operations.
 */
class ISessionRepository {
public:
    virtual ~ISessionRepository() = default;

    virtual bool createSession(const models::Session& session) = 0;
    virtual std::optional<models::Session> findByTokenHash(const std::string& tokenHash) const = 0;
    virtual std::optional<models::Session> findById(const std::string& sessionId) const = 0;
    virtual bool revokeSession(const std::string& sessionId, int64_t revokedAt) = 0;
    virtual bool revokeAllForUser(const std::string& userId, int64_t revokedAt) = 0;
    virtual bool revokeAllForUserExcept(const std::string& userId, const std::string& exceptSessionId, int64_t revokedAt) = 0;
    virtual std::vector<models::Session> findActiveSessionsForUser(const std::string& userId, int64_t now) const = 0;
    virtual bool updateLastSeen(const std::string& sessionId, int64_t lastSeenAt) = 0;
    virtual size_t deleteExpiredSessions(int64_t now) = 0;
};

} // namespace codevault::persistence
