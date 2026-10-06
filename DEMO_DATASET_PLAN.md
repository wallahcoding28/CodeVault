# CodeVault — Demonstration Dataset Plan

> **Purpose**: Recommended representative problem dataset for live demonstrations, evaluator walkthroughs, and portfolio screenshots.  
> **Rule**: Uses publicly known algorithmic problem metadata without fabricating company statistics or private interview details.

---

## 1. Existing Sample Catalog (`data/sample/questions.csv`)

CodeVault ships with a pristine, verified 9-question sample dataset designed to demonstrate core functionality immediately upon repository cloning:

| ID | Title | Topic | Difficulty | Platform | Status | Demonstrates |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `Q-1001` | Two Sum | Arrays | Easy | LeetCode | Solved | Hash Map pattern, prefix search ("Two") |
| `Q-1002` | Add Two Numbers | LinkedLists | Medium | LeetCode | Solved | Linked List traversal, prefix search |
| `Q-1003` | Longest Substring Without Repeating | Strings | Medium | LeetCode | InProgress | Sliding window pattern, due revision |
| `Q-1004` | Median of Two Sorted Arrays | Arrays | Hard | LeetCode | Unsolved | Binary search, Hard difficulty filter |
| `Q-1005` | Longest Palindromic Substring | Strings | Medium | LeetCode | Solved | Dynamic programming, string algorithms |
| `Q-1006` | Reverse Integer | Math | Medium | LeetCode | Unsolved | Integer overflow, edge cases |
| `Q-1007` | String to Integer (atoi) | Strings | Medium | LeetCode | InProgress | State machine, string parsing |
| `Q-1008` | Regular Expression Matching | DynamicProgramming | Hard | LeetCode | Unsolved | 2D dynamic programming |
| `Q-1009` | Container With Most Water | TwoPointers | Medium | LeetCode | Solved | Two pointer optimization |

---

## 2. Extended Demonstration Dataset (Recommended for Live Demos)

For full demonstrations highlighting all 14 topic cards and the 5-tier recommendation cascade, the presenter may optionally add these 6 standard synthetic records:

```json
[
  {
    "id": "Q-1010",
    "title": "Merge Intervals",
    "topic": "Arrays",
    "difficulty": "Medium",
    "platform": "LeetCode",
    "status": "InProgress",
    "url": "https://leetcode.com/problems/merge-intervals/",
    "notes": "Sort by start time, merge overlapping bounds.",
    "tags": ["arrays", "sorting", "intervals"]
  },
  {
    "id": "Q-1011",
    "title": "Invert Binary Tree",
    "topic": "Trees",
    "difficulty": "Easy",
    "platform": "LeetCode",
    "status": "Solved",
    "url": "https://leetcode.com/problems/invert-binary-tree/",
    "notes": "Recursively swap left and right subtrees.",
    "tags": ["trees", "binary-tree", "recursion"]
  },
  {
    "id": "Q-1012",
    "title": "Number of Islands",
    "topic": "Graphs",
    "difficulty": "Medium",
    "platform": "LeetCode",
    "status": "Unsolved",
    "url": "https://leetcode.com/problems/number-of-islands/",
    "notes": "DFS/BFS flood fill on 2D grid.",
    "tags": ["graphs", "dfs", "bfs"]
  },
  {
    "id": "Q-1013",
    "title": "Climbing Stairs",
    "topic": "DynamicProgramming",
    "difficulty": "Easy",
    "platform": "LeetCode",
    "status": "Solved",
    "url": "https://leetcode.com/problems/climbing-stairs/",
    "notes": "Fibonacci transition: dp[i] = dp[i-1] + dp[i-2].",
    "tags": ["dp", "math"]
  },
  {
    "id": "Q-1014",
    "title": "Kth Largest Element in an Array",
    "topic": "Heaps",
    "difficulty": "Medium",
    "platform": "LeetCode",
    "status": "InProgress",
    "url": "https://leetcode.com/problems/kth-largest-element-in-an-array/",
    "notes": "Maintain min-heap of size K, or use QuickSelect.",
    "tags": ["heaps", "priority-queue", "quickselect"]
  },
  {
    "id": "Q-1015",
    "title": "Course Schedule",
    "topic": "Graphs",
    "difficulty": "Medium",
    "platform": "LeetCode",
    "status": "Unsolved",
    "url": "https://leetcode.com/problems/course-schedule/",
    "notes": "Cycle detection in directed graph using Kahn's algorithm (topological sort).",
    "tags": ["graphs", "topological-sort"]
  }
]
```

### Why This Extended Dataset Is Effective:
1. **Topic Coverage**: Expands coverage across Trees, Graphs, Heaps, and Dynamic Programming, showcasing the 14-topic dashboard distribution.
2. **Practice Next Cascade**: Provides a realistic mix of overdue revisions (`Merge Intervals`), in-progress problems, and unsolved problems across Easy, Medium, and Hard tiers.
3. **Prefix Trie Demo**: Allows searching `"Climb"`, `"Merge"`, `"Number"`, and `"Course"` to highlight prefix autocomplete.
