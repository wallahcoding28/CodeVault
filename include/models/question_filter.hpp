#pragma once

#include "models/enums.hpp"
#include "models/question.hpp"

#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace codevault::models {

namespace detail {

inline std::string toLowerString(std::string_view sv) {
    std::string s;
    s.reserve(sv.size());
    for (char c : sv) {
        s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return s;
}

inline std::string trimString(std::string_view sv) {
    size_t first = 0;
    while (first < sv.size() && std::isspace(static_cast<unsigned char>(sv[first]))) {
        ++first;
    }
    size_t last = sv.size();
    while (last > first && std::isspace(static_cast<unsigned char>(sv[last - 1]))) {
        --last;
    }
    return std::string(sv.substr(first, last - first));
}

inline bool containsIgnoreCase(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) return true;
    if (haystack.empty()) return false;
    std::string h = toLowerString(haystack);
    std::string n = toLowerString(needle);
    return h.find(n) != std::string::npos;
}

inline bool startsWithIgnoreCase(std::string_view text, std::string_view prefix) {
    if (prefix.empty()) return true;
    if (text.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(text[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace detail

/**
 * @brief Criteria object for filtering and querying questions.
 *
 * Supports combinations of topic, difficulty, status, company, favorite status,
 * keyword matching across multiple text fields, and title prefix.
 */
struct QuestionFilter {
    std::optional<Topic> topic;
    std::optional<Difficulty> difficulty;
    std::optional<Status> status;
    std::optional<std::string> company;
    std::optional<bool> isFavorite;
    std::optional<std::string> keyword;
    std::optional<std::string> titlePrefix;

    /**
     * @brief Checks whether any filter condition is active.
     */
    bool isEmpty() const noexcept {
        if (topic.has_value() || difficulty.has_value() || status.has_value() || isFavorite.has_value()) {
            return false;
        }
        if (company.has_value() && !detail::trimString(*company).empty()) {
            return false;
        }
        if (keyword.has_value() && !detail::trimString(*keyword).empty()) {
            return false;
        }
        if (titlePrefix.has_value() && !detail::trimString(*titlePrefix).empty()) {
            return false;
        }
        return true;
    }

    /**
     * @brief Evaluates whether a given Question satisfies all active criteria.
     *
     * Multiple criteria operate as a logical AND.
     */
    bool matches(const Question& question) const {
        if (topic.has_value() && question.getTopic() != *topic) {
            return false;
        }

        if (difficulty.has_value() && question.getDifficulty() != *difficulty) {
            return false;
        }

        if (status.has_value() && question.getStatus() != *status) {
            return false;
        }

        if (isFavorite.has_value() && question.isFavorite() != *isFavorite) {
            return false;
        }

        if (company.has_value()) {
            std::string trimmedComp = detail::trimString(*company);
            if (!trimmedComp.empty()) {
                if (!detail::containsIgnoreCase(question.getCompany(), trimmedComp)) {
                    return false;
                }
            }
        }

        if (titlePrefix.has_value()) {
            std::string trimmedPrefix = detail::trimString(*titlePrefix);
            if (!trimmedPrefix.empty()) {
                if (!detail::startsWithIgnoreCase(question.getTitle(), trimmedPrefix)) {
                    return false;
                }
            }
        }

        if (keyword.has_value()) {
            std::string trimmedKey = detail::trimString(*keyword);
            if (!trimmedKey.empty()) {
                bool foundInTitle = detail::containsIgnoreCase(question.getTitle(), trimmedKey);
                bool foundInDesc = detail::containsIgnoreCase(question.getDescription(), trimmedKey);
                bool foundInComp = detail::containsIgnoreCase(question.getCompany(), trimmedKey);
                bool foundInTopic = detail::containsIgnoreCase(topicToString(question.getTopic()), trimmedKey);
                bool foundInTag = false;
                for (const auto& tag : question.getTags()) {
                    if (detail::containsIgnoreCase(tag, trimmedKey)) {
                        foundInTag = true;
                        break;
                    }
                }

                if (!foundInTitle && !foundInDesc && !foundInComp && !foundInTopic && !foundInTag) {
                    return false;
                }
            }
        }

        return true;
    }
};

} // namespace codevault::models
