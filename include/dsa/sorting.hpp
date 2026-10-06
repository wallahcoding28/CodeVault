#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

namespace codevault::dsa {

/**
 * @brief Stable Merge Sort implementation with O(N log N) worst-case time complexity.
 *
 * Guarantees stability: items with equal comparison keys retain their relative initial order.
 *
 * Complexities:
 * - Best:    O(N log N)
 * - Average: O(N log N)
 * - Worst:   O(N log N)
 * - Space:   O(N) auxiliary vector allocation
 *
 * @param first Beginning of random-access range.
 * @param last  End of random-access range.
 * @param comp  Binary comparator returning true if left < right.
 */
template <typename RandomIt, typename Compare>
void merge(RandomIt first, RandomIt mid, RandomIt last, Compare comp) {
    using ValueType = typename std::iterator_traits<RandomIt>::value_type;
    std::vector<ValueType> temp;
    temp.reserve(static_cast<size_t>(std::distance(first, last)));

    RandomIt left = first;
    RandomIt right = mid;

    while (left != mid && right != last) {
        // Enforce stability: take left item if !comp(*right, *left)
        if (comp(*right, *left)) {
            temp.push_back(std::move(*right));
            ++right;
        } else {
            temp.push_back(std::move(*left));
            ++left;
        }
    }

    while (left != mid) {
        temp.push_back(std::move(*left));
        ++left;
    }

    while (right != last) {
        temp.push_back(std::move(*right));
        ++right;
    }

    for (size_t i = 0; i < temp.size(); ++i) {
        *(first + i) = std::move(temp[i]);
    }
}

template <typename RandomIt, typename Compare = std::less<>>
void mergeSort(RandomIt first, RandomIt last, Compare comp = Compare()) {
    auto count = std::distance(first, last);
    if (count <= 1) {
        return;
    }
    RandomIt mid = first + count / 2;
    mergeSort(first, mid, comp);
    mergeSort(mid, last, comp);
    merge(first, mid, last, comp);
}

/**
 * @brief In-place Quick Sort implementation with median-of-three pivot selection
 *        and tail-call recursion optimization.
 *
 * Complexities:
 * - Best:    O(N log N)
 * - Average: O(N log N)
 * - Worst:   O(N^2) (mitigated by median-of-three pivot)
 * - Space:   O(log N) stack frames
 *
 * @param first Beginning of random-access range.
 * @param last  End of random-access range.
 * @param comp  Binary comparator returning true if left < right.
 */
template <typename RandomIt, typename Compare>
RandomIt partitionLomuto(RandomIt first, RandomIt last, Compare comp) {
    auto len = std::distance(first, last);
    if (len <= 1) return first;

    RandomIt mid = first + len / 2;
    RandomIt lastElem = last - 1;

    // Median-of-three pivot selection
    if (comp(*mid, *first)) std::iter_swap(first, mid);
    if (comp(*lastElem, *first)) std::iter_swap(first, lastElem);
    if (comp(*lastElem, *mid)) std::iter_swap(mid, lastElem);

    std::iter_swap(mid, lastElem);

    RandomIt i = first;
    for (RandomIt j = first; j < lastElem; ++j) {
        if (comp(*j, *lastElem)) {
            if (i != j) {
                std::iter_swap(i, j);
            }
            ++i;
        }
    }
    std::iter_swap(i, lastElem);
    return i;
}

template <typename RandomIt, typename Compare = std::less<>>
void quickSort(RandomIt first, RandomIt last, Compare comp = Compare()) {
    while (std::distance(first, last) > 1) {
        RandomIt p = partitionLomuto(first, last, comp);
        // Tail-call recursion elimination: recurse on smaller slice, loop on larger
        if (p - first < last - (p + 1)) {
            quickSort(first, p, comp);
            first = p + 1;
        } else {
            quickSort(p + 1, last, comp);
            last = p;
        }
    }
}

} // namespace codevault::dsa
