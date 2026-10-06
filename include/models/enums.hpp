#pragma once

#include <string>
#include <string_view>

namespace codevault::models {

enum class Difficulty {
    Easy,
    Medium,
    Hard,
    Unknown
};

enum class Status {
    Unsolved,
    InProgress,
    Solved,
    Mastered,
    // Aliases for compatibility
    Todo = Unsolved,
    Attempted = InProgress
};

enum class Topic {
    Arrays,
    Strings,
    LinkedLists,
    StacksQueues,
    Trees,
    Graphs,
    DynamicProgramming,
    BinarySearch,
    RecursionBacktracking,
    Greedy,
    Heaps,
    BitManipulation,
    MathGeometry,
    Other
};

enum class Platform {
    LeetCode,
    HackerRank,
    Codeforces,
    GeeksforGeeks,
    CodeStudio,
    Custom
};

// Case-insensitive string equality helper
inline bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// Difficulty conversions
inline std::string difficultyToString(Difficulty diff) {
    switch (diff) {
        case Difficulty::Easy:   return "Easy";
        case Difficulty::Medium: return "Medium";
        case Difficulty::Hard:   return "Hard";
        default:                 return "Unknown";
    }
}

inline Difficulty stringToDifficulty(std::string_view str) {
    if (iequals(str, "Easy") || str == "1")     return Difficulty::Easy;
    if (iequals(str, "Medium") || str == "2")   return Difficulty::Medium;
    if (iequals(str, "Hard") || str == "3")     return Difficulty::Hard;
    return Difficulty::Unknown;
}

inline bool isValidDifficulty(std::string_view str) {
    return stringToDifficulty(str) != Difficulty::Unknown;
}

// Status conversions
inline std::string statusToString(Status status) {
    switch (status) {
        case Status::Unsolved:   return "Unsolved";
        case Status::InProgress: return "InProgress";
        case Status::Solved:     return "Solved";
        case Status::Mastered:   return "Mastered";
        default:                 return "Unsolved";
    }
}

inline Status stringToStatus(std::string_view str) {
    if (iequals(str, "Solved") || str == "3")                                      return Status::Solved;
    if (iequals(str, "InProgress") || iequals(str, "In Progress") ||
        iequals(str, "Attempted") || str == "2")                                   return Status::InProgress;
    if (iequals(str, "Mastered") || str == "4")                                    return Status::Mastered;
    if (iequals(str, "Unsolved") || iequals(str, "Todo") || str == "1")           return Status::Unsolved;
    return Status::Unsolved;
}

inline bool isValidStatus(std::string_view str) {
    return iequals(str, "Solved") || iequals(str, "InProgress") ||
           iequals(str, "In Progress") || iequals(str, "Attempted") ||
           iequals(str, "Mastered") || iequals(str, "Unsolved") ||
           iequals(str, "Todo") || str == "1" || str == "2" || str == "3" || str == "4";
}

// Topic conversions
inline std::string topicToString(Topic topic) {
    switch (topic) {
        case Topic::Arrays:                return "Arrays";
        case Topic::Strings:               return "Strings";
        case Topic::LinkedLists:           return "LinkedLists";
        case Topic::StacksQueues:          return "StacksQueues";
        case Topic::Trees:                 return "Trees";
        case Topic::Graphs:                return "Graphs";
        case Topic::DynamicProgramming:    return "DynamicProgramming";
        case Topic::BinarySearch:          return "BinarySearch";
        case Topic::RecursionBacktracking: return "RecursionBacktracking";
        case Topic::Greedy:                return "Greedy";
        case Topic::Heaps:                 return "Heaps";
        case Topic::BitManipulation:       return "BitManipulation";
        case Topic::MathGeometry:          return "MathGeometry";
        default:                           return "Other";
    }
}

inline Topic stringToTopic(std::string_view str) {
    if (iequals(str, "Arrays") || iequals(str, "Array"))                         return Topic::Arrays;
    if (iequals(str, "Strings") || iequals(str, "String"))                       return Topic::Strings;
    if (iequals(str, "LinkedLists") || iequals(str, "Linked List") ||
        iequals(str, "LinkedList"))                                              return Topic::LinkedLists;
    if (iequals(str, "StacksQueues") || iequals(str, "Stack") ||
        iequals(str, "Queue") || iequals(str, "Stacks") ||
        iequals(str, "Queues"))                                                  return Topic::StacksQueues;
    if (iequals(str, "Trees") || iequals(str, "Tree") ||
        iequals(str, "BinaryTree"))                                              return Topic::Trees;
    if (iequals(str, "Graphs") || iequals(str, "Graph"))                         return Topic::Graphs;
    if (iequals(str, "DynamicProgramming") || iequals(str, "DP") ||
        iequals(str, "Dynamic Programming"))                                     return Topic::DynamicProgramming;
    if (iequals(str, "BinarySearch") || iequals(str, "Binary Search"))           return Topic::BinarySearch;
    if (iequals(str, "RecursionBacktracking") || iequals(str, "Recursion") ||
        iequals(str, "Backtracking"))                                            return Topic::RecursionBacktracking;
    if (iequals(str, "Greedy"))                                                  return Topic::Greedy;
    if (iequals(str, "Heaps") || iequals(str, "Heap") ||
        iequals(str, "PriorityQueue"))                                           return Topic::Heaps;
    if (iequals(str, "BitManipulation") || iequals(str, "Bit Manipulation") ||
        iequals(str, "Bits"))                                                    return Topic::BitManipulation;
    if (iequals(str, "MathGeometry") || iequals(str, "Math") ||
        iequals(str, "Geometry"))                                                return Topic::MathGeometry;
    return Topic::Other;
}

// Platform conversions
inline std::string platformToString(Platform platform) {
    switch (platform) {
        case Platform::LeetCode:      return "LeetCode";
        case Platform::HackerRank:    return "HackerRank";
        case Platform::Codeforces:    return "Codeforces";
        case Platform::GeeksforGeeks: return "GeeksforGeeks";
        case Platform::CodeStudio:    return "CodeStudio";
        default:                      return "Custom";
    }
}

inline Platform stringToPlatform(std::string_view str) {
    if (iequals(str, "LeetCode") || iequals(str, "LC"))         return Platform::LeetCode;
    if (iequals(str, "HackerRank") || iequals(str, "HR"))       return Platform::HackerRank;
    if (iequals(str, "Codeforces") || iequals(str, "CF"))       return Platform::Codeforces;
    if (iequals(str, "GeeksforGeeks") || iequals(str, "GFG"))   return Platform::GeeksforGeeks;
    if (iequals(str, "CodeStudio"))                             return Platform::CodeStudio;
    return Platform::Custom;
}

} // namespace codevault::models
