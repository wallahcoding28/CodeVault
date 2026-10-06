#include "services/recent_history_service.hpp"

namespace codevault::services {

void RecentHistoryService::recordView(const std::string& questionId) {
    if (questionId.empty()) return;

    // Suppress consecutive duplicate entry
    if (!stack_.empty() && stack_.top() == questionId) {
        return;
    }

    stack_.push(questionId);
}

std::optional<std::string> RecentHistoryService::getLatest() const {
    if (stack_.empty()) {
        return std::nullopt;
    }
    return stack_.top();
}

std::optional<std::string> RecentHistoryService::goBack() {
    if (stack_.empty()) {
        return std::nullopt;
    }
    stack_.pop();
    if (stack_.empty()) {
        return std::nullopt;
    }
    return stack_.top();
}

bool RecentHistoryService::pop() {
    if (stack_.empty()) {
        return false;
    }
    stack_.pop();
    return true;
}

size_t RecentHistoryService::size() const noexcept {
    return stack_.size();
}

bool RecentHistoryService::empty() const noexcept {
    return stack_.empty();
}

void RecentHistoryService::clear() noexcept {
    stack_.clear();
}

std::vector<std::string> RecentHistoryService::getHistory() const {
    // To preserve stack and dump items without destroying stack_:
    // Make a copy of the stack and pop into vector
    dsa::Stack<std::string> tempStack = stack_;
    std::vector<std::string> result;
    result.reserve(tempStack.size());

    while (!tempStack.empty()) {
        result.push_back(tempStack.top());
        tempStack.pop();
    }
    return result;
}

} // namespace codevault::services
