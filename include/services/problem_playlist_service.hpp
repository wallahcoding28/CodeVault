#pragma once

#include "dsa/doubly_linked_list.hpp"

#include <optional>
#include <string>
#include <vector>

namespace codevault::services {

/**
 * @brief Manages an ordered study session playlist with bidirectional navigation
 *        powered by a custom DoublyLinkedList.
 */
class ProblemPlaylistService {
public:
    ProblemPlaylistService() = default;

    /**
     * @brief Initialize playlist with a list of question IDs.
     */
    void loadPlaylist(const std::vector<std::string>& questionIds);

    /**
     * @brief Append a problem to the end of the playlist.
     */
    void addProblem(const std::string& questionId);

    /**
     * @brief Insert a problem directly after the currently selected problem.
     */
    void insertAfterCurrent(const std::string& questionId);

    /**
     * @brief Get the ID of the current active problem.
     */
    std::optional<std::string> getCurrentProblem() const;

    /**
     * @brief Advance to the next problem in the playlist.
     * @return Next problem ID if advanced, or nullopt if at the end.
     */
    std::optional<std::string> nextProblem();

    /**
     * @brief Step back to the previous problem in the playlist.
     * @return Previous problem ID if stepped back, or nullopt if at the beginning.
     */
    std::optional<std::string> previousProblem();

    /**
     * @brief Check whether there is a next problem after current.
     */
    bool hasNext() const noexcept;

    /**
     * @brief Check whether there is a previous problem before current.
     */
    bool hasPrevious() const noexcept;

    /**
     * @brief Remove the currently active problem from the playlist.
     * Repositions cursor to the next problem, or previous if at tail.
     * @return The removed problem ID, or nullopt if playlist was empty.
     */
    std::optional<std::string> removeCurrentProblem();

    /**
     * @brief Retrieve all problems in playlist order.
     */
    std::vector<std::string> getAllProblems() const;

    /**
     * @brief Retrieve all problems in reverse playlist order.
     */
    std::vector<std::string> getAllProblemsReversed() const;

    /**
     * @brief Number of problems in playlist.
     */
    size_t size() const noexcept;

    /**
     * @brief Check if playlist is empty.
     */
    bool empty() const noexcept;

    /**
     * @brief Clear playlist.
     */
    void clear() noexcept;

private:
    dsa::DoublyLinkedList<std::string> list_;
    dsa::DoublyLinkedList<std::string>::Node* current_{nullptr};
};

} // namespace codevault::services
