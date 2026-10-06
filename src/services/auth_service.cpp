#include "services/auth_service.hpp"
#include "utils/crypto.hpp"
#include <chrono>
#include <regex>

namespace codevault::services {

static int64_t getCurrentTimeSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

static bool isValidEmail(const std::string& email) {
    static const std::regex emailRegex(R"(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)");
    return std::regex_match(email, emailRegex);
}

AuthService::AuthService(std::shared_ptr<persistence::IUserRepository> userRepo,
                         std::shared_ptr<persistence::ISessionRepository> sessionRepo,
                         int64_t sessionDurationSeconds)
    : userRepo_(std::move(userRepo)),
      sessionRepo_(std::move(sessionRepo)),
      sessionDurationSeconds_(sessionDurationSeconds) {}

AuthResult AuthService::registerUser(const std::string& username,
                                     const std::string& displayName,
                                     const std::string& email,
                                     const std::string& password) {
    AuthResult result;

    // Validate username
    if (username.length() < 3 || username.length() > 50) {
        result.errorMessage = "Username must be between 3 and 50 characters";
        return result;
    }

    for (char c : username) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            result.errorMessage = "Username can only contain alphanumeric characters, hyphens, and underscores";
            return result;
        }
    }

    // Validate password
    if (password.length() < 8) {
        result.errorMessage = "Password must be at least 8 characters long";
        return result;
    }

    // Validate email if provided
    if (!email.empty()) {
        static const std::regex emailRegex(R"(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)");
        if (!std::regex_match(email, emailRegex)) {
            result.errorMessage = "Invalid email format";
            return result;
        }
    }

    // Check unique username
    if (userRepo_->findByUsername(username).has_value()) {
        result.errorMessage = "Username already taken";
        return result;
    }

    // Check unique email if provided
    if (!email.empty() && userRepo_->findByEmail(email).has_value()) {
        result.errorMessage = "Email already registered";
        return result;
    }

    // Hash password with Argon2id
    std::string passwordHash = utils::crypto::hashPassword(password);
    if (passwordHash.empty()) {
        result.errorMessage = "Failed to hash password securely";
        return result;
    }

    int64_t now = getCurrentTimeSeconds();
    std::string userId = "usr_" + utils::crypto::generateSecureToken(8);
    std::string finalDisplayName = displayName.empty() ? username : displayName;

    models::User user(userId, username, finalDisplayName, email, now, now, true);
    if (!userRepo_->createUser(user)) {
        result.errorMessage = "Failed to create user record";
        return result;
    }

    models::UserCredentials creds(userId, passwordHash, now, now);
    if (!userRepo_->saveCredentials(creds)) {
        user.setActive(false);
        userRepo_->updateUser(user);
        result.errorMessage = "Failed to save user credentials";
        return result;
    }

    return createSessionForUser(user);
}

AuthResult AuthService::login(const std::string& identifier, const std::string& password) {
    AuthResult result;

    if (identifier.empty() || password.empty()) {
        result.errorMessage = "Invalid username or password";
        return result;
    }

    // Lookup user by username or email
    std::optional<models::User> userOpt = userRepo_->findByUsername(identifier);
    if (!userOpt && identifier.find('@') != std::string::npos) {
        userOpt = userRepo_->findByEmail(identifier);
    }

    if (!userOpt) {
        // Generic failure prevents user enumeration
        result.errorMessage = "Invalid username or password";
        return result;
    }

    if (!userOpt->isActive()) {
        result.errorMessage = "Account is disabled";
        return result;
    }

    auto credsOpt = userRepo_->getCredentials(userOpt->getId());
    if (!credsOpt) {
        result.errorMessage = "Invalid username or password";
        return result;
    }

    if (!utils::crypto::verifyPassword(password, credsOpt->getPasswordHash())) {
        result.errorMessage = "Invalid username or password";
        return result;
    }

    return createSessionForUser(*userOpt);
}

std::optional<models::User> AuthService::authenticateToken(const std::string& rawToken) {
    if (rawToken.empty() || !sessionRepo_ || !userRepo_) {
        return std::nullopt;
    }

    std::string tokenHash = utils::crypto::hashToken(rawToken);
    auto sessionOpt = sessionRepo_->findByTokenHash(tokenHash);
    if (!sessionOpt) {
        return std::nullopt;
    }

    int64_t now = getCurrentTimeSeconds();
    if (!sessionOpt->isValid(now)) {
        return std::nullopt;
    }

    auto userOpt = userRepo_->findById(sessionOpt->getUserId());
    if (!userOpt || !userOpt->isActive()) {
        return std::nullopt;
    }

    // Update session last seen timestamp
    sessionRepo_->updateLastSeen(sessionOpt->getId(), now);

    return userOpt;
}

