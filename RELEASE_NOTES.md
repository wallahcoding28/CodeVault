# CodeVault — Release Candidate Release Notes

> **Version**: Release Candidate (RC-1)  
> **Status**: Feature Freeze & Production Ready  
> **Target Platforms**: Windows, Linux, macOS  
> **Standard**: C++17 / Embedded SQLite Schema v3 / React 18 + TypeScript

---

## 1. Project Overview

CodeVault is a high-performance, structured coding question management and spaced practice platform engineered for competitive programmers and software engineering students. It unifies problem tracking across multiple judges (LeetCode, Codeforces, HackerRank, GeeksforGeeks) into a private, local-first workspace.

Unlike traditional trackers, CodeVault implements genuine, educational Data Structures & Algorithms (DSA) as the primary execution engine behind search autocomplete, spaced revision scheduling, and practice workflows.

---

## 2. Major Capabilities

### 2.1 Core Question Management
- Full lifecycle CRUD for coding problems with title, difficulty (`Easy`, `Medium`, `Hard`), status (`Unsolved`, `InProgress`, `Solved`, `Mastered`), primary algorithmic topic, target companies, platforms, URLs, notes, and favorite flags.
- Comprehensive field validation (e.g. 255-character title bounds, sanitized URLs, and valid enum mapping).
- Sequential, deterministic ID generation (`Q-1001`, `Q-1002`).

### 2.2 Transparent DSA Engine
Every data structure in CodeVault possesses a genuine, defensible product responsibility:
- **Custom Prefix Trie (`PrefixTrie`)**: Case-insensitive prefix search and autocomplete with leaf-pruning memory management.
- **Custom Min-Heap (`MinHeap<T>`)**: Binary priority queue powering Leitner spaced repetition with 3-key tie-breaking (`next_revision_at`, `priority`, `id`).
- **Custom FIFO Queue (`Queue<T>`)**: Singly linked queue for practice session problem progression with targeted node unlinking.
- **Custom LIFO Stack (`Stack<T>`)**: Singly linked stack tracking recent problem inspection history.
- **Custom Doubly Linked List (`DoublyLinkedList<T>`)**: Bidirectional problem playlist walkthroughs with $O(1)$ reordering.
- **Custom Merge Sort (`mergeSort`)**: Stable $O(N \log N)$ sorting with arbitrary comparators.
- **Custom Quick Sort (`quickSort`)**: In-place sorting with median-of-three pivot selection and tail-call optimization.

### 2.3 Authentication & Session Management
- **Argon2id Hashing**: Memory-hard password hashing ($t=2, m=19456, p=1$) with 16-byte cryptographically secure random salts. Plaintext passwords or hashes are never exposed.
- **Server-Side Token Digests**: Cryptographically random 256-bit session tokens stored exclusively as Blake2b-256 digests (`token_hash`).
- **Secure Cookie Transport**: `HttpOnly`, `SameSite=Lax`, environment-aware `Secure` cookies with 7-day expiration and server-side revocation on logout.
- **Multi-Device Session Control**: List active devices without token leakage, revoke targeted sessions, or revoke all other sessions.
- **Backward Compatibility**: Seeded `local_user` ensures local CLI workflows (`codevault --cli`) operate without authentication barriers.

### 2.4 Owner Isolation & Tenant Boundaries
- Strict multi-user tenant isolation across all application layers.
- Users can never read, update, delete, practice, or export problems owned by another user.
- Imported data with foreign or spoofed `owner_id` fields are sanitized and bound to the authenticated user.

### 2.5 Deterministic Practice Next & Spaced Repetition (SRS)
- **Deterministic 5-Tier Recommendation Cascade**:
  1. Active FIFO Practice Queue Head
  2. Overdue Leitner Revisions (highest priority first)
  3. Never-Practiced Unsolved Problems
  4. In-Progress / Attempted Problems
  5. Stale Solved/Mastered Retention Reinforcement
