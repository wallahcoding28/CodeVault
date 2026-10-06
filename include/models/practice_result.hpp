#pragma once

#include <string>
#include <string_view>

namespace codevault::models {

/**
 * @brief Represents the verdict or result of a practice attempt.
 */
enum class PracticeVerdict {
    Solved,
    NeedsReview,
    Skipped
};

/**
 * @brief Convert PracticeVerdict to a readable string.
 */
inline std::string verdictToString(PracticeVerdict verdict) {
    switch (verdict) {
        case PracticeVerdict::Solved:      return "Solved";
        case PracticeVerdict::NeedsReview: return "Needs Review";
        case PracticeVerdict::Skipped:     return "Skipped";
        default:                           return "Unknown";
    }
}

/**
 * @brief Parse PracticeVerdict from string or numeric choice.
 */
inline PracticeVerdict stringToVerdict(std::string_view str) {
    if (str == "Solved" || str == "solved" || str == "1") {
        return PracticeVerdict::Solved;
    }
    if (str == "NeedsReview" || str == "Needs Review" || str == "needs_review" || str == "2") {
        return PracticeVerdict::NeedsReview;
    }
    return PracticeVerdict::Skipped;
}

} // namespace codevault::models
