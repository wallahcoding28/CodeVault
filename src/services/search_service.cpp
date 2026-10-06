#include "services/search_service.hpp"
#include "dsa/sorting.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace codevault::services {

namespace {

int difficultyWeight(models::Difficulty diff) {
    switch (diff) {
        case models::Difficulty::Easy:   return 1;
        case models::Difficulty::Medium: return 2;
        case models::Difficulty::Hard:   return 3;
        default:                         return 4;
    }
}

int statusWeight(models::Status status) {
    switch (status) {
        case models::Status::Unsolved:   return 1;
        case models::Status::InProgress: return 2;
        case models::Status::Solved:     return 3;
        case models::Status::Mastered:   return 4;
        default:                         return 5;
    }
}

auto makeComparator(const models::SortOptions& opts) {
    return [opts](const models::Question& a, const models::Question& b) -> bool {
        bool aLessThanB = false;
        bool bLessThanA = false;

        switch (opts.field) {
            case models::SortField::Title: {
                if (a.getTitle() != b.getTitle()) {
                    aLessThanB = a.getTitle() < b.getTitle();
                    bLessThanA = b.getTitle() < a.getTitle();
                }
                break;
            }
            case models::SortField::Difficulty: {
                int wa = difficultyWeight(a.getDifficulty());
                int wb = difficultyWeight(b.getDifficulty());
                if (wa != wb) {
                    aLessThanB = wa < wb;
                    bLessThanA = wb < wa;
                }
                break;
            }
            case models::SortField::Topic: {
                std::string ta = topicToString(a.getTopic());
                std::string tb = topicToString(b.getTopic());
                if (ta != tb) {
                    aLessThanB = ta < tb;
                    bLessThanA = tb < ta;
                }
                break;
            }
            case models::SortField::Status: {
                int sa = statusWeight(a.getStatus());
                int sb = statusWeight(b.getStatus());
                if (sa != sb) {
                    aLessThanB = sa < sb;
                    bLessThanA = sb < sa;
                }
                break;
            }
            case models::SortField::Company: {
                if (a.getCompany() != b.getCompany()) {
                    aLessThanB = a.getCompany() < b.getCompany();
                    bLessThanA = b.getCompany() < a.getCompany();
                }
                break;
            }
            case models::SortField::CreatedAt: {
                if (a.getCreatedAt() != b.getCreatedAt()) {
                    aLessThanB = a.getCreatedAt() < b.getCreatedAt();
                    bLessThanA = b.getCreatedAt() < a.getCreatedAt();
                }
                break;
            }
            case models::SortField::UpdatedAt: {
                if (a.getUpdatedAt() != b.getUpdatedAt()) {
                    aLessThanB = a.getUpdatedAt() < b.getUpdatedAt();
                    bLessThanA = b.getUpdatedAt() < a.getUpdatedAt();
                }
                break;
            }
            case models::SortField::RevisionPriority: {
                if (a.getRevisionPriority() != b.getRevisionPriority()) {
                    aLessThanB = a.getRevisionPriority() < b.getRevisionPriority();
                    bLessThanA = b.getRevisionPriority() < a.getRevisionPriority();
                }
                break;
            }
        }

        if (opts.direction == models::SortDirection::Ascending) {
            if (aLessThanB) return true;
            if (bLessThanA) return false;
            return a.getId() < b.getId();
        } else {
            if (bLessThanA) return true;
            if (aLessThanB) return false;
            return a.getId() < b.getId();
        }
    };
}

} // namespace

SearchService::SearchService() = default;

std::string SearchService::normalize(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char ch : str) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return result;
}

void SearchService::indexQuestion(const models::Question& question) {
    const std::string& title = question.getTitle();
    const std::string& id = question.getId();
    if (title.empty()) return;

    titleTrie_.insert(title);

    std::string key = normalize(title);
    auto& ids = titleToIds_[key];
    if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
        ids.push_back(id);
    }
}

void SearchService::removeQuestion(const models::Question& question) {
    const std::string& title = question.getTitle();
    const std::string& id = question.getId();
    if (title.empty()) return;

    std::string key = normalize(title);
    auto it = titleToIds_.find(key);
    if (it != titleToIds_.end()) {
        auto& ids = it->second;
        ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
        if (ids.empty()) {
            titleToIds_.erase(it);
            titleTrie_.remove(title);
        }
    }
}

void SearchService::updateQuestion(const std::string& oldTitle, const models::Question& updatedQuestion) {
    if (normalize(oldTitle) != normalize(updatedQuestion.getTitle())) {
        models::Question oldQ;
        oldQ.setId(updatedQuestion.getId());
        oldQ.setTitle(oldTitle);
        removeQuestion(oldQ);
    }
    indexQuestion(updatedQuestion);
}

