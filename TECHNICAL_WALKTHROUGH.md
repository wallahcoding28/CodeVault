# CodeVault — Technical Walkthrough & Viva Guide

> **Purpose**: Concise technical explanations for major engineering decisions, architectural patterns, and algorithmic choices in CodeVault. Formatted for direct reference during technical interviews and college viva examinations.

---

## 1. System Architecture Decisions

### Why a C++17 Backend?
- **Deterministic Resource Management**: RAII (Resource Acquisition Is Initialization) guarantees that memory, file descriptors, and database connections are freed the instant they leave scope, without relying on unpredictable garbage collectors.
- **Authentic DSA Demonstrations**: Building custom data structures (Tries, Heaps, Linked Lists) requires direct pointer manipulation, template generic programming, and Rule-of-5 copy/move semantics that are best expressed in modern C++.
- **Low Latency**: Sub-millisecond local execution guarantees that search queries, heap scheduling, and table sorting respond instantly on commodity hardware.

### Why CMake & Ninja?
- **Portability**: CMake provides cross-platform build definitions across Windows (MinGW/MSVC), Linux (GCC/Clang), and macOS.
- **Fast Incremental Builds**: Ninja executes dependency graphs in parallel, reducing compilation and test iteration cycles.

### Why Embedded SQLite 3?
- **Zero-Config Portability**: Embeds directly into the binary with zero daemon processes, external server ports, or database administration overhead.
- **ACID Integrity**: Enforces atomicity, transactions, foreign keys, and Write-Ahead Logging (WAL) for concurrency.
- **Single-File Portability**: All user data, credentials, and revision histories reside cleanly in `data/codevault.db`.

### Why React 18 + TypeScript for the Frontend?
- **Type Safety**: TypeScript interfaces (`Question`, `User`, `PracticeNextResult`) mirror C++ domain structs, preventing contract drift between frontend and backend.
- **Component Modularity**: Allows isolating complex interactive components (such as `PracticeQueueDrawer` and `StartSessionModal`) with self-contained state.
- **Native Coding-Platform UX**: Tailwind CSS enables a clean, restrained dark theme familiar to competitive programmers without unnecessary styling bloat.

### Why Separate Service and Domain Layers (Clean Architecture)?
- Decouples business logic from persistence and presentation.
- Domain models (`Question`, `Topic`, `Difficulty`) are plain data carriers with validation methods.
- Services (`QuestionService`, `RevisionService`, `SearchService`, `PracticeService`) orchestrate business rules and synchronize in-memory DSA indexes with SQLite without coupling to HTTP or CLI code.

### Why Keep DSA Implementations Independent of the UI?
- High cohesion and low coupling: Custom data structures in `include/dsa/` are header-only, templated, and completely agnostic to terminal escape codes, JSON formatting, or HTTP request structures.
- Testability: Data structures are unit-tested in isolation (e.g. Test 176–180) for memory safety, underflow handling, and boundary edge cases.

---

## 2. Authentication & Security Decisions

### How Do Sessions Work?
- Upon successful login or registration, the backend generates a cryptographically secure 256-bit random session token using system CSPRNG (CryptoAPI on Windows).
- The raw token is returned to the client exclusively via an `HttpOnly`, `SameSite=Lax`, environment-aware `Secure` cookie named `codevault_session`.
- The client browser automatically includes this cookie in subsequent requests without JavaScript access, protecting against Cross-Site Scripting (XSS) token theft.

### Why Is Argon2id Used for Passwords?
- Argon2id is the state-of-the-art password hashing standard (winner of the Password Hashing Competition).
- It provides hybrid resistance against both GPU-based side-channel attacks and ASIC brute-forcing by requiring configurable memory allocation ($m = 19456\text{ KiB}$) and iterations ($t = 2$).
- Each password uses a unique cryptographically random 16-byte salt; plaintexts and hashes are never exposed.

### Why Are Session Tokens Not Stored Directly in the Database?
- If a database file is compromised or leaked, raw session tokens would allow session hijacking.
- CodeVault hashes the session token using **Blake2b-256** and stores only the `token_hash` in SQLite. When a request arrives, the server hashes the incoming token and compares digests in constant time.

### How Does Multi-User Owner Isolation Work?
- Every question record possesses an `owner_id` column with a foreign key referencing `users.id`.
- Every protected API route retrieves the authenticated caller's identity via `CurrentUserProvider`.
- All service queries (`findById`, `findAll`, `save`, `remove`) enforce owner boundaries. If User B attempts to access `GET /api/questions/Q-1001` owned by User A, the repository query returns empty, resulting in a clean 404 Not Found.

---

## 3. Data Structures & Algorithms (DSA) Rationale

### Why Prefix Trie?
- A standard database `LIKE '%prefix%'` or substring scan requires scanning $N$ strings.
- A Trie matches prefixes in $O(L)$ time, where $L$ is query length, regardless of catalog size ($N$). Subtree traversal provides instant autocomplete suggestions.

