#pragma once

#include "models/enums.hpp"
#include "models/question.hpp"

#include <optional>
#include <string>

namespace codevault::models {

/**
 * @brief Criteria parameters for requesting a deterministic next practice recommendation.
 */
struct PracticeNextCriteria {
    std::optional<Topic> topic;
    std::optional<Difficulty> difficulty;
    bool includeDueRevisions{true};
    bool preferUnsolved{true};
};

/**
 * @brief Result containing the recommended question and transparent rationale.
 */
struct PracticeNextResult {
    bool hasQuestion{false};
    std::optional<Question> question{std::nullopt};
    std::string recommendationReason;
};

} // namespace codevault::models