- **Explainability String**: Every recommendation includes a human-readable explanation of why the problem was selected (zero AI/LLM hallucination risk).
- **Leitner 5-Box SRS Intervals**: Configurable spaced intervals (1, 3, 7, 14, 30 days) adjusting dynamically upon `Solved` or `NeedsReview` verdicts.

### 2.6 Catalog Portability & Ingestion Subsystem
- **Structured JSON Export/Import**: Full problem metadata export with atomic SQLite batch rollback on malformed input.
- **Conflict Strategies**: User-configurable import behavior (`skip`, `overwrite`, `generate_new_id`).
- **RFC 4180 CSV Export/Import**: State-machine CSV parser handling multiline fields, commas inside quotes, and escaped quotes.
- **Markdown Export**: Formatted catalog export categorized by algorithmic topic and difficulty.
- **Anki-Compatible TSV Deck**: 3-column TSV format ready for direct flashcard import.

### 2.7 Modern Desktop Web UI
- Responsive, coding-platform-native interface built with React 18, Vite, TypeScript, and Tailwind CSS.
- Slide-out `PracticeQueueDrawer`, `StartSessionModal`, interactive filter bars, keyboard navigation shortcut (`N`), and real-time status updates.
- Hardened error boundary states preventing infinite spinners on network or service disruptions.

---

## 3. Persistence & Architecture

- **Clean Architecture**: Layered separation between Presentation (CLI / HTTP), Application Services, Core DSA, Domain Entities, and Persistence.
- **Embedded SQLite Persistence (Schema v3)**: WAL mode, foreign keys, secondary indices, and automated idempotent schema migrations.
- **Transient Practice Queue (ADR-023)**: In-memory FIFO practice queue intentionally resets on process restart, while all durable question statuses, timestamps, and Leitner intervals remain safely persisted in SQLite.

---

## 4. Verification & Quality Assurance Baseline

CodeVault adheres to strict verification gating:

| Verification Suite | Result | Scope / Coverage |
| :--- | :--- | :--- |
| **Backend Unit Tests** | **289 / 289 PASSED (100%)** | Domain, DSA, Services, SQLite repositories, Auth, Practice API |
| **Automated Smoke Tests** | **5 / 5 PASSED (100%)** | CSV ingestion, CLI workflows, and core query pipeline |
| **Unified CTest Runner** | **2 / 2 PASSED (100%)** | Automated cross-target test runner |
| **Live HTTP E2E Suite** | **49 / 49 PASSED (100%)** | Registration, login, logout, 401s, tenant isolation, import/export, practice, persistence |
| **Frontend Production Build** | **PASSED (0 Errors)** | Vite + TypeScript compilation (147 modules compiled cleanly) |
| **Compiler / Linter** | **0 Warnings / 0 Errors** | GCC 16.1.0 (`-std=c++17 -Wall -Wextra -Wpedantic`) |

---

## 5. Known Limitations & Architectural Boundaries

1. **Transient Practice Queue Persistence**:
   Per design decision ADR-023, the active daily practice queue is held in memory for low-latency FIFO sequencing and resets on server restart. All permanent revision schedules, problem statuses, and timestamps are durable in SQLite.
2. **Automated Browser Runner**:
   Browser verification was executed via TypeScript type checking, production Vite builds, and live backend HTTP API test automation. An automated Playwright/Cypress browser runner is not configured.
3. **Out-of-Scope Roadmap Capabilities**:
   Third-party OAuth providers, email verification, cloud database clustering (PostgreSQL/Redis), and external Online Judge scraping connectors are deferred to future post-release stages.

---

## 6. Getting Started

### CLI Mode:
```bash
./build/bin/codevault.exe --cli
```

### Full Web Experience:
```bash
# Terminal 1: Start native HTTP backend
./build/bin/codevault.exe --server 8080

# Terminal 2: Start frontend dev server
cd frontend
npm run dev
```
Navigate to `http://localhost:5173` to register an account and start practicing.
