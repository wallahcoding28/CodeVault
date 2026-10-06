#pragma once

#include <cstdint>
#include <string>

namespace codevault::models {

/**
 * @brief Represents an item scheduled for algorithmic revision.
 */
struct RevisionItem {
    std::string questionId;
    int64_t nextRevisionAt{0};
    int32_t priority{2}; // 1 = High, 5 = Low

    RevisionItem() = default;

    RevisionItem(std::string id, int64_t revisionAt, int32_t prio = 2)
        : questionId(std::move(id)), nextRevisionAt(revisionAt), priority(prio) {}

    bool operator==(const RevisionItem& other) const noexcept {
        return questionId == other.questionId &&
               nextRevisionAt == other.nextRevisionAt &&
               priority == other.priority;
    }
};

/**
 * @brief Comparator for MinHeap revision ordering.
 *
 * Earliest scheduled timestamp (smallest nextRevisionAt) comes first.
 * For equal timestamps, highest urgency (smallest priority value) comes first.
 */
struct RevisionItemComparator {
    bool operator()(const RevisionItem& a, const RevisionItem& b) const noexcept {
        if (a.nextRevisionAt != b.nextRevisionAt) {
            return a.nextRevisionAt < b.nextRevisionAt;
        }
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.questionId < b.questionId;
    }
};

} // namespace codevault::models
