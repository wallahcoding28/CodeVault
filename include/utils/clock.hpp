#pragma once

#include <chrono>
#include <cstdint>

namespace codevault::utils {

/**
 * @brief Interface for querying the current time in Unix timestamp seconds.
 *
 * Allows deterministic, testable time handling without dependency on system time.
 */
class IClock {
public:
    virtual ~IClock() = default;
    virtual int64_t now() const = 0;
};

/**
 * @brief Standard clock implementation using system_clock.
 */
class SystemClock : public IClock {
public:
    int64_t now() const override {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

/**
 * @brief Mock clock implementation for automated testing.
 */
class MockClock : public IClock {
public:
    explicit MockClock(int64_t initialTime = 0) : currentTime_(initialTime) {}

    int64_t now() const override {
        return currentTime_;
    }

    void setTime(int64_t newTime) {
        currentTime_ = newTime;
    }

    void advanceSeconds(int64_t seconds) {
        currentTime_ += seconds;
    }

    void advanceDays(int64_t days) {
        currentTime_ += days * 86400;
    }

private:
    int64_t currentTime_{0};
};

} // namespace codevault::utils
