# Core DSA Engine (Header Declarations)

This directory contains the public header specifications for custom Data Structures & Algorithms implemented in CodeVault:

- `trie.hpp`: Prefix tree (`PrefixTrie`) for sub-millisecond autocomplete and question title prefix matching. (IMPLEMENTED)
- `min_heap.hpp`: Templated binary min-heap priority queue (`MinHeap<T, Compare>`) for spaced revision scheduling. (IMPLEMENTED)
- `queue.hpp`: Linked-node FIFO queue (`Queue<T>`) for daily study sprints and practice problem progression. (IMPLEMENTED)
- `stack.hpp`: Linked-node LIFO stack (`Stack<T>`) for tracking recently inspected problems and navigation history. (IMPLEMENTED)
- `doubly_linked_list.hpp`: Bidirectional list (`DoublyLinkedList<T>`) for problem playlist inspection and $O(1)$ reordering. (IMPLEMENTED)

All custom data structures in this directory adhere to strict encapsulation: they are UI-agnostic, persistence-agnostic, and model-agnostic.
