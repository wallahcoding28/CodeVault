# CodeVault — Project Context & Engineering Blueprint

> **Notice for AI Coding Agents & Engineers**:  
> This document is the source of truth for CodeVault's context, architecture, principles, constraints, and evolutionary path. Read this document thoroughly before proposing or writing code.

---

## 1. What is CodeVault?

**CodeVault** is a structured, desktop-first coding practice and question management platform designed to help students and software engineers systematically organize, search, practice, track, and revise Data Structures & Algorithms (DSA) problems.

Rather than relying on scattered browser bookmarks, unstructured spreadsheets, or ephemeral note-taking apps, CodeVault provides a centralized workspace with first-class support for:
- Question organization (topic, difficulty, target company, platform, status).
- Sub-millisecond prefix search (via Trie).
- Algorithmic spaced revision scheduling (via Min-Heap Priority Queue).
- Practice queues (via FIFO Queue).
- Navigation and inspection history (via LIFO Stack).
- Multi-field indexing and instant lookups (via Hash Maps).
- Analytical statistics and progress tracking.

---

## 2. Why Does It Exist?

### The Problem
Students preparing for campus placements, competitive programming, and technical interviews face a common set of friction points:
1. **Disorganization**: Questions are solved across multiple platforms (LeetCode, Codeforces, HackerRank, GeeksforGeeks, CodeStudio) with no unified catalog.
2. **Revision Deficit**: Spaced repetition is critical for retaining pattern recognition in algorithmic problem solving, yet manual revision tracking is tedious and prone to abandonment.
3. **Black-box Tooling**: Most existing tracker tools are opaque web apps or basic spreadsheets that disconnect the learner from the very data structures they are studying.

### The CodeVault Solution
CodeVault bridges academic coursework and real-world productivity:
- It serves as a practical, polished desktop utility for interview prep.
- Its internal engine relies on transparent, rigorously implemented data structures (Trie, Heap, Hash Map, Queue, Stack, Linked List), turning the application itself into a living demonstration of DSA concepts in action.

---

## 3. Product Philosophy & Non-Negotiable Principles

1. **Production-Quality Foundations from Day 1**:
   Even though CodeVault originates in a 2nd-year B.Tech DSA Lab project, it is **not** a throwaway academic exercise. The codebase must follow clean modern C++ conventions, strict separation of concerns, comprehensive documentation, and deterministic testing.

2. **Genuine, Pedagogical DSA Integration**:
   Every data structure incorporated into CodeVault must have an authentic, defensible responsibility. Data structures must **never** be injected as decorative checkboxes.

3. **Appropriate Simplicity (Anti-Overengineering)**:
   Avoid premature abstraction, unnecessary design patterns, microservices, complex meta-programming, or external frameworks when standard, readable modern C++17 suffices.

4. **Zero Fake Functionality**:
   Stubbed features or mock responses must never be masqueraded as operational logic. If a feature is scheduled for a future stage, document it in `TODO.md` and keep the current interface cleanly scoped.

5. **Strict Stage Gating**:
   Development progresses through explicit, controlled stages (from Stage 1 Foundation up to Stage 9 Product Expansion). Never begin implementing features belonging to subsequent stages without explicit sign-off.

---

## 4. Technology Decisions

| Dimension | Selection | Rationale |
| :--- | :--- | :--- |
| **Language** | **C++17** | Provides strong type safety, deterministic performance, RAII memory management, standard STL containers, and clear pedagogical visibility into memory and pointers. Understandable to a 2nd-year CS student. |
| **Build System** | **CMake (>= 3.16)** | Industry standard cross-platform build tool; simplifies dependency management, out-of-source builds, and test runner integration. |
| **Testing** | **Custom Lightweight Test Harness / CTest** | Keeps the initial footprint dependency-free, fast, and easy to run on any machine without requiring external package installations (294 automated tests: 289 unit tests + 5 smoke checks). |
| **Persistence** | **SQLite (Schema v3 with WAL) & Local Files** | Zero-daemon, ACID-compliant local database storing questions, users, credentials, and sessions, complemented by CSV/JSON flat file support and full import/export portability (`IQuestionRepository`). |
| **User Interface** | **Dual Interface: CLI Shell & React 18 Web UI** | Interactive terminal CLI for low-overhead local usage, plus a modern React 18 + Vite frontend communicating via an embedded C++ HTTP REST API gateway. |

---

## 5. Prohibited Technologies & Anti-Patterns (At Current Stage)

The following items are **strictly prohibited** at this stage of development:
- ❌ **No AI / LLMs**: No OpenAI API, Gemini API, LangChain, or neural components.
- ❌ **No Cloud Backends / BaaS**: No Firebase, Supabase, AWS, Azure, GCP dependencies.
- ❌ **No Heavy External Database Daemons**: No PostgreSQL, MongoDB, MySQL server daemons or ORMs (embedded zero-daemon SQLite is utilized).
- ❌ **No Payment Gateways**: No Stripe, Razorpay, or billing logic.
- ❌ **No Social / Multiplayer**: No feeds, friends lists, leaderboards, or external networking sockets.
- ❌ **No Heavy GUI Runtimes in Core C++**: Clean separation between native C++ engine and presentation layer.
- ❌ **No Global Mutable State**: No static global singletons or unbounded global registries.

---

## 6. Architecture Overview & Layer Boundaries

CodeVault enforces a strict five-tier modular architecture:

