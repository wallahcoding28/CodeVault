# CodeVault — Data Structures & Algorithms (DSA) Design Specification

## 1. Pedagogical & Architectural Thesis

CodeVault balances two core design requirements:
1. **Academic Rigor**: Providing authentic, decoupled, educational custom implementations of fundamental data structures as expected in a computer science curriculum (without wrapping `std::priority_queue`, `std::queue`, `std::stack`, or `std::list`).
2. **Production Integrity**: Guaranteeing that every data structure possesses a distinct, non-redundant system responsibility, with predictable memory safety (RAII, Rule of 5), encapsulation, and mathematically accurate time complexity bounds.

---

## 2. Core DSA Mapping Matrix

| Data Structure | CodeVault Purpose | Main Operations | Complexity | Status |
| :--- | :--- | :--- | :--- | :--- |
| **Trie** (`PrefixTrie`) | Prefix search & title autocomplete | `insert`, `contains`, `startsWith`, `autocomplete`, `remove` | Insert: $O(L)$<br>Search: $O(L)$<br>Prefix: $O(L + K \log K)$<br>Remove: $O(L)$ | **IMPLEMENTED** |
| **Min-Heap** (`MinHeap<T>`) | Revision priority scheduling | `push`, `pop`, `top`, `extractMin` | Push: $O(\log N)$<br>Pop: $O(\log N)$<br>Top: $O(1)$ | **IMPLEMENTED** |
| **Queue** (`Queue<T>`) | Practice session problem progression (FIFO) | `enqueue`, `dequeue`, `front`, `back` | Enqueue: $O(1)$<br>Dequeue: $O(1)$<br>Front/Back: $O(1)$ | **IMPLEMENTED** |
| **Stack** (`Stack<T>`) | Recent question navigation history (LIFO) | `push`, `pop`, `top` | Push: $O(1)$<br>Pop: $O(1)$<br>Top: $O(1)$ | **IMPLEMENTED** |
| **Doubly Linked List** (`DoublyLinkedList<T>`) | Ordered problem playlist & bidirectional traversal | `pushFront`, `pushBack`, `popFront`, `popBack`, `insert`, `removeAt`, `traverse` | Head/Tail Insert/Remove: $O(1)$<br>Index Lookup/Remove: $O(N)$<br>Node Splicing: $O(1)$<br>Traversal: $O(N)$ | **IMPLEMENTED** |
| **Dynamic Array** (`std::vector<T>`) | Primary contiguous catalog storage | Random access, append | Access: $O(1)$<br>Append: $O(1)$ amortized | **IMPLEMENTED** |
| **Merge Sort** (`dsa::mergeSort`) | Stable multi-parameter question ordering | `mergeSort(first, last, comp)` | Time: $O(N \log N)$ all cases<br>Space: $O(N)$ auxiliary | **IMPLEMENTED** |
| **Quick Sort** (`dsa::quickSort`) | In-place question reordering | `quickSort(first, last, comp)` | Time: $O(N \log N)$ avg, $O(N^2)$ worst<br>Space: $O(\log N)$ stack frames | **IMPLEMENTED** |

*Where $L$ is string length, $K$ is number of matching descendant words, and $N$ is element count.*

---

## 3. Deep-Dive Design per Data Structure

### 3.1 Prefix Trie (`codevault::dsa::PrefixTrie`)
- **CodeVault Problem**: Scanning flat arrays for title substrings takes $O(N \times L)$, becoming unacceptable as problem banks scale. A Prefix Trie guarantees prefix lookup bounded solely by query length $L$, invariant to question count $N$.
- **Implementation**:
  - Located in `include/dsa/trie.hpp` and `src/dsa/trie.cpp`.
  - Node contains `std::unordered_map<char, std::unique_ptr<TrieNode>>` supporting all alphanumeric characters, spaces, and punctuation (`(`, `)`, `-`, `+`).
  - Terminal nodes store `original_word` to preserve original display casing.
  - Case-insensitive routing via internal normalization helper.
  - Safe handling of empty strings, duplicate insertions, and node pruning upon deletion.
- **Key Operations**:
  - `insert(word)`: $O(L)$
  - `contains(word)`: $O(L)$
  - `startsWith(prefix)`: $O(L)$
  - `autocomplete(prefix)`: $O(L + K \log K)$ with deterministic alphabetical sorting
  - `remove(word)`: $O(L)$ with recursive pruning of non-terminal leaves
- **Service Integration**: Managed by `SearchService`. Automatically populated on question ingestion; incrementally synchronized on `createQuestion`, `updateQuestion`, and `deleteQuestion` inside `QuestionService`.

### 3.2 Min-Heap Priority Queue (`codevault::dsa::MinHeap<T, Compare>`)
- **CodeVault Problem**: Spaced revision scheduling requires identifying which problem is due next or overdue without repeated $O(N \log N)$ sorts. A binary Min-Heap maintains the earliest due item at root in $O(1)$ peek time.
- **Implementation**:
  - Located in `include/dsa/min_heap.hpp`.
  - Generic templated binary heap over contiguous `std::vector<T>` storage (does not use `std::priority_queue`).
  - Custom `heapifyUp` and `heapifyDown` routines with parent index `(i-1)/2`, left child `2i+1`, right child `2i+2`.
  - Handles duplicate priorities deterministically.
- **Key Operations**:
  - `push(item)`: $O(\log N)$
  - `pop()`: $O(\log N)$
  - `top()`: $O(1)$
  - `extractMin()`: $O(\log N)$
- **Service Integration**: Fully integrated with `RevisionService` using `models::RevisionItem` and `models::RevisionItemComparator`. Prioritizes by `nextRevisionAt` ascending, breaking ties by `priority` urgency (1=High, 5=Low) and deterministically by `questionId` (Stage 5). Supports `getDueQuestions()`, `getUpcomingRevisions()`, and schedule calculations.

