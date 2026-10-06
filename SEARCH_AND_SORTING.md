# CodeVault — Search, Filtering & Sorting (Stage 4)

## 1. Overview & Objective

Stage 4 enhances CodeVault with a high-performance, robust search, filtering, and sorting subsystem. The implementation bridges clean application architecture with custom educational Data Structures & Algorithms (DSA), providing:
- **Instant Prefix Autocomplete**: Powered by the custom `PrefixTrie`.
- **Full-Field Keyword Search**: Multi-field scanning across title, description, company, topic, and tags.
- **Unified Multi-Criteria Filtering**: Composable filtering by Topic, Difficulty, Status, Company, and Favorite flags.
- **Custom Algorithmic Sorting**: Hand-crafted **Merge Sort** (stable) and **Quick Sort** (in-place) without reliance on STL sort algorithms.
- **Deterministic Ordering**: Reliable secondary key tie-breaking by Question ID.
- **Repository Safety**: All view queries and sorts operate on isolated collections, guaranteeing zero mutation to the underlying persistent CSV catalog.

---

## 2. Architecture & Design

The Stage 4 subsystem adheres to CodeVault's Clean Architecture layer separation:

```
┌────────────────────────────────────────────────────────┐
│                   Presentation Layer                   │
│      CliShell: Search Menu & Formatted Table Display   │
└───────────────────────────┬────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│                    Application Layer                   │
│  SearchService: Coordinates Trie, Filters, and Sorters │
└─────────────┬────────────────────────────┬─────────────┘
              │ Coordinates                │ Operates on
┌─────────────▼──────────────┐ ┌───────────▼─────────────┐
│       Core / DSA Layer     │ │       Domain Models     │
│  - PrefixTrie              │ │  - Question             │
│  - mergeSort (Stable)      │ │  - QuestionFilter       │
│  - quickSort (In-Place)    │ │  - SortOptions          │
└────────────────────────────┘ └─────────────────────────┘
```

---

## 3. Search Engine Specifications

### 3.1 Title Prefix Lookup & Autocomplete (`PrefixTrie`)
- **Engine**: Custom `codevault::dsa::PrefixTrie`.
- **Operation**:
  - `searchTitlesByPrefix(prefix)`: Traverses Trie nodes matching the query prefix and collects completions.
  - `searchByTitlePrefix(questions, prefix)`: Maps matched Trie titles to indexed Question IDs, returning matched question objects.
- **Time Complexity**:
  - Traversal: $O(L)$, where $L$ is prefix length.
  - Suggestion Enumeration: $O(K)$, where $K$ is the number of nodes in the matched subtree.
- **Case Normalization**: Case-insensitive (queries normalized to lowercase).

### 3.2 Broad Keyword Search
- **Operation**: `searchByKeyword(questions, keyword)`.
- **Field Coverage**: Scans Question Title, Description, Company, Topic Name, and all associated Tags.
- **String Handling**: Safe whitespace trimming and case-insensitive substring matching.
- **Time Complexity**: $O(N \cdot M)$, where $N$ is question count and $M$ is total text length per question.

---

## 4. Multi-Criteria Filtering

### 4.1 Filter Criteria (`QuestionFilter`)
Filtering is encapsulated in a dedicated criteria struct rather than sprawling parameter lists:

```cpp
struct QuestionFilter {
    std::optional<Topic> topic;
    std::optional<Difficulty> difficulty;
    std::optional<Status> status;
    std::optional<std::string> company;
    std::optional<bool> isFavorite;
    std::optional<std::string> keyword;
    std::optional<std::string> titlePrefix;

    bool isEmpty() const noexcept;
    bool matches(const Question& question) const;
};
```

### 4.2 Filtering Behavior
- **Multi-Filter Combination**: Operates as a logical `AND`. A question must satisfy all active constraints.
- **Empty Filter Behavior**: If no filter is set (or fields are empty whitespace), matches all questions.
- **Safe Matching**: Empty strings or unselected fields are ignored. Company matching supports case-insensitive substring discovery.

---

## 5. Custom Sorting Algorithms

Per DSA Lab requirements, sorting is implemented using custom educational algorithms rather than `std::sort`.

### 5.1 Custom Merge Sort (`codevault::dsa::mergeSort`)
- **Design**: Stable divide-and-conquer implementation.
- **Guaranteed Stability**: Elements with equal primary keys preserve their original relative insertion order.
- **Complexity**:
  - Best Case: $O(N \log N)$
  - Average Case: $O(N \log N)$
  - Worst Case: $O(N \log N)$
  - Auxiliary Memory: $O(N)$ for temporary merge buffer.

