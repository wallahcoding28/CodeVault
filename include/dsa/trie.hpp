#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace codevault::dsa {

/**
 * @brief Node within the custom PrefixTrie.
 */
struct TrieNode {
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool is_terminal{false};
    std::string original_word; // Preserves original casing for retrieval
};

/**
 * @brief Custom Prefix Trie (Prefix Tree) for sub-millisecond string prefix lookups.
 * 
 * Time Complexities:
 * - Insert: O(L)
 * - Search / Contains: O(L)
 * - Prefix search (startsWith): O(L)
 * - Autocomplete / getWordsWithPrefix: O(L + K log K) for deterministic sorted output
 * - Remove: O(L)
 * where L is the length of the string, and K is the count of matched descendant words.
 *
 * Space Complexity:
 * - O(Sigma * L * N) where Sigma is alphabet branch factor, L average length, N words.
 */
class PrefixTrie {
public:
    PrefixTrie();
    ~PrefixTrie() = default;

    // Rule of 5: Prevent accidental shallow slicing; allow movement
    PrefixTrie(const PrefixTrie& other);
    PrefixTrie& operator=(const PrefixTrie& other);
    PrefixTrie(PrefixTrie&& other) noexcept = default;
    PrefixTrie& operator=(PrefixTrie&& other) noexcept = default;

    /**
     * @brief Insert a word into the trie.
     * @param word The word or phrase to insert.
     * @return true if inserted as a new word, false if empty or already existed.
     */
    bool insert(const std::string& word);

    /**
     * @brief Remove a word from the trie.
     * @param word The word to remove.
     * @return true if removed, false if not present or empty.
     */
    bool remove(const std::string& word);

    /**
     * @brief Check whether an exact word exists in the trie.
     * @param word The word to search.
     * @return true if present as a terminal word, false otherwise.
     */
    bool contains(const std::string& word) const;

    /**
     * @brief Check whether any word in the trie begins with the given prefix.
     * @param prefix The prefix string.
     * @return true if prefix exists, false otherwise.
     */
    bool startsWith(const std::string& prefix) const;

    /**
     * @brief Retrieve all words starting with the specified prefix (case-insensitive search).
     * @param prefix Prefix to autocomplete.
     * @return Deterministically sorted vector of matching original words.
     */
    std::vector<std::string> autocomplete(const std::string& prefix) const;

    /**
     * @brief Alias for autocomplete.
     */
    std::vector<std::string> getWordsWithPrefix(const std::string& prefix) const {
        return autocomplete(prefix);
    }

    /**
     * @brief Clear all nodes from the trie.
     */
    void clear();

    /**
     * @brief Get the count of unique words stored in the trie.
     */
    size_t size() const noexcept { return word_count_; }

    /**
     * @brief Check if the trie is empty.
     */
    bool empty() const noexcept { return word_count_ == 0; }

private:
    std::unique_ptr<TrieNode> root_;
    size_t word_count_{0};

    // Helper functions
    static std::string normalize(const std::string& str);
    static void collectWords(const TrieNode* node, std::vector<std::string>& results);
    bool removeHelper(TrieNode* current, const std::string& normalized, size_t depth, bool& removed);
    void copyHelper(TrieNode* dest, const TrieNode* src);
};

} // namespace codevault::dsa
