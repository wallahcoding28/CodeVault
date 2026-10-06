#pragma once
#include "dsa/trie.hpp"
#include "models/enums.hpp"
#include "models/question.hpp"
#include "models/question_filter.hpp"
#include "models/sort_options.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace codevault::services {

/**
 * @brief Search, filtering, and sorting service powered by PrefixTrie and custom sorting algorithms.
 *
 * Provides:
 * 1. O(L) prefix autocomplete and lookup via Trie
 * 2. O(N) multi-field keyword and criteria filtering
 * 3. Custom Merge Sort (stable, O(N log N)) and Quick Sort (in-place)
 * 4. Combined search + filter + sort workflows
 */
class SearchService {
public:
    SearchService();
    ~SearchService() = default;

    /**
     * @brief Index a question's title into the Trie.
     */
    void indexQuestion(const models::Question& question);

    /**
     * @brief Remove a question's title from the Trie index.
     */
    void removeQuestion(const models::Question& question);

    /**
     * @brief Update the index when a question's title changes.
     */
    void updateQuestion(const std::string& oldTitle, const models::Question& updatedQuestion);

    /**
     * @brief Rebuild index from an entire list of questions.
     */
    void rebuildIndex(const std::vector<models::Question>& questions);

    /**
     * @brief Search question titles matching a given prefix.
     * @param prefix Case-insensitive prefix string.
     * @return List of matching question titles from PrefixTrie.
     */
    std::vector<std::string> searchTitlesByPrefix(const std::string& prefix) const;

    /**
     * @brief Search question IDs whose titles match the given prefix.
     * @param prefix Case-insensitive prefix string.
     * @return List of question IDs.
     */
    std::vector<std::string> searchQuestionIdsByPrefix(const std::string& prefix) const;

    /**
     * @brief Retrieve questions whose titles match a prefix using the Trie index.
     * @param questions Master question collection to filter from.
     * @param prefix Prefix query string.
     * @return Matching questions.
     */
    std::vector<models::Question> searchByTitlePrefix(const std::vector<models::Question>& questions,
                                                     const std::string& prefix) const;

    /**
     * @brief Broad keyword scan across title, description, tags, company, and topic.
     * @param questions Master question collection.
     * @param keyword Search term.
     * @return Questions containing the keyword.
     */
    std::vector<models::Question> searchByKeyword(const std::vector<models::Question>& questions,
                                                  const std::string& keyword) const;

    /**
     * @brief Filter questions by topic.
     */
    std::vector<models::Question> filterByTopic(const std::vector<models::Question>& questions,
                                                models::Topic topic) const;

    /**
     * @brief Filter questions by difficulty.
     */
    std::vector<models::Question> filterByDifficulty(const std::vector<models::Question>& questions,
                                                     models::Difficulty difficulty) const;

    /**
     * @brief Filter questions by solving status.
     */
    std::vector<models::Question> filterByStatus(const std::vector<models::Question>& questions,
                                                 models::Status status) const;

    /**
     * @brief Filter questions by company (case-insensitive substring match).
     */
    std::vector<models::Question> filterByCompany(const std::vector<models::Question>& questions,
                                                  const std::string& company) const;

    /**
     * @brief Filter questions by favorite flag.
     */
    std::vector<models::Question> filterFavorites(const std::vector<models::Question>& questions,
                                                  bool favoriteOnly = true) const;

    /**
     * @brief Apply a multi-field QuestionFilter across a collection.
     */
    std::vector<models::Question> filter(const std::vector<models::Question>& questions,
                                         const models::QuestionFilter& filterCriteria) const;

    /**
     * @brief Sort questions in-place in the provided collection using custom algorithms.
     * @param questions Collection to sort (non-mutating to repository).
     * @param options Field, direction, and algorithm selection.
     */
    void sort(std::vector<models::Question>& questions,
              const models::SortOptions& options) const;

    /**
     * @brief Complete pipeline: filter and then sort questions.
     */
    std::vector<models::Question> searchAndFilter(const std::vector<models::Question>& questions,
                                                  const models::QuestionFilter& filterCriteria,
                                                  const models::SortOptions& sortOptions) const;

    /**
     * @brief Number of indexed titles.
     */
    size_t getIndexedTitleCount() const noexcept;

    /**
     * @brief Check whether a specific title is indexed.
     */
    bool containsTitle(const std::string& title) const;

    /**
     * @brief Clear all search indices.
     */
    void clear();

private:
    dsa::PrefixTrie titleTrie_;
    // Title -> list of Question IDs with this title
    std::unordered_map<std::string, std::vector<std::string>> titleToIds_;

    static std::string normalize(const std::string& str);
};

} // namespace codevault::services
