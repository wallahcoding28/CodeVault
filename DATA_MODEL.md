# CodeVault — Domain Data Model Specification

## 1. Overview
The CodeVault data model is designed to be lightweight, strongly-typed, and future-proof. In Stage 1, the core entity is the **`Question`**, representing a single algorithmic coding problem tracked by the user.

---

## 2. The `Question` Entity Schema

```
┌────────────────────────────────────────────────────────┐
│                        Question                        │
├────────────────────────────────────────────────────────┤
│ id                 : string (UUID / alphanumeric slug) │
│ title              : string                            │
│ description        : string (problem summary)          │
│ topic              : Topic (enum)                      │
│ difficulty         : Difficulty (enum)                 │
│ company            : string (primary company)          │
│ platform           : Platform (enum / string)          │
│ source_url         : string                            │
│ status             : Status (enum)                     │
│ is_favorite        : boolean                           │
│ notes              : string (markdown formatted)       │
│ created_at         : int64 (Unix timestamp, seconds)   │
│ updated_at         : int64 (Unix timestamp, seconds)   │
│ last_practiced_at  : int64 (Unix timestamp, seconds)   │
│ next_revision_at   : int64 (Unix timestamp, seconds)   │
│ revision_priority  : int32 (1: High, 2: Med, 3: Low)   │
│ tags               : vector<string>                    │
├────────────────────────────────────────────────────────┤
│ owner_id           : string (Multi-user reservation)   │
└────────────────────────────────────────────────────────┘
```

---

## 3. Detailed Field Definitions

| Field Name | Type | Constraints / Validation | Description |
| :--- | :--- | :--- | :--- |
| `id` | `std::string` | Non-empty, unique, URL-safe slug or auto-incremented string (e.g. `Q-1001`). | Primary identifier for hash indexing and persistence. |
| `title` | `std::string` | 1–255 characters, non-whitespace. | Name of the problem (e.g. "Two Sum"). |
| `description` | `std::string` | Optional, text summary or key problem statement. | Quick problem synopsis for quick recall without opening the URL. |
| `topic` | `Topic` (enum) | Value from defined `Topic` enum. | Primary algorithmic category (e.g. `Arrays`, `Graphs`, `DynamicProgramming`). |
| `difficulty` | `Difficulty` (enum) | `Easy`, `Medium`, `Hard`. | Standard complexity tier. |
| `company` | `std::string` | Max 100 characters. | Target hiring company (e.g. "Google", "Amazon", "Meta"). |
| `platform` | `Platform` (enum) | `LeetCode`, `HackerRank`, `Codeforces`, `GeeksforGeeks`, `CodeStudio`, `Custom`. | Origin platform of the problem. |
| `source_url` | `std::string` | Valid web URL format or empty. | Direct link to the online judge problem. |
| `status` | `Status` (enum) | `Todo`, `Attempted`, `Solved`, `Mastered`. | Current student completion status. |
| `is_favorite` | `bool` | `true` or `false`. | Quick bookmark flag for high-priority review. |
| `notes` | `std::string` | Plaintext / Markdown. | Personal hints, optimal time/space complexities, gotchas. |
| `created_at` | `int64_t` | Positive Unix epoch timestamp. | When the record was first created. |
| `updated_at` | `int64_t` | Positive Unix epoch timestamp. | Timestamp of the most recent field modification. |
| `last_practiced_at`| `int64_t` | Epoch timestamp or 0 if never practiced. | Most recent practice session completion time. |
| `next_revision_at` | `int64_t` | Epoch timestamp. | Scheduled date for the next spaced repetition review. Used by Min-Heap. |
| `revision_priority`| `int32_t` | 1 (Urgent/High) to 5 (Low). | Priority score used to break ties in revision queue. |
| `tags` | `std::vector<std::string>` | Lowercase normalized strings. | Auxiliary tags (e.g., `["hash-table", "two-pointers"]`). Used by Trie. |
| `owner_id` | `std::string` | Non-empty string. | Reserved field for multi-user tenancy. Defaults to `"local_user"` in Stage 1. |

---

## 4. Enumerations & Value Objects

### 4.1 `Difficulty`
```cpp
enum class Difficulty {
    Easy,
    Medium,
    Hard,
    Unknown
};
```

### 4.2 `Status`
```cpp
enum class Status {
    Unsolved,
    InProgress,
    Solved,
    Mastered,
    // Aliases for compatibility
    Todo = Unsolved,
    Attempted = InProgress
};
```

### 4.3 `Topic`
```cpp
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
```

### 4.4 `Platform`
```cpp
enum class Platform {
    LeetCode,
    HackerRank,
    Codeforces,
    GeeksforGeeks,
    CodeStudio,
    Custom
};
```

---

## 5. Multi-User Future Expansion Path

While Stage 1 operates purely for a single local user, the entity architecture avoids the costly refactoring trap of single-user lock-in:
1. **The `owner_id` Attribute**:
   Every record includes an `owner_id`. In Stage 1, all questions written or read assume `owner_id == "local_user"`.
2. **User Profiles (Future Entity)**:
   In Stage 9, a `User` entity will be introduced:
   - `user_id` (string)
   - `username` (string)
   - `email` (string)
   - `created_at` (int64)
3. **Multi-Tenancy Filtering**:
   When accounts are activated, repository queries will filter records with `WHERE owner_id = :current_user_id`, leaving all underlying DSA logic, Trie indexing, and Min-Heap structures functioning identically per-user.