```
┌────────────────────────────────────────────────────────┐
│                   Presentation Layer                   │
│   (CLI Menu Shell, Text Formatter, Input Validator)    │
└───────────────────────────┬────────────────────────────┘
                            │ Calls Services
┌───────────────────────────▼────────────────────────────┐
│                    Application Layer                   │
│   (QuestionService, SearchService, RevisionService,    │
│    PracticeService, StatisticsService)                 │
└─────────────┬────────────────────────────┬─────────────┘
              │ Coordinates                │ Queries
┌─────────────▼──────────────┐ ┌───────────▼─────────────┐
│          Core / DSA        │ │       Domain Models     │
│  (Trie, Min-Heap, HashMap, │ │  (Question, Topic,      │
│   Stack, Queue, LinkedList)│ │   Difficulty, Status)   │
└─────────────┬──────────────┘ └───────────▲─────────────┘
              │ Interacts                  │ Maps
┌─────────────▼────────────────────────────┴─────────────┐
│                    Persistence Layer                   │
│   (IQuestionRepository, FileRepository, Serializer)    │
└────────────────────────────────────────────────────────┘
```

### Boundary Invariants
1. **The DSA Core is UI-Agnostic**: Data structures accept and return domain models or identifiers. They do not know about terminal colours, menus, stdout, or JSON formatting.
2. **Services Orchestrate, Core Computes**: `QuestionService` coordinates between storage and the indexing data structures; it does not implement custom pointer manipulation itself.
3. **Repository Abstraction (`IQuestionRepository`)**: Application code interacts with the repository contract, ensuring a painless future transition from local CSV/JSON to SQLite or cloud persistence.

---

## 7. Educational DSA Requirements & Mapping

Every data structure has a genuine product responsibility:

| Data Structure | Implementation Type | Product Role | Responsibility |
| :--- | :--- | :--- | :--- |
| **Vector / Array** | `std::vector` | Ordered Question Catalog | Contiguous storage for fast indexing and linear traversals. |
| **Hash Map** | `std::unordered_map` | Instant Lookups | $O(1)$ lookup by `id`, category index by `topic` and `company`. |
| **Trie** | Custom `PrefixTrie` | Auto-complete & Search | $O(L)$ prefix matching on question titles and tag keywords (Stage 4). |
| **Priority Queue** | Custom `MinHeap<T>` | Spaced Revision Scheduler | Extracts question with earliest `next_revision_at` in $O(\log N)$ via `RevisionService` with 3-key tie-breaking (Stage 5). |
| **Queue** | Custom Linked `Queue<T>` | Daily Practice Session | Strict FIFO processing of problems queued for study sessions via `PracticeService` with skip cycling (Stage 5). |
| **Stack** | Custom Linked `Stack<T>` | Navigation History | Strict LIFO tracking of recently viewed questions. |
| **Linked List** | Custom `DoublyLinkedList<T>` | Active Session Navigation | Sequential iteration through active question lists with $O(1)$ reordering. |
| **Sorting** | Custom Algorithms | Flexible Multi-key Ordering | Custom `mergeSort` (stable, $O(N \log N)$) and `quickSort` (in-place) implemented in `include/dsa/sorting.hpp` (Stage 4). |
| **Aggregation / Stats** | `StatisticsService` | Deterministic Dashboard Analytics | Zero-mutation aggregation across questions and revision heap for difficulty, topic, status, revision, and practice breakdowns (Stage 6). |

---

## 8. Current Stage Assumptions & Target User

- [x] **User Model & Authentication**: Authenticated user accounts with Argon2id password hashing and session tokens in SQLite schema v3 (Stage 9.3/9.4), while CLI supports local single-user mode (`local_user`).
- [x] **Data Isolation & Owner Scoping**: Strictly scoped per-user catalog isolation across all queries, DSA indexes, and revision schedules. User A cannot inspect, modify, or delete questions belonging to User B.
- [x] **Data Portability**: Full bidirectional import/export (JSON, RFC 4180 CSV, Markdown, Anki TSV) with atomic rollback and conflict resolution (Stage 9.5).
- [x] **Practice & Queue Engine**: Deterministic 5-tier "Practice Next" recommendation cascade, targeted FIFO drill sessions, queue inspection/removal, single-question practice verdicts, and Leitner 5-box SRS interval advancement (Stage 10).
- [x] **Stage 11 Production Readiness & Release Candidate**: Contract reconciliation, live HTTP E2E automation (49 assertions), frontend error handling hardening, zero compiler warnings, 289 unit tests + 5 smoke tests + 2 CTest suites (Stage 11).

---

## 9. Coding & Naming Conventions

- **Standard**: C++17.
- **File Names**: Snake case (e.g., `question_service.hpp`, `min_heap.hpp`, `file_repository.cpp`).
- **Class & Struct Names**: PascalCase (e.g., `Question`, `QuestionRepository`, `PrefixTrie`).
- **Function & Method Names**: camelCase (e.g., `getQuestionById`, `insertQuestion`, `scheduleRevision`).
- **Member Variables**: trailing underscore (e.g., `title_`, `difficulty_`, `repository_`).
- **Enums**: Strongly typed `enum class` in PascalCase (e.g., `Difficulty::Medium`, `Status::Solved`).
- **Pointers & Memory**: Explicit ownership semantics using `std::unique_ptr` and `std::shared_ptr`. Raw pointers used only for non-owning node traversal in educational data structures.
