#include "utils/crypto.hpp"

#include <argon2.h>
#include "../third_party/argon2/src/blake2/blake2.h"

#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#else
#include <fstream>
#endif

namespace codevault::utils::crypto {

namespace {

constexpr uint32_t ARGON2_TIME_COST = 2;          // 2 iterations
constexpr uint32_t ARGON2_MEMORY_COST = 19456;    // 19 MiB memory
constexpr uint32_t ARGON2_PARALLELISM = 1;        // 1 thread
constexpr size_t ARGON2_SALT_LENGTH = 16;         // 16 bytes salt
constexpr size_t ARGON2_HASH_LENGTH = 32;         // 32 bytes derived key

std::string bytesToHex(const uint8_t* data, size_t length) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

} // anonymous namespace

std::vector<uint8_t> generateRandomBytes(size_t count) {
    if (count == 0) {
        return {};
    }
    std::vector<uint8_t> buffer(count, 0);

#ifdef _WIN32
    HCRYPTPROV hCryptProv = 0;
    if (!CryptAcquireContext(&hCryptProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
        throw std::runtime_error("CSPRNG failure: CryptAcquireContext failed to acquire security context");
    }
    BOOL success = CryptGenRandom(hCryptProv, static_cast<DWORD>(count), buffer.data());
    CryptReleaseContext(hCryptProv, 0);
    if (!success) {
        throw std::runtime_error("CSPRNG failure: CryptGenRandom failed to generate cryptographically secure bytes");
    }
    return buffer;
#else
    std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);
    if (!urandom.is_open()) {
        throw std::runtime_error("CSPRNG failure: Unable to open /dev/urandom");
    }
    urandom.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(count));
    if (!urandom) {
        throw std::runtime_error("CSPRNG failure: Unable to read required entropy from /dev/urandom");
    }
    return buffer;
#endif
}

std::string generateSecureToken(size_t byteCount) {
    auto bytes = generateRandomBytes(byteCount);
    return bytesToHex(bytes.data(), bytes.size());
}

std::string hashPassword(const std::string& password) {
    if (password.empty()) {
        throw std::invalid_argument("Password cannot be empty for hashing");
    }

    auto salt = generateRandomBytes(ARGON2_SALT_LENGTH);
    size_t encodedLen = argon2_encodedlen(
        ARGON2_TIME_COST,
        ARGON2_MEMORY_COST,
        ARGON2_PARALLELISM,
        static_cast<uint32_t>(salt.size()),
        static_cast<uint32_t>(ARGON2_HASH_LENGTH),
        Argon2_id
    );

    std::vector<char> encoded(encodedLen + 32, '\0');

    int rc = argon2id_hash_encoded(
        ARGON2_TIME_COST,
        ARGON2_MEMORY_COST,
        ARGON2_PARALLELISM,
        password.data(),
        password.size(),
        salt.data(),
        salt.size(),
        ARGON2_HASH_LENGTH,
        encoded.data(),
        encoded.size()
    );

    if (rc != ARGON2_OK) {
        throw std::runtime_error("Argon2id password hashing failed with code: " + std::to_string(rc));
    }

    return std::string(encoded.data());
}

bool verifyPassword(const std::string& password, const std::string& encodedHash) {
    if (password.empty() || encodedHash.empty()) {
        return false;
    }

    int rc = argon2id_verify(encodedHash.c_str(), password.data(), password.size());
    return (rc == ARGON2_OK);
}

std::string hashToken(const std::string& rawToken) {
    if (rawToken.empty()) {
        return {};
    }

    uint8_t hashBuf[32];
    blake2b(hashBuf, sizeof(hashBuf), rawToken.data(), rawToken.size(), nullptr, 0);
    return bytesToHex(hashBuf, sizeof(hashBuf));
}

} // namespace codevault::utils::crypto
