#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace codevault::utils {

/**
 * @brief Format a Unix timestamp (seconds) into "YYYY-MM-DD HH:MM".
 */
inline std::string formatTimestamp(int64_t timestamp) {
    if (timestamp <= 0) {
        return "Never";
    }
    std::time_t tt = static_cast<std::time_t>(timestamp);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &tt);
#else
    localtime_r(&tt, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M");
    return oss.str();
}

/**
 * @brief Format a Unix timestamp (seconds) into "YYYY-MM-DD".
 */
inline std::string formatDateOnly(int64_t timestamp) {
    if (timestamp <= 0) {
        return "None";
    }
    std::time_t tt = static_cast<std::time_t>(timestamp);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &tt);
#else
    localtime_r(&tt, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d");
    return oss.str();
}

} // namespace codevault::utils
