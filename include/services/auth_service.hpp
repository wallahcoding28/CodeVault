#pragma once

#include "models/user.hpp"
#include "models/session.hpp"
#include "persistence/i_user_repository.hpp"
#include "persistence/i_session_repository.hpp"
#include <string>
#include <optional>
#include <memory>

namespace codevault::services {

struct AuthResult {
    bool success = false;
    std::string errorMessage;
    std::optional<models::User> user;
    std::string sessionToken; // Raw token sent to client (via Set-Cookie)
};

class AuthService {
public:
    AuthService(std::shared_ptr<persistence::IUserRepository> userRepo,
                std::shared_ptr<persistence::ISessionRepository> sessionRepo,
                int64_t sessionDurationSeconds = 604800); // 7 days

    AuthResult registerUser(const std::string& username,
                            const std::string& displayName,
                            const std::string& email,
                            const std::string& password);

    AuthResult login(const std::string& identifier, const std::string& password);

    std::optional<models::User> authenticateToken(const std::string& rawToken);

    bool logout(const std::string& rawToken);

    std::optional<models::User> getUserById(const std::string& userId);

    AuthResult createSessionForUser(const models::User& user);

    // --- Stage 9.4: Profile, Security & Session Management ---
    bool updateProfile(const std::string& userId,
                       const std::string& displayName,
                       const std::string& email,
                       std::string& outError);

    bool changePassword(const std::string& userId,
                        const std::string& currentPassword,
                        const std::string& newPassword,
                        std::string& outError);

    std::vector<models::Session> getActiveSessions(const std::string& userId);

    bool revokeSession(const std::string& userId, const std::string& sessionId);

    bool revokeOtherSessions(const std::string& userId, const std::string& currentRawToken);

private:
    std::shared_ptr<persistence::IUserRepository> userRepo_;
    std::shared_ptr<persistence::ISessionRepository> sessionRepo_;
    int64_t sessionDurationSeconds_;
};

} // namespace codevault::services
