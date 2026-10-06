#pragma once

#include <string>

namespace codevault::models {

/**
 * @brief Available fields for sorting question collections.
 */
enum class SortField {
    Title,
    Difficulty,
    Topic,
    Status,
    Company,
    CreatedAt,
    UpdatedAt,
    RevisionPriority
};

/**
 * @brief Direction for ordering sorted results.
 */
enum class SortDirection {
    Ascending,
    Descending
};

/**
 * @brief Algorithmic sorting strategy.
 */
enum class SortAlgorithm {
    MergeSort,
    QuickSort
};

/**
 * @brief Encapsulates sorting preferences for queries and displays.
 */
struct SortOptions {
    SortField field{SortField::Title};
    SortDirection direction{SortDirection::Ascending};
    SortAlgorithm algorithm{SortAlgorithm::MergeSort};

    constexpr SortOptions() = default;
    constexpr SortOptions(SortField f, SortDirection d = SortDirection::Ascending, SortAlgorithm a = SortAlgorithm::MergeSort)
        : field(f), direction(d), algorithm(a) {}
};

inline std::string sortFieldToString(SortField field) {
    switch (field) {
        case SortField::Title:            return "Title";
        case SortField::Difficulty:       return "Difficulty";
        case SortField::Topic:            return "Topic";
        case SortField::Status:           return "Status";
        case SortField::Company:          return "Company";
        case SortField::CreatedAt:        return "Created Date";
        case SortField::UpdatedAt:        return "Updated Date";
        case SortField::RevisionPriority: return "Revision Priority";
        default:                          return "Title";
    }
}

inline std::string sortDirectionToString(SortDirection dir) {
    return (dir == SortDirection::Ascending) ? "Ascending" : "Descending";
}

inline std::string sortAlgorithmToString(SortAlgorithm algo) {
    return (algo == SortAlgorithm::MergeSort) ? "MergeSort" : "QuickSort";
}

} // namespace codevault::models