bool AuthService::logout(const std::string& rawToken) {
    if (rawToken.empty() || !sessionRepo_) {
        return true;
    }

    std::string tokenHash = utils::crypto::hashToken(rawToken);
    auto sessionOpt = sessionRepo_->findByTokenHash(tokenHash);
    if (sessionOpt) {
        int64_t now = getCurrentTimeSeconds();
        sessionRepo_->revokeSession(sessionOpt->getId(), now);
    }
    return true;
}

std::optional<models::User> AuthService::getUserById(const std::string& userId) {
    if (!userRepo_) return std::nullopt;
    return userRepo_->findById(userId);
}

AuthResult AuthService::createSessionForUser(const models::User& user) {
    AuthResult result;
    if (!sessionRepo_) {
        result.errorMessage = "Session repository unavailable";
        return result;
    }

    std::string rawToken = utils::crypto::generateSecureToken(32); // 64 hex chars
    std::string tokenHash = utils::crypto::hashToken(rawToken);
    std::string sessionId = "ses_" + utils::crypto::generateSecureToken(8);
    int64_t now = getCurrentTimeSeconds();
    int64_t expiresAt = now + sessionDurationSeconds_;

    models::Session session(sessionId, user.getId(), tokenHash, now, expiresAt, 0, now);
    if (!sessionRepo_->createSession(session)) {
        result.errorMessage = "Failed to establish session";
        return result;
    }

    result.success = true;
    result.user = user;
    result.sessionToken = rawToken;
    return result;
}

bool AuthService::updateProfile(const std::string& userId,
                                const std::string& displayName,
                                const std::string& email,
                                std::string& outError) {
    if (!userRepo_ || userId.empty()) {
        outError = "User repository unavailable";
        return false;
    }

    auto userOpt = userRepo_->findById(userId);
    if (!userOpt || !userOpt->isActive()) {
        outError = "User not found or inactive";
        return false;
    }

    models::User user = *userOpt;

    // Email validation & uniqueness if provided
    if (!email.empty()) {
        if (!isValidEmail(email)) {
            outError = "Invalid email format";
            return false;
        }

        if (email != user.getEmail()) {
            auto existingWithEmail = userRepo_->findByEmail(email);
            if (existingWithEmail && existingWithEmail->getId() != userId) {
                outError = "Email already registered to another account";
                return false;
            }
        }
        user.setEmail(email);
    }

    if (!displayName.empty()) {
        user.setDisplayName(displayName);
    }

    user.setUpdatedAt(getCurrentTimeSeconds());

    if (!userRepo_->updateUser(user)) {
        outError = "Failed to update user profile";
        return false;
    }

    return true;
}

bool AuthService::changePassword(const std::string& userId,
                                 const std::string& currentPassword,
                                 const std::string& newPassword,
                                 std::string& outError) {
    if (!userRepo_ || userId.empty()) {
        outError = "User repository unavailable";
        return false;
    }

    if (newPassword.length() < 8) {
        outError = "New password must be at least 8 characters long";
        return false;
    }

    auto userOpt = userRepo_->findById(userId);
    if (!userOpt || !userOpt->isActive()) {
        outError = "User not found or inactive";
        return false;
    }

    auto credsOpt = userRepo_->getCredentials(userId);
    if (!credsOpt) {
        outError = "Credentials record not found";
        return false;
    }

    if (!utils::crypto::verifyPassword(currentPassword, credsOpt->getPasswordHash())) {
        outError = "Current password is incorrect";
        return false;
    }

    std::string newHash = utils::crypto::hashPassword(newPassword);
    int64_t now = getCurrentTimeSeconds();
    models::UserCredentials newCreds(userId, newHash, credsOpt->getCreatedAt(), now);

    if (!userRepo_->saveCredentials(newCreds)) {
        outError = "Failed to save updated credentials";
        return false;
    }

    return true;
}

std::vector<models::Session> AuthService::getActiveSessions(const std::string& userId) {
    if (!sessionRepo_ || userId.empty()) {
        return {};
    }
    return sessionRepo_->findActiveSessionsForUser(userId, getCurrentTimeSeconds());
}

bool AuthService::revokeSession(const std::string& userId, const std::string& sessionId) {
    if (!sessionRepo_ || userId.empty() || sessionId.empty()) {
        return false;
    }

    auto sessionOpt = sessionRepo_->findById(sessionId);
    if (!sessionOpt || sessionOpt->getUserId() != userId) {
        return false; // Cannot revoke other user's session
    }

    return sessionRepo_->revokeSession(sessionId, getCurrentTimeSeconds());
}

bool AuthService::revokeOtherSessions(const std::string& userId, const std::string& currentRawToken) {
    if (!sessionRepo_ || userId.empty() || currentRawToken.empty()) {
        return false;
    }

    std::string tokenHash = utils::crypto::hashToken(currentRawToken);
    auto currentSession = sessionRepo_->findByTokenHash(tokenHash);
    if (!currentSession || currentSession->getUserId() != userId) {
        return false;
    }

    return sessionRepo_->revokeAllForUserExcept(userId, currentSession->getId(), getCurrentTimeSeconds());
}

} // namespace codevault::services