### 3.3 Custom FIFO Queue (`codevault::dsa::Queue<T>`)
- **CodeVault Problem**: Daily study sprints require deterministic FIFO question progression. Array-based queues incur $O(N)$ shift penalties on dequeue; this custom linked-node queue guarantees strict $O(1)$ operations.
- **Implementation**:
  - Located in `include/dsa/queue.hpp`.
  - Singly-linked node structure with dedicated `head_` and `tail_` pointers and `size_` counter (does not use `std::queue`).
  - Rule of 5: Deep copying constructor/assignment, no-leak destructor, zero-allocation move semantics.
- **Key Operations**:
  - `enqueue(item)`: $O(1)$
  - `dequeue()`: $O(1)$
  - `front()` / `back()`: $O(1)$
  - `empty()` / `size()`: $O(1)$
- **Service Integration**: Embedded inside `PracticeService` to manage active practice sessions, supporting question completion, queue inspection, verdict recording (`Solved`, `NeedsReview`, `Skipped`), and skip-to-back problem cycling in strict FIFO order (Stage 5).

### 3.4 Custom LIFO Stack (`codevault::dsa::Stack<T>`)
- **CodeVault Problem**: Navigation across problems requires a "Recently Viewed / Back" mechanism. The custom Stack maintains LIFO inspection history.
- **Implementation**:
  - Located in `include/dsa/stack.hpp`.
  - Singly-linked node structure with `top_` pointer and `size_` counter (does not use `std::stack`).
  - Rule of 5 memory safety.
- **Key Operations**:
  - `push(item)`: $O(1)$
  - `pop()`: $O(1)$
  - `top()`: $O(1)$
  - `empty()` / `size()`: $O(1)$
- **Service Integration**: Embedded inside `RecentHistoryService`. Suppresses consecutive duplicate views and allows `goBack()` and `getHistory()` operations. Hooked into CLI problem view operations.

### 3.5 Custom Doubly Linked List (`codevault::dsa::DoublyLinkedList<T>`)
- **CodeVault Problem**: Interactive problem session walkthroughs require bidirectional movement (Next / Previous problem) and $O(1)$ node insertion/removal without array shifting.
- **Implementation**:
  - Located in `include/dsa/doubly_linked_list.hpp`.
  - Node structure with `data`, `prev`, and `next` pointers (does not use `std::list`).
  - Bidirectional forward and const iterators (`begin()`, `end()`).
  - Forward (`toVector`) and reverse (`toReverseVector`) sequence dumping.
  - Robust boundary handling: empty list, single node, head/tail/middle insertion and deletion.
  - Rule of 5 compliant with deep copy and move semantics.
- **Key Operations**:
  - `pushFront(val)` / `pushBack(val)`: $O(1)$
  - `popFront()` / `popBack()`: $O(1)$
  - `insert(index, val)`: $O(N)$ by index ($O(1)$ node-level)
  - `remove(val)` / `removeAt(index)`: $O(N)$ by value/index ($O(1)$ node-level)
  - `front()` / `back()`: $O(1)$
  - `begin()` / `end()` traversal: $O(N)$
- **Service Integration**: Integrated with `ProblemPlaylistService`, enabling sequential study playlist management with next/previous stepping, in-playlist problem insertion, and removal.

### 3.6 Custom Merge Sort (`codevault::dsa::mergeSort`)
- **CodeVault Problem**: Displaying questions sorted by difficulty, title, or status requires guaranteed stability so that questions with equivalent primary keys retain their original relative order (or secondary ID order).
- **Implementation**:
  - Located in `include/dsa/sorting.hpp`.
  - Recursive divide-and-conquer implementation with temporary merge vector.
  - Takes arbitrary random-access range `[first, last)` and configurable binary comparator.
  - Enforces stability during merge step: takes left element if `!comp(*right, *left)`.
- **Complexity**:
  - Best Case: $O(N \log N)$
  - Average Case: $O(N \log N)$
  - Worst Case: $O(N \log N)$
  - Space: $O(N)$ auxiliary allocation.
- **Service Integration**: Primary sorting strategy in `SearchService::sort()`.

### 3.7 Custom Quick Sort (`codevault::dsa::quickSort`)
- **CodeVault Problem**: Educational demonstration of in-place sorting and partition algorithms without auxiliary buffer allocation.
- **Implementation**:
  - Located in `include/dsa/sorting.hpp`.
  - Median-of-three pivot selection ($first$, $mid$, $last - 1$) prevents worst-case quadratic degradation on sorted/reverse-sorted data.
  - Lomuto partitioning scheme.
  - Tail-call recursion elimination: always recurses on the smaller partition and iterates on the larger partition, guaranteeing $O(\log N)$ maximum call stack depth.
- **Complexity**:
  - Best Case: $O(N \log N)$
  - Average Case: $O(N \log N)$
  - Worst Case: $O(N^2)$ (mitigated by median-of-three pivot)
  - Space: $O(\log N)$ stack frames.
- **Service Integration**: Selectable alternative in `SearchService::sort()` via `SortAlgorithm::QuickSort`.

---

## 4. Architectural Boundaries: Custom DSA vs. Production Infrastructure

1. **Pure DSA Layer (`include/dsa/`, `src/dsa/`)**: UI-agnostic, persistence-agnostic, and model-agnostic. Contains only generic data structures and core algorithms.
2. **Service Layer (`include/services/`, `src/services/`)**: Bridges domain models with custom DSA engines (e.g. `QuestionService` -> `SearchService` -> `PrefixTrie`).
3. **Presentation Layer (`include/app/`, `src/app/`)**: Interacts exclusively through domain services, never manipulating raw data structure nodes directly.
