# CodeVault — Practical Demonstration Guide

> **Target Audience**: Technical evaluators, interviewers, hiring managers, and academic viva panels.  
> **Duration**: 5–10 Minutes  
> **Focus**: Demonstrating authentic Data Structures & Algorithms (DSA), clean architecture, and practical coding practice workflows.

---

## 1. Introduction (20–30 Seconds)

> *"CodeVault is a high-performance, structured coding question management and practice platform designed around foundational data structures. It gives students and software engineers a centralized workspace to organize coding problems from multiple platforms, search them with sub-millisecond autocomplete, sequence focused practice drills, automate spaced repetition revisions, and track progress—all backed by custom, transparent DSA implementations."*

**Key Distinction**: CodeVault is a deterministic, algorithmically sound platform. It does not use opaque AI/LLM components or cloud microservices; every decision (search matching, revision intervals, practice recommendations) is driven by inspectable C++ algorithms and data structures.

---

## 2. Problem Statement (30 Seconds)

Aspiring engineers face three major hurdles during interview prep:
1. **Catalog Fragmentation**: Problems solved across LeetCode, Codeforces, HackerRank, and GeeksforGeeks are scattered across bookmarks, browser histories, and ad-hoc spreadsheets.
2. **Revision Deficit**: Retention of algorithmic pattern recognition drops rapidly without structured spaced intervals.
3. **Black-Box Tools**: Traditional tools hide the very data structures learners are trying to study. CodeVault turns its own architecture into a transparent, living demonstration of DSA in action.

---

## 3. Recommended 5–10 Minute Demonstration Flow

### Step 1: Authentication & Local Session (1 Minute)
1. **Open the Web UI** at `http://localhost:5173`.
2. **Explain**: CodeVault uses memory-hard **Argon2id** password hashing and stores Blake2b-256 session token digests in an embedded SQLite database (Schema v3).
3. **Show**: Log in or register an account. Note that successful login lands directly on the **Dashboard** (`/dashboard`), serving as the user's primary command cockpit. Note that session authentication is maintained via secure `HttpOnly` cookies with zero token leakage into client-side JavaScript.
4. **Demonstrate User Account Menu & Theme Switching**:
   - In the top-right header, click the user account chip. Note the accessible dropdown displaying user profile details, direct links to Profile and Settings, the integrated Light / Dark / System theme switcher, and Logout.
   - Switch between Light, Dark, and System themes to demonstrate full-app styling adaptation across cards, tables, navigation, and dialogs. Show that the preference is persisted in `localStorage` across page reloads.

### Step 2: Problem Catalog, Prefix Search & Sorting (2 Minutes)
1. **Navigate to the Questions Catalog** (`/questions`).
2. **Demonstrate Prefix Search**:
   - In the search bar, type `"Two"`.
   - **Explain**: The search input triggers a custom **Prefix Trie** (`PrefixTrie`), traversing node branches in $O(L)$ time (where $L$ is query length) to provide instantaneous autocomplete suggestions.
3. **Demonstrate Multi-Parameter Sorting**:
   - Sort questions by Difficulty (Hard $\to$ Easy) or Last Practiced date.
   - **Explain**: Sorting utilizes custom **Merge Sort** (stable, guaranteed $O(N \log N)$) and in-place **Quick Sort** with median-of-three pivot selection.
4. **Demonstrate Multi-Criteria Filtering**: Filter by algorithmic topic (e.g. `Arrays`, `DynamicProgramming`) or status (`Solved`, `Unsolved`).

### Step 3: Practice Next & Deterministic Recommendation (1.5 Minutes)
1. **Click "Practice Next Problem"** on the catalog header or Dashboard card.
2. **Explain the 5-Tier Deterministic Cascade**:
   - Point to the **Recommendation Reason** card (e.g., *"Prioritizing active practice queue head"* or *"Overdue Leitner revision with urgent priority"*).
   - **Explain**: Recommendations are 100% deterministic—evaluating active queue head $\to$ overdue Leitner revisions $\to$ never-practiced unsolved problems $\to$ in-progress problems $\to$ stale solved problems.

### Step 4: Daily Practice Queue & Verdict Submission (2 Minutes)
1. **Open the Practice Queue Drawer** (`PracticeQueueDrawer` on the right side).
2. **Explain the Queue DSA**:
   - Problems queued for study form a strict FIFO sequence managed by a custom linked **Queue** (`Queue<T>`).
3. **Demonstrate Verdict Submission**:
   - Open a problem detail view. Click **"Mark Solved"**.
   - Show how the Leitner interval dynamically advances (e.g. Box 1: +1 day $\to$ Box 2: +3 days).
   - Point out that upon verdict recording, the problem automatically unlinks from the active FIFO practice queue.
   - Click **"Needs Review"** on another problem to demonstrate interval reset to 1 day with priority escalation to urgent.
4. **Quick Navigation**: Press keyboard shortcut `N` to immediately jump to the next recommended problem.

### Step 5: Spaced Revision Workspace & Priority Queue (1 Minute)
1. **Navigate to Revision** (`/revision`).
2. **Explain the Priority Queue DSA**:
   - The Leitner revision schedule is powered by a custom **Min-Heap** (`MinHeap<T>`).
   - The heap prioritizes questions with the earliest `next_revision_at` in $O(\log N)$ time, applying deterministic tie-breaking on priority levels and IDs.
3. **Show**: The 5 Leitner level boxes (1, 3, 7, 14, 30 days) and the "Due Now" section.

