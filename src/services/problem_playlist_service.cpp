#include "services/problem_playlist_service.hpp"

namespace codevault::services {

void ProblemPlaylistService::loadPlaylist(const std::vector<std::string>& questionIds) {
    clear();
    for (const auto& id : questionIds) {
        addProblem(id);
    }
}

void ProblemPlaylistService::addProblem(const std::string& questionId) {
    if (questionId.empty()) return;
    list_.pushBack(questionId);
    if (!current_) {
        // Point current_ to the head
        current_ = list_.begin().getNode();
    }
}

void ProblemPlaylistService::insertAfterCurrent(const std::string& questionId) {
    if (questionId.empty()) return;
    if (!current_) {
        addProblem(questionId);
        return;
    }

    // Insert directly after current_
    // Find index of current_
    size_t idx = 0;
    auto currNode = list_.begin().getNode();
    while (currNode && currNode != current_) {
        currNode = currNode->next;
        ++idx;
    }
    list_.insert(idx + 1, questionId);
}

std::optional<std::string> ProblemPlaylistService::getCurrentProblem() const {
    if (!current_) {
        return std::nullopt;
    }
    return current_->data;
}

std::optional<std::string> ProblemPlaylistService::nextProblem() {
    if (!current_ || !current_->next) {
        return std::nullopt;
    }
    current_ = current_->next;
    return current_->data;
}

std::optional<std::string> ProblemPlaylistService::previousProblem() {
    if (!current_ || !current_->prev) {
        return std::nullopt;
    }
    current_ = current_->prev;
    return current_->data;
}

bool ProblemPlaylistService::hasNext() const noexcept {
    return current_ && current_->next != nullptr;
}

bool ProblemPlaylistService::hasPrevious() const noexcept {
    return current_ && current_->prev != nullptr;
}

std::optional<std::string> ProblemPlaylistService::removeCurrentProblem() {
    if (!current_) {
        return std::nullopt;
    }

    std::string removedId = current_->data;
    auto nextNode = current_->next ? current_->next : current_->prev;

    // Remove current node by finding its index
    size_t idx = 0;
    auto node = list_.begin().getNode();
    while (node && node != current_) {
        node = node->next;
        ++idx;
    }

    list_.removeAt(idx);
    current_ = nextNode;

    return removedId;
}

std::vector<std::string> ProblemPlaylistService::getAllProblems() const {
    return list_.toVector();
}

std::vector<std::string> ProblemPlaylistService::getAllProblemsReversed() const {
    return list_.toReverseVector();
}

size_t ProblemPlaylistService::size() const noexcept {
    return list_.size();
}

bool ProblemPlaylistService::empty() const noexcept {
    return list_.empty();
}

void ProblemPlaylistService::clear() noexcept {
    list_.clear();
    current_ = nullptr;
}

} // namespace codevault::services