### Why Min-Heap for Spaced Revision?
- Spaced repetition requires querying the question with the earliest due date (`next_revision_at`).
- An unsorted list takes $O(N)$ to find the minimum; a sorted array takes $O(N)$ to insert.
- A binary Min-Heap provides $O(1)$ lookup for the most urgent item and $O(\log N)$ insert and extraction, ideal for dynamic schedules.

### Why FIFO Queue for Practice?
- Daily practice blocks require problems to be addressed in the order they were scheduled or selected.
- A custom linked `Queue<T>` provides $O(1)$ enqueue and dequeue operations without array element shifting. Our custom implementation includes targeted node unlinking when problems are solved out of order.

### Why LIFO Stack for History?
- Browsing problems mirrors browser history: opening problem $A \to B \to C$ pushes to the top; backtracking returns to $B$ then $A$. A linked Stack executes this in $O(1)$ operations with zero wasted capacity.

### Why Doubly Linked List for Playlists?
- In an interactive playlist, moving a problem forward, backward, or deleting an item from the middle takes $O(1)$ pointer adjustments once the node is located, avoiding vector reallocation and element copying.

### Why Custom Sorting Algorithms (Merge Sort & Quick Sort)?
- **Merge Sort**: Stable $O(N \log N)$ sorting essential when sorting by primary criteria (e.g. Difficulty) while preserving existing secondary order (e.g. Date Added).
- **Quick Sort**: In-place $O(N \log N)$ average-case sorting with median-of-three pivot selection, eliminating extra memory buffers when sorting large catalogs.

---

## 4. Practice & Spaced Revision Mechanics

### How Does the "Practice Next" Cascade Work?
The recommendation engine evaluates candidate questions across 5 deterministic priority tiers:
1. **Active Practice Queue Head**: If the user has manually queued problems, the head of the FIFO queue is served first.
2. **Overdue Leitner Revisions**: Problems where `next_revision_at` is past the current timestamp, sorted by urgency priority.
3. **Never-Practiced Unsolved Problems**: Fresh questions from the catalog to introduce new material.
4. **In-Progress / Attempted Problems**: Previously attempted questions needing completion.
5. **Stale Solved Problems**: Questions solved long ago that need retention reinforcement.

### Why Is the Recommendation Cascade Deterministic?
- Zero LLM dependencies eliminates hallucinations, non-deterministic recommendations, latency spikes, and internet connectivity requirements.
- Predictable and explainable: The API returns a human-readable `recommendationReason` string stating the exact rule triggered.

### How Does the Leitner Spaced Revision Schedule Work?
- Implements a 5-box Leitner model with intervals:
  - **Level 1**: +1 Day
  - **Level 2**: +3 Days
  - **Level 3**: +7 Days
  - **Level 4**: +14 Days
  - **Level 5**: +30 Days
- **On "Solved" Verdict**: Question advances to the next level (e.g., Level 1 $\to$ Level 2), increasing the interval, and status updates to `Solved`.
- **On "Needs Review" Verdict**: Interval resets immediately to Level 1 (+1 day), priority escalates to urgent (1), and status updates to `InProgress`.
- In both cases, the question is unlinked from the active practice queue upon verdict recording.

---

## 5. Import / Export Data Portability

- **Structured JSON**: Full metadata backup; batch import uses an atomic SQLite transaction (`BEGIN IMMEDIATE TRANSACTION ... COMMIT`) that automatically rolls back on validation errors.
- **RFC 4180 CSV**: Custom finite-state parser safely handles quoted commas, multiline descriptions, and escaped quotes (`""`).
- **Markdown Export**: Human-readable catalog grouped by topic and difficulty for notes or GitHub wikis.
- **Anki TSV Deck**: 3-column tab-separated output (`Title`, `Topic / Difficulty / Tags`, `Description & Notes`) formatted with `<br>` tags for direct import into Anki flashcards.
- **Conflict Strategies**: User chooses how existing IDs are handled: `skip` (preserves existing), `overwrite` (updates matched), or `generate_new_id` (imports as fresh records).

---

## 6. Testing & Quality Assurance Summary

When explaining the test suite during evaluation, cite the exact numbers:
- **289 Unit & Integration Tests** (`build/bin/unit_tests.exe`): Covering domain models, custom DSA, SQLite repositories, Argon2id auth, practice cascades, and API contracts.
- **5 Automated Smoke Tests** (`build/bin/smoke_test.exe`): Validating CSV ingestion, CLI menus, and core service queries.
- **2 Unified CTest Suites** (`ctest --test-dir build`): Automated cross-target test runner.
- **49 Live HTTP Runtime E2E Assertions** (`test_live_e2e.ps1`): End-to-end integration testing against a live server process, verifying registration, logins, cookie sessions, tenant isolation, import/export, and server-restart persistence.
- **Frontend Production Build**: Clean Vite + TypeScript compilation (1588 modules, 0 errors).
- **Manual QA Protocol**: Comprehensive 8-stage manual browser verification checklist documented in `MANUAL_QA_CHECKLIST.md`.