### 5.2 Custom Quick Sort (`codevault::dsa::quickSort`)
- **Design**: In-place recursive sorting with optimizations.
- **Pivot Selection**: **Median-of-three** ($first$, $mid$, $last - 1$) prevents worst-case quadratic behavior on already-sorted or reverse-sorted sequences.
- **Recursion Optimization**: Tail-call recursion elimination recurses on the smaller partition and loops on the larger partition, guaranteeing $O(\log N)$ maximum call stack depth.
- **Complexity**:
  - Best Case: $O(N \log N)$
  - Average Case: $O(N \log N)$
  - Worst Case: $O(N^2)$ (mitigated by median-of-three pivot)
  - Auxiliary Memory: $O(\log N)$ stack frames.

### 5.3 Supported Sort Fields & Deterministic Tie-Breaking
Questions can be sorted in **Ascending** or **Descending** order across:
1. **Title**: Alphabetical lexicographical ordering.
2. **Difficulty**: Easy ($1$) $\rightarrow$ Medium ($2$) $\rightarrow$ Hard ($3$).
3. **Topic**: Alphabetical ordering of topic enum strings.
4. **Status**: Workflow progression: Unsolved ($1$) $\rightarrow$ InProgress ($2$) $\rightarrow$ Solved ($3$) $\rightarrow$ Mastered ($4$).
5. **Company**: Alphabetical ordering.
6. **Created Date**: Timestamp ordering.
7. **Updated Date**: Timestamp ordering.
8. **Revision Priority**: Numerical priority ordering.

**Deterministic Tie-Breaking**:
If two questions evaluate to equal values on the primary sort field, secondary ordering is strictly resolved by **Question ID** (`a.getId() < b.getId()`), guaranteeing stable, reproducible output across all runs.

---

## 6. CLI Integration

The interactive CLI shell (`CliShell`) provides a dedicated submenu accessible via Option 2 from the main menu:

```
================================
        SEARCH & FILTER         
================================
1. Search by title prefix (Trie)
2. Search by keyword (All fields)
3. Filter questions
4. Sort questions (Merge / Quick Sort)
5. Search + Filter + Sort
6. Reset / Show all
0. Back
================================
Enter choice:
```

### Table Output Formatting
Results are rendered in a clean, fixed-width columnar table:
```
ID        Title                         Topic             Difficulty  Company       Status       Fav
------------------------------------------------------------------------------------------------------
Q-1001    Two Sum                       Arrays            Easy        Google        Solved       [*]
Q-1002    Binary Search                 BinarySearch      Easy        Amazon        Solved        - 
Q-1007    Number of Islands             Graphs            Medium      Google        Unsolved     [*]
------------------------------------------------------------------------------------------------------
Total Questions: 3
```
- Long titles (>28 chars) and company names are cleanly truncated with ellipsis (`...`).
- When zero items match, displays: `No questions matched the selected criteria.`

---

## 7. Automated Testing & Verification

The test suite in `tests/unit/unit_tests.cpp` was expanded with 43 comprehensive unit and integration tests:

| Test Group | Test Count | Key Invariants Verified |
| :--- | :--- | :--- |
| **MergeSort (Custom DSA)** | 6 | Ascending, descending, duplicate keys, sorted/reversed, empty/single, **mathematical stability**. |
| **QuickSort (Custom DSA)** | 6 | Ascending, descending, duplicates, sorted/reversed, empty/single, median-of-3 partition. |
| **Prefix Search (Trie)** | 6 | Exact match, prefix match, case-insensitivity, no match, deleted question pruning, title update. |
| **Keyword Search** | 2 | Match across title, description, tags, company; case-insensitive tolerance. |
| **Criteria Filtering** | 8 | Topic only, difficulty only, status only, company substring, favorites, multi-criteria AND, empty filter. |
| **Question Sorting** | 8 | Title asc/desc, difficulty asc/desc, topic, company, status, tie-breaking by ID, algorithm parity. |
| **Combined Workflows** | 3 | Search + Filter + Sort pipelines; master collection immutability guarantee. |

**Current Suite Status**:
- Unit Tests: **122 / 122 PASSED (100%)**
- Smoke Tests: **5 / 5 PASSED (100%)**
- Clean CMake Build: **0 compilation errors, 0 warnings**
