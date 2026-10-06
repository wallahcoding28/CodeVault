#pragma once

#include "dsa/stack.hpp"

#include <optional>
#include <string>
#include <vector>

namespace codevault::services {

/**
 * @brief Manages recent question navigation history using a custom LIFO Stack.
 *
 * Behavior:
 * - Consecutive duplicate views are ignored (viewing Q-1001 followed immediately by Q-1001 does not push twice).
 * - goBack() removes the top (most recent) item and returns the previous item if available.
 * - getHistory() produces an ordered vector of visited IDs from most recent to oldest.
 */
class RecentHistoryService {
public:
    RecentHistoryService() = default;

    /**
     * @brief Record a viewed question ID onto the history stack.
     * Consecutive duplicate views of the same ID are suppressed.
     */
    void recordView(const std::string& questionId);

    /**
     * @brief Retrieve the most recently viewed question ID.
     */
    std::optional<std::string> getLatest() const;

    /**
     * @brief Pop the current item from history and return the previous item.
     * @return The previous question ID if available, or nullopt if history becomes empty.
     */
    std::optional<std::string> goBack();

    /**
     * @brief Pop the current item without returning the next.
     * @return true if an item was popped, false if already empty.
     */
    bool pop();

    /**
     * @brief Number of entries in recent history.
     */
    size_t size() const noexcept;

    /**
     * @brief Check whether history is empty.
     */
    bool empty() const noexcept;

    /**
     * @brief Clear all history records.
     */
    void clear() noexcept;

    /**
     * @brief Dump history from most recent to oldest.
     */
    std::vector<std::string> getHistory() const;

private:
    dsa::Stack<std::string> stack_;
};

} // namespace codevault::services
