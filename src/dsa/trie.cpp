#include "dsa/trie.hpp"

#include <algorithm>
#include <cctype>

namespace codevault::dsa {

PrefixTrie::PrefixTrie()
    : root_(std::make_unique<TrieNode>()), word_count_(0) {}

std::string PrefixTrie::normalize(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char ch : str) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return result;
}

void PrefixTrie::copyHelper(TrieNode* dest, const TrieNode* src) {
    if (!src) return;
    dest->is_terminal = src->is_terminal;
    dest->original_word = src->original_word;
    for (const auto& [ch, childNode] : src->children) {
        auto newChild = std::make_unique<TrieNode>();
        copyHelper(newChild.get(), childNode.get());
        dest->children[ch] = std::move(newChild);
    }
}

PrefixTrie::PrefixTrie(const PrefixTrie& other)
    : root_(std::make_unique<TrieNode>()), word_count_(other.word_count_) {
    copyHelper(root_.get(), other.root_.get());
}

PrefixTrie& PrefixTrie::operator=(const PrefixTrie& other) {
    if (this != &other) {
        auto newRoot = std::make_unique<TrieNode>();
        copyHelper(newRoot.get(), other.root_.get());
        root_ = std::move(newRoot);
        word_count_ = other.word_count_;
    }
    return *this;
}

bool PrefixTrie::insert(const std::string& word) {
    if (word.empty()) {
        return false;
    }

    std::string key = normalize(word);
    TrieNode* current = root_.get();

    for (char ch : key) {
        auto it = current->children.find(ch);
        if (it == current->children.end()) {
            auto newNode = std::make_unique<TrieNode>();
            TrieNode* rawPtr = newNode.get();
            current->children[ch] = std::move(newNode);
            current = rawPtr;
        } else {
            current = it->second.get();
        }
    }

    bool wasTerminal = current->is_terminal;
    current->is_terminal = true;
    current->original_word = word;

    if (!wasTerminal) {
        ++word_count_;
        return true;
    }
    return false;
}

bool PrefixTrie::contains(const std::string& word) const {
    if (word.empty()) {
        return false;
    }

    std::string key = normalize(word);
    const TrieNode* current = root_.get();

    for (char ch : key) {
        auto it = current->children.find(ch);
        if (it == current->children.end()) {
            return false;
        }
        current = it->second.get();
    }

    return current->is_terminal;
}

bool PrefixTrie::startsWith(const std::string& prefix) const {
    if (prefix.empty()) {
        return word_count_ > 0;
    }

    std::string key = normalize(prefix);
    const TrieNode* current = root_.get();

    for (char ch : key) {
        auto it = current->children.find(ch);
        if (it == current->children.end()) {
            return false;
        }
        current = it->second.get();
    }

    return true;
}

void PrefixTrie::collectWords(const TrieNode* node, std::vector<std::string>& results) {
    if (!node) return;

    if (node->is_terminal) {
        results.push_back(node->original_word);
    }

    for (const auto& [ch, child] : node->children) {
        collectWords(child.get(), results);
    }
}

std::vector<std::string> PrefixTrie::autocomplete(const std::string& prefix) const {
    std::vector<std::string> results;
    std::string key = normalize(prefix);
    const TrieNode* current = root_.get();

    for (char ch : key) {
        auto it = current->children.find(ch);
        if (it == current->children.end()) {
            return results; // Prefix not found
        }
        current = it->second.get();
    }

    collectWords(current, results);
    std::sort(results.begin(), results.end());
    return results;
}

bool PrefixTrie::removeHelper(TrieNode* current, const std::string& normalized, size_t depth, bool& removed) {
    if (!current) return false;

    if (depth == normalized.size()) {
        if (!current->is_terminal) {
            removed = false;
            return false;
        }

        current->is_terminal = false;
        current->original_word.clear();
        --word_count_;
        removed = true;

        // If this node has no children, it can be pruned by the parent
        return current->children.empty();
    }

    char ch = normalized[depth];
    auto it = current->children.find(ch);
    if (it == current->children.end()) {
        removed = false;
        return false;
    }

    bool shouldPruneChild = removeHelper(it->second.get(), normalized, depth + 1, removed);
    if (shouldPruneChild) {
        current->children.erase(it);
    }

    // Node can be pruned if it is not terminal and has no remaining children
    return !current->is_terminal && current->children.empty();
}

bool PrefixTrie::remove(const std::string& word) {
    if (word.empty()) {
        return false;
    }

    std::string key = normalize(word);
    bool removed = false;
    removeHelper(root_.get(), key, 0, removed);
    return removed;
}

void PrefixTrie::clear() {
    root_ = std::make_unique<TrieNode>();
    word_count_ = 0;
}

} // namespace codevault::dsa
