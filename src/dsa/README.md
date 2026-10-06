# Core DSA Engine (Implementations)

This directory houses the non-templated source code and implementation details for CodeVault's custom Data Structures & Algorithms:

- Templated structures (such as `MinHeap<T>`, `PracticeQueue<T>`, `RecentStack<T>`, and `DoublyLinkedList<T>`) are primarily implemented as header-only constructs in `include/dsa/` to maximize compiler inlining and type flexibility.
- Non-templated structures and complex algorithmic utilities (such as `PrefixTrie` node storage and `SortingAlgorithms` helper routines) will have their `.cpp` implementation units placed here during Stage 3 and Stage 4.
