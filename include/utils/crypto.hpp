#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace codevault::utils::crypto {

/**
 * @brief Hashes a plaintext password using Argon2id with OWASP-recommended parameters.
 *
 * @param password The plaintext password to hash.
 * @return Formatted PHC encoded string ($argon2id$v=19$m=...,t=...,p=...$salt$hash)
 */
std::string hashPassword(const std::string& password);

/**
 * @brief Verifies a plaintext password against an Argon2id encoded hash string.
 *
 * @param password The plaintext password to verify.
 * @param encodedHash The stored PHC encoded Argon2id hash.
 * @return true if password matches, false otherwise.
 */
bool verifyPassword(const std::string& password, const std::string& encodedHash);

/**
 * @brief Generates cryptographically secure random bytes.
 *
 * @param count Number of random bytes.
 * @return Vector of random bytes.
 */
std::vector<uint8_t> generateRandomBytes(size_t count);

/**
 * @brief Generates a cryptographically secure random token formatted as a hex string.
 *
 * @param byteCount Number of random bytes of entropy (default 32 = 256 bits).
 * @return Hexadecimal string representation (e.g. 64 hex characters for 32 bytes).
 */
std::string generateSecureToken(size_t byteCount = 32);

/**
 * @brief Computes a cryptographic hash (Blake2b-256) of a session token for secure database storage.
 *
 * @param rawToken The raw session token presented by client.
 * @return 64-character lowercase hex digest.
 */
std::string hashToken(const std::string& rawToken);

} // namespace codevault::utils::crypto