### Step 6: Dashboard Analytics & Zero-Mutation Integrity (1 Minute)
1. **Navigate to Dashboard** (`/dashboard`).
2. **Explain**:
   - Aggregation metrics (Curriculum Completion %, 14-topic coverage distribution, difficulty breakdown) are computed deterministically by `StatisticsService` without mutating question records.

### Step 7: Data Portability & Tenant Isolation (1 Minute)
1. **Navigate to Settings $\to$ Data Export** (`/settings`).
2. **Demonstrate Export**:
   - Click **"Export JSON"**, **"Export CSV"**, **"Export Markdown"**, or **"Export Anki TSV"**.
   - Show how CodeVault provides zero vendor lock-in.
3. **Explain Multi-User Tenant Isolation**:
   - Mention that if another user logs in, they cannot inspect, edit, delete, or practice problems belonging to the current user. Foreign `owner_id` fields in imported files are stripped and bound to the authenticated caller.

---

## 4. How DSA is Actually Used (Viva & Technical Talking Points)

| Data Structure / Algorithm | Source Implementation | Concrete Product Problem Solved | Computational Complexity | Viva Explanation |
| :--- | :--- | :--- | :--- | :--- |
| **Prefix Trie** | `include/dsa/trie.hpp`<br>`src/dsa/trie.cpp` | Instant search autocomplete across problem titles and tag keywords. | Lookup / Insert: $O(L)$<br>($L = \text{string length}$) | *"Rather than scanning every problem linearly with regex ($O(N \cdot L)$), we insert titles into a character-level Trie. When a user types a prefix, we traverse down the prefix path and collect matching subtree leaves in time proportional to key length, independent of total catalog size."* |
| **Min-Heap Priority Queue** | `include/dsa/min_heap.hpp` | Spaced repetition scheduler that surfaces overdue revision problems. | Insert: $O(\log N)$<br>Extract-Min: $O(\log N)$ | *"We manage revision due dates using a binary Min-Heap. Finding the problem that is most urgently due takes $O(1)$ time at the root, and extracting it or updating its interval takes $O(\log N)$ heapify operations, ensuring fast scheduling even with thousands of problems."* |
| **FIFO Queue** | `include/dsa/queue.hpp` | Daily practice drill sequencing where problems are solved in order. | Enqueue: $O(1)$<br>Dequeue: $O(1)$<br>Unlink: $O(N)$ | *"A linked FIFO Queue maintains the problem sequence for daily study blocks. When a user marks a problem as solved, our custom node-splicing implementation removes the problem from anywhere in the queue without invalidating the rest of the queue sequence."* |
| **LIFO Stack** | `include/dsa/stack.hpp` | Inspection history tracking recently viewed questions. | Push: $O(1)$<br>Pop: $O(1)$ | *"A custom linked Stack records the user's recent problem navigation trail. Opening problems pushes them to the top; backtracking pops the most recently inspected item in strict LIFO order."* |
| **Doubly Linked List** | `include/dsa/doubly_linked_list.hpp` | Interactive problem playlists with bidirectional step navigation. | Push/Pop: $O(1)$<br>Reorder: $O(1)$ node splice | *"Unlike contiguous vectors where inserting or removing elements in the middle requires shifting elements in $O(N)$, our Doubly Linked List allows $O(1)$ pointer reassignment when rearranging playlist problems."* |
| **Hash Map** | `std::unordered_map` | Instant lookups by primary key, category index by topic, and company tags. | Average Lookup: $O(1)$<br>Worst-case: $O(N)$ | *"We use hash maps for multi-index secondary lookups. Looking up a question by ID or retrieving all questions tagged under 'Google' or 'DynamicProgramming' executes in average $O(1)$ time."* |
| **Merge Sort** | `include/dsa/sorting.hpp` | Stable sorting of problem tables across multiple combined criteria. | Time: $O(N \log N)$<br>Space: $O(N)$ | *"Merge Sort is chosen when table stability matters—for instance, sorting problems by Difficulty while preserving their existing secondary ordering by Practice Date. Its worst-case time complexity is strictly $O(N \log N)$."* |
| **Quick Sort** | `include/dsa/sorting.hpp` | In-place, low-overhead sorting for large memory-constrained catalogs. | Average: $O(N \log N)$<br>Worst: $O(N^2)$<br>Space: $O(\log N)$ | *"We use an in-place Quick Sort with median-of-three pivot selection and tail-call optimization to prevent stack overflow and avoid additional buffer allocations during catalog reordering."* |

---

## 5. Architectural Q&A for Technical Evaluators

**Q: Why C++17 for the backend instead of Node.js or Python?**  
*A: C++ provides deterministic memory management, zero-overhead abstraction, and allows us to implement custom data structures from raw pointers without hidden runtime garbage collection pauses. It delivers sub-millisecond API response times on consumer hardware.*

**Q: Why embedded SQLite rather than PostgreSQL or MySQL?**  
*A: CodeVault is designed as a desktop-first, privacy-focused utility. SQLite requires zero configuration, embeds directly inside the C++ binary via the official amalgamation, supports full ACID transactions and WAL mode, and stores all user data in a single portable file (`data/codevault.db`).*

**Q: Why is the Practice Queue kept in memory rather than persisted in the database?**  
*A: Per design decision ADR-023, the daily practice queue represents transient session state for quick FIFO drilling. Separating transient session sequencing from durable question revision intervals keeps database writes minimal and guarantees sub-millisecond queue manipulations. Permanent question statuses and Leitner intervals are always durable in SQLite.*