void SearchService::rebuildIndex(const std::vector<models::Question>& questions) {
    clear();
    for (const auto& q : questions) {
        indexQuestion(q);
    }
}

std::vector<std::string> SearchService::searchTitlesByPrefix(const std::string& prefix) const {
    return titleTrie_.autocomplete(prefix);
}

std::vector<std::string> SearchService::searchQuestionIdsByPrefix(const std::string& prefix) const {
    auto matchedTitles = titleTrie_.autocomplete(prefix);
    std::vector<std::string> ids;
    for (const auto& title : matchedTitles) {
        std::string key = normalize(title);
        auto it = titleToIds_.find(key);
        if (it != titleToIds_.end()) {
            for (const auto& id : it->second) {
                if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
                    ids.push_back(id);
                }
            }
        }
    }
    return ids;
}

std::vector<models::Question> SearchService::searchByTitlePrefix(const std::vector<models::Question>& questions,
                                                               const std::string& prefix) const {
    std::string trimmed = models::detail::trimString(prefix);
    if (trimmed.empty()) {
        return questions;
    }

    auto matchedIds = searchQuestionIdsByPrefix(trimmed);
    std::unordered_set<std::string> idSet(matchedIds.begin(), matchedIds.end());

    std::vector<models::Question> results;
    results.reserve(questions.size());

    for (const auto& q : questions) {
        if (!idSet.empty()) {
            if (idSet.count(q.getId()) > 0) {
                results.push_back(q);
            }
        } else {
            // Fallback for unindexed collections: perform direct case-insensitive prefix check
            if (models::detail::startsWithIgnoreCase(q.getTitle(), trimmed)) {
                results.push_back(q);
            }
        }
    }
    return results;
}

std::vector<models::Question> SearchService::searchByKeyword(const std::vector<models::Question>& questions,
                                                            const std::string& keyword) const {
    models::QuestionFilter criteria;
    criteria.keyword = keyword;
    return filter(questions, criteria);
}

std::vector<models::Question> SearchService::filterByTopic(const std::vector<models::Question>& questions,
                                                          models::Topic topic) const {
    models::QuestionFilter criteria;
    criteria.topic = topic;
    return filter(questions, criteria);
}

std::vector<models::Question> SearchService::filterByDifficulty(const std::vector<models::Question>& questions,
                                                               models::Difficulty difficulty) const {
    models::QuestionFilter criteria;
    criteria.difficulty = difficulty;
    return filter(questions, criteria);
}

std::vector<models::Question> SearchService::filterByStatus(const std::vector<models::Question>& questions,
                                                           models::Status status) const {
    models::QuestionFilter criteria;
    criteria.status = status;
    return filter(questions, criteria);
}

std::vector<models::Question> SearchService::filterByCompany(const std::vector<models::Question>& questions,
                                                            const std::string& company) const {
    models::QuestionFilter criteria;
    criteria.company = company;
    return filter(questions, criteria);
}

std::vector<models::Question> SearchService::filterFavorites(const std::vector<models::Question>& questions,
                                                            bool favoriteOnly) const {
    models::QuestionFilter criteria;
    criteria.isFavorite = favoriteOnly;
    return filter(questions, criteria);
}

std::vector<models::Question> SearchService::filter(const std::vector<models::Question>& questions,
                                                   const models::QuestionFilter& filterCriteria) const {
    std::vector<models::Question> results;
    results.reserve(questions.size());
    for (const auto& q : questions) {
        if (filterCriteria.matches(q)) {
            results.push_back(q);
        }
    }
    return results;
}

void SearchService::sort(std::vector<models::Question>& questions,
                         const models::SortOptions& options) const {
    if (questions.size() <= 1) return;

    auto comp = makeComparator(options);
    if (options.algorithm == models::SortAlgorithm::QuickSort) {
        dsa::quickSort(questions.begin(), questions.end(), comp);
    } else {
        dsa::mergeSort(questions.begin(), questions.end(), comp);
    }
}

std::vector<models::Question> SearchService::searchAndFilter(const std::vector<models::Question>& questions,
                                                            const models::QuestionFilter& filterCriteria,
                                                            const models::SortOptions& sortOptions) const {
    auto results = filter(questions, filterCriteria);
    sort(results, sortOptions);
    return results;
}

size_t SearchService::getIndexedTitleCount() const noexcept {
    return titleTrie_.size();
}

bool SearchService::containsTitle(const std::string& title) const {
    return titleTrie_.contains(title);
}

void SearchService::clear() {
    titleTrie_.clear();
    titleToIds_.clear();
}

} // namespace codevault::services
