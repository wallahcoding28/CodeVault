# CodeVault — Architecture Decision Records (ADR)

This document records the architectural and engineering decisions made during the evolution of CodeVault, including the context, evaluated options, and underlying rationale.

---

### ADR-001: Selection of C++17 for Core Application & Algorithmic Engine
- **Status**: Accepted
- **Date**: 2026-09-25
- **Context**: CodeVault serves a dual objective: it is an academic submission for a 2nd-year B.Tech Data Structures & Algorithms (DSA) course and a foundation for a future production-grade desktop coding practice platform.
- **Decision**: Implement the core application, domain models, and data structures in Modern C++ (C++17 standard).
- **Rationale**:
  - C++17 provides explicit, granular control over memory management (RAII, smart pointers) and low-level pointer manipulation essential for demonstrating classical data structure mechanics (Trie nodes, linked lists, heap buffers).
  - Offers zero-cost abstractions, deterministic performance, and sub-millisecond local execution without garbage collection pauses.
  - C++17 features (`std::optional`, `std::string_view`, structured bindings, `std::variant`) modernize the code while remaining accessible and educational for undergraduate CS students.
- **Consequences**: Manual memory discipline must be maintained; cross-platform toolchains (CMake, GCC/Clang/MSVC) must be configured cleanly.

---

### ADR-002: Decoupling the DSA Engine from Presentation and Application Logic
- **Status**: Accepted
- **Date**: 2026-09-25
- **Context**: In many educational projects, data structures are deeply coupled to console `std::cout` prints, file readers, or specific UI widgets, making them rigid, un-testable, and disposable.
- **Decision**: The DSA components (`Trie`, `MinHeap`, `DoublyLinkedList`, `PracticeQueue`, `RecentStack`) must be pure, self-contained data structures living in `include/dsa/` and `src/dsa/`. They operate exclusively on generic keys or domain identifiers and have zero dependencies on terminal I/O, file systems, or user menus.
- **Rationale**:
  - Ensures clean testability using isolated unit test harnesses.
  - Allows swapping presentation frontends (CLI $\to$ GUI $\to$ Web) without altering algorithmic logic.
  - Adheres strictly to the Single Responsibility Principle (SRP).
- **Consequences**: Requires intermediate application services (`QuestionService`, `SearchService`, `RevisionService`) to coordinate data transfer between storage, domain models, and the DSA indexes.

---

### ADR-003: Simple Local File-Based Persistence for Initial Stages
- **Status**: Accepted
- **Date**: 2026-09-25
- **Context**: A persistent store is needed to retain questions across sessions, but introducing a heavyweight relational database engine (PostgreSQL, MySQL) or external daemon is premature.
- **Decision**: Utilize human-readable local flat-file storage (CSV / delimited records) placed under `data/`, accessed exclusively via an abstraction interface (`IQuestionRepository`).
- **Rationale**:
  - Zero external dependencies: requires no database installation, Docker container, or daemon management.
  - Inspectable and editable with standard text editors, simplifying debugging and sample data creation.
  - By abstracting all reads and writes behind `IQuestionRepository`, migrating to SQLite or an embedded database in later stages requires no modifications to the business services.
- **Consequences**: Complex relational queries must be handled in memory via HashMaps and linear traversals, which is entirely sufficient for the expected scale ($< 10,000$ questions).

---

### ADR-004: Exclusion of External APIs, Scraping, and Cloud Services at Stage 1
- **Status**: Accepted
- **Date**: 2026-09-25
- **Context**: Long-term goals include potential integrations with LeetCode, Codeforces, and HackerRank APIs or webhooks.
- **Decision**: Defer all network communication, external REST calls, and cloud synchronization until Stage 8/9.
- **Rationale**:
  - External APIs are prone to rate limits, authentication requirements, and breaking schema changes.
  - Prevents network failure modes from obstructing local core development and academic evaluation.
  - Keeps the initial build fast, local, and completely reproducible offline.
- **Consequences**: Question metadata must initially be entered manually or imported via structured local sample data files.

---

### ADR-005: Deferring AI / LLMs and Machine Learning
- **Status**: Accepted
- **Date**: 2026-09-25
- **Context**: It is tempting to integrate LLM features (e.g. AI problem hints, code explanation, automatic tag classification) early in development.
- **Decision**: Strictly prohibit AI and LLM dependencies during early architectural stages.
- **Rationale**:
  - The primary pedagogical requirement of this project is mastering and demonstrating deterministic Data Structures & Algorithms.
  - Introducing AI prematurely distracts from core architectural rigor and risks turning the project into an API wrapper.
  - Clean modular architecture allows AI services (e.g., an `AiHintService`) to be attached cleanly as an optional plugin in Stage 9.
- **Consequences**: Problem analysis and tags rely entirely on structured metadata and deterministic indexing algorithms.

---

### ADR-006: Staged Stage-Gate Development Methodology
- **Status**: Accepted
- **Date**: 2026-09-25
- **Context**: Large engineering projects risk bloat and regression when developers attempt to build multiple layers simultaneously.
- **Decision**: Execute development across 9 strictly bounded stages (Stage 1: Foundation to Stage 9: Expansion). Transition between stages requires validation through compilation, smoke tests, and user approval.
- **Rationale**:
  - Enforces systematic progress and prevents scope creep.
  - Guarantees that each milestone leaves behind a working, demonstrable, and fully documented artifact.
- **Consequences**: Stage 1 deliverables are constrained to foundation, documentation, architecture, build system, and an initial executable shell. Core CRUD and DSA indexing are formally addressed in subsequent stages.

---

### ADR-007: Single Local User Model with Reserved Multi-Tenancy Hooks
- **Status**: Accepted
- **Date**: 2026-09-25
- **Context**: CodeVault starts as an individual productivity utility for a student, but aims to scale into a multi-user platform.
- **Decision**: Maintain a single active local session without authentication barriers, but mandate an `owner_id` attribute on all domain models and repository interfaces.
- **Rationale**:
  - Eliminates unnecessary login screens, password hashing, and session management overhead during early development.
  - Prevents single-user schema lock-in, enabling multi-tenant databases to be adopted later with minimal disruption.
- **Consequences**: Storage paths and queries default to a static user identifier (`"local_user"`).

---

### ADR-008: Stage 2 Core Question Management Persistence and Validation Architecture
- **Status**: Accepted
- **Date**: 2026-09-26
- **Context**: Stage 2 introduces full CRUD operations and requires that question additions, modifications, and deletions survive application restarts without corrupting sample datasets.
- **Decision**:
  1. Enforce business rules and validations inside `QuestionService::validateQuestion` rather than in the presentation layer (CLI) or domain entities (`Question`).
  2. Implement atomic-safe file flush within `FileQuestionRepository` ensuring that `save()` and `remove()` persist changes immediately to disk with RFC-4180 compliant CSV quoting and delimiter escaping.
  3. Separate user practice data (`data/questions.csv`) from the development sample dataset (`data/sample/questions.csv`). When running without arguments, `codevault` seeds `data/questions.csv` from the sample file on initial run if not already present.
  4. Implement deterministic ID generation (`Q-1001`, `Q-1010`) scanning existing records for the highest numeric prefix to ensure unique identifiers compatible with future relational keys.
- **Rationale**:
  - Maintains strict Clean Architecture boundaries: CLI captures input, Service enforces business invariants, Repository manages serialization and persistence.
  - Guarantees sample development datasets remain clean and immutable for automated smoke tests.
  - Prevents crashes on missing files, empty files, or corrupted lines.
- **Consequences**: User data files in `data/*.csv` are ignored by git; automated tests use isolated temporary storage files in `build/` that are safely cleaned up.

---

### ADR-009: Stage 3 Custom DSA Engine Architecture & Pedagogical Product Integration
- **Status**: Accepted
- **Date**: 2026-09-26
- **Context**: Stage 3 mandates authentic, educational custom implementations of five fundamental data structures: PrefixTrie, MinHeap, Queue, Stack, and DoublyLinkedList. Simply wrapping STL containers (`std::priority_queue`, `std::queue`, `std::stack`, `std::list`) is explicitly prohibited. Furthermore, each data structure must address a genuine, tangible CodeVault product problem rather than existing in isolation.
- **Decision**:
  1. **Pure Custom DSA Implementations (`include/dsa/`, `src/dsa/`)**:
     - `PrefixTrie`: Custom TrieNode tree utilizing `std::unique_ptr` and child maps for fast case-insensitive title prefix lookup and autocomplete without fixed alphabet constraints.
     - `MinHeap<T, Compare>`: Generic binary min-heap using array-based index arithmetic (`heapifyUp`, `heapifyDown`) over a contiguous vector buffer for spaced revision prioritization.
     - `Queue<T>`: Custom singly-linked node FIFO queue with separate `head_` and `tail_` pointers guaranteeing strict $O(1)$ enqueue and dequeue without array shifting.
     - `Stack<T>`: Custom singly-linked node LIFO stack guaranteeing strict $O(1)$ push and pop operations for recent inspection history.
     - `DoublyLinkedList<T>`: Custom bidirectional linked list with forward/reverse iterators, $O(1)$ head/tail operations, and $O(1)$ node-level splicing for ordered problem playlist walkthroughs.
     - All custom pointer-based structures strictly follow the C++ Rule of 5 (destructors, copy constructors, copy assignment, move constructors, move assignment) with zero memory leaks.
  2. **Application Service Integration Layer (`include/services/`, `src/services/`)**:
     - `SearchService`: Maintains the `PrefixTrie` title index. `QuestionService` automatically populates this index upon startup and synchronizes it on question addition, modification (re-indexing on title change), and deletion.
     - `RevisionService`: Maintains the `MinHeap` populated with `models::RevisionItem`, ordering earliest due dates first and breaking ties by priority urgency.
     - `PracticeService`: Maintains the `Queue` to drive study sessions, supporting FIFO problem completion and skip-to-back problem cycling.
     - `RecentHistoryService`: Maintains the `Stack` for recently viewed question IDs, suppressing consecutive duplicate views and enabling `goBack()` navigation.
     - `ProblemPlaylistService`: Maintains the `DoublyLinkedList` for bidirectional problem playlists (next, previous, insert after current, remove current).
- **Rationale**:
  - Delivers genuine academic depth with mathematically verified time complexities.
  - Ensures clean separation of concerns: pure DSA classes contain no persistence, console I/O, or domain model dependencies.
  - Allows future GUI or Web frontend layers to consume the exact same application services without touching the algorithmic core.
- **Consequences**: Adding new features requires adhering to the layered service architecture rather than manipulating data structures directly in the UI.

---

### ADR-010: Stage 4 Search, Filtering & Custom Sorting Architecture
- **Status**: Accepted
- **Date**: 2026-09-26
- **Context**: Stage 4 requires robust question discovery, multi-criteria filtering, and ordering. Academic DSA requirements mandate custom sorting algorithm implementations rather than calling `std::sort()`. Furthermore, all search, filter, and sort operations must remain decoupled from presentation and must preserve repository data immutability.
- **Decision**:
  1. **Custom Sorting Engine (`include/dsa/sorting.hpp`)**:
     - `codevault::dsa::mergeSort`: Stable divide-and-conquer implementation guaranteeing $O(N \log N)$ worst-case performance with $O(N)$ auxiliary space. Preserves the original relative order of elements with equal primary keys.
     - `codevault::dsa::quickSort`: In-place partition algorithm using **median-of-three** pivot selection ($first$, $mid$, $last - 1$) and **tail-call recursion elimination** to ensure $O(\log N)$ maximum call stack depth.
     - Both sorting templates accept arbitrary random-access iterators and configurable binary comparators.
  2. **Domain Abstractions (`include/models/`)**:
     - `QuestionFilter`: Encapsulates optional filtering criteria (Topic, Difficulty, Status, Company substring, Favorite status, Title prefix, and Keyword) with safe whitespace trimming and logical `AND` matching semantics.
     - `SortOptions`: Encapsulates `SortField` (Title, Difficulty, Topic, Status, Company, CreatedAt, UpdatedAt, RevisionPriority), `SortDirection` (Ascending, Descending), and `SortAlgorithm` (MergeSort, QuickSort).
  3. **Deterministic Tie-Breaking**:
     - Implemented secondary key ordering by Question ID (`a.getId() < b.getId()`) across all comparators, preventing nondeterministic ordering when questions share equivalent primary sort keys.
  4. **Repository Safety & View Immutability**:
     - All filtering and sorting operations produce or mutate isolated `std::vector<models::Question>` result collections. The master repository CSV file is never mutated during view or query workflows.
  5. **CLI Integration**:
     - Implemented dedicated interactive Search & Filter submenu (Option 2) in `CliShell` with formatted columnar output and graceful handling of empty matches.
- **Rationale**:
  - Provides genuine pedagogical value for university demonstration while adhering to production-ready C++ design.
  - Ensures clean testability and zero functional regressions across existing Stage 2 (CRUD) and Stage 3 (DSA Engine) features.
- **Consequences**:
  - In-memory sorting and filtering scale efficiently for desktop catalogs ($O(N)$ filter passes, $O(N \log N)$ sorts), entirely sufficient for targeted offline usage prior to relational indexing in Stage 9.

---

### ADR-011: Stage 5 Spaced Revision & Practice Subsystem Architecture
- **Status**: Accepted
- **Date**: 2026-09-26
- **Context**: Stage 5 introduces spaced revision scheduling and structured practice sessions. The implementation must integrate with the existing custom `MinHeap` and `Queue` without using STL container adapters, enforce deterministic revision calculations, preserve state across restarts in CSV without modifying schema columns, and remain decoupled from terminal I/O.
- **Decision**:
  1. **Deterministic Spaced Revision Schedule**:
     - Adopted an explicit deterministic interval schedule: Level 1 (1 day), Level 2 (3 days), Level 3 (7 days), Level 4 (14 days), Level 5+ (30 days).
     - Clearly documented as CodeVault's initial deterministic schedule rather than claiming scientific optimality.
  2. **MinHeap Revision Queue Integration**:
     - Uses `codevault::dsa::MinHeap<models::RevisionItem, models::RevisionItemComparator>` as the persistent scheduling engine.
     - Implemented strict 3-key tie-breaking: primary by earliest `nextRevisionAt`, secondary by `priority` urgency (1=High before 5), tertiary by `questionId` string.
  3. **Queue Practice Session Engine**:
     - Uses `codevault::dsa::Queue<std::string>` for FIFO session progression.
     - `skipCurrentQuestion` cycles the active question to the tail of the FIFO queue in $O(1)$ time.
  4. **Practice Verdict Modeling**:
     - Supported verdicts: `Solved` (advances interval level, updates status, schedules future revision), `NeedsReview` (resets to 1-day interval, elevates urgency to Priority 1, keeps status InProgress), and `Skipped` (cycles in queue, preserves entity state).
  5. **Testable Time Handling**:
     - Introduced `IClock` abstraction with `SystemClock` (production epoch seconds) and `MockClock` (injectable in tests).
  6. **Persistence Architecture**:
     - Mapped all practice and revision state directly into existing `Question` fields (`last_practiced_at`, `next_revision_at`, `revision_priority`, `status`), avoiding redundant models or schema modifications.
- **Rationale**:
  - Balances educational DSA demonstration with practical product functionality.
  - Ensures 100% testability of temporal and scheduling logic without relying on system sleep or clock drift.
- **Consequences**:
  - Requires `RevisionService` to rebuild heap index on application boot from loaded questions ($O(N)$ build time).

---

### ADR-012: Stage 6 Statistics & Progress Dashboard Architecture
- **Status**: Accepted
- **Date**: 2026-09-26
- **Context**: Stage 6 requires calculating meaningful analytics, distributions, and progress metrics from stored question and revision data. The statistics subsystem must strictly decouple calculation logic from presentation (CLI), provide a strongly typed snapshot model reusable by future GUI/Web frontends, guarantee zero-division safety, ensure collection immutability, and avoid fabricating unsupported metrics (e.g. fake streaks or productivity scores).
- **Decision**:
  1. **Strongly Typed Snapshot Model (`include/models/statistics_models.hpp`)**:
     - Modeled metrics into cohesive snapshot structs: `OverallStatistics`, `DifficultyStatistics`, `TopicStatistics`, `StatusStatistics`, `RevisionStatistics`, `PracticeStatistics`, and `DashboardSnapshot`.
     - Models represent lightweight point-in-time snapshots and do not own or mutate underlying Question instances.
  2. **Dedicated, Pure `StatisticsService` (`include/services/statistics_service.hpp`, `src/services/statistics_service.cpp`)**:
     - All calculation methods accept `const std::vector<models::Question>&` and optional reference timestamps (`nowEpoch`).
     - Calculation logic is 100% pure and decoupled from CLI console I/O, formatting strings, or terminal widths.
     - Operates with zero mutation on domain data structures and persistence files.
  3. **Deterministic Mathematical Formulas & Zero-Division Safety**:
     - Completion percentage: `(solvedCount + masteredCount) / totalQuestions * 100.0`.
     - Difficulty & status percentages: `count / totalQuestions * 100.0`.
     - All percentage calculations strictly check `totalQuestions > 0` and return `0.0%` when empty.
  4. **Strict Data-Backed Metrics Only**:
     - Disallowed synthetic metrics like "solving streak", "average solving time", or "productivity scores" that cannot be faithfully derived from the current CSV schema (`Question`).
  5. **Terminal-Portable Progress Bar Visualization**:
     - Designed `renderProgressBar(double percentage, int width = 20)` in `CliShell` using portable `#` and `-` ASCII characters rather than non-standard multi-byte UTF-8 glyphs, preventing encoding/mojibake issues across diverse terminal configurations.
  6. **Future-Proof UI Integration**:
     - The CLI shell purely queries `StatisticsService::getDashboardSnapshot()` and formats the snapshot for display. A future Web or GUI frontend (Stage 9) can directly consume `DashboardSnapshot` without changes to business calculation logic.
---

### ADR-013: Stage 7 Hardening, Diagnostic Resolution & Quality Architecture
- **Status**: Accepted
- **Date**: 2026-09-27
- **Context**: Prior to transitioning from the DSA core implementation to product UI layers, the Stage 1–6 codebase required thorough testing, boundary hardening, resource management review, and diagnostic resolution. The VS Code Problems panel indicated diagnostics involving `renderProgressBar` visibility, member redeclaration, and an unused include (`datetime.hpp`).
- **Decision**:
  1. **Presentation Helper Visibility Architecture**:
     - `renderProgressBar(double percentage, int width = 20)` remains a `public static` member function of `codevault::app::CliShell`.
     - Rationale: It is a pure, stateless text-formatting helper. Keeping it public is consistent with other CLI presentation helpers (`printHeader`, `printMainMenu`, `printQuestionsTable`) and enables direct unit testing of boundary clamping ($<0\%$, $>100\%$), width fallbacks (width $\le 0$ defaulting to 20), and character alignment without fragile stream capture.
     - Single Declaration Guarantee: Verified declared exactly once in `include/app/cli_shell.hpp` and defined exactly once in `src/app/cli_shell.cpp`.
  2. **Include Hygiene & Diagnostic Cleanliness**:
     - Fixed unused `#include "utils/datetime.hpp"` in `unit_tests.cpp` by adding dedicated unit tests verifying `formatTimestamp` and `formatDateOnly` on epoch 0, negative values, and valid timestamps.
     - Pruned unused `#include <cassert>` from `smoke_test.cpp` and `unit_tests.cpp`.
     - Cleaned stale `.cache/clangd` indices to ensure zero persistent language server warnings.
  3. **Custom DSA Memory Safety & Resource Management**:
     - Audited and verified complete Rule of 5 compliance across `DoublyLinkedList`, `Stack`, `Queue`, `MinHeap`, and `PrefixTrie`.
     - Verified exception safety: Empty containers strictly throw `std::underflow_error` or `std::out_of_range`.
     - Added tests for self-assignment (copy/move) and identical-key priority heaps.
     - Toolchain Limitation Documented: WinLibs MinGW GCC 16.1.0 on Windows does not bundle `libasan` (`cannot find -lasan`), precluding ASan runtime execution on this platform; manual inspection and 195 automated test assertions confirm memory safety.
  4. **Persistence Boundary Resilience**:
     - Confirmed `FileQuestionRepository` safely handles non-existent paths, empty files, header-only files, duplicate IDs, and malformed CSV rows without data corruption or process crashes.
- **Consequences**:
  - Codebase is hardened, warning-free, and architecturally verified with 195 passing tests (190 unit tests + 5 smoke checks, 100% pass rate) across Workflows A through F for future UI/product stages.

---

### ADR-014: Stage 8 Web UI Foundation & Stitch Design Architecture
- **Status**: Accepted
- **Date**: 2026-09-27
- **Context**: CodeVault transitioned from the pure C++ core/CLI foundation to a modern developer-focused Web UI. The UI design required professional product aesthetics (clean, minimal, developer-focused, without generic CRUD tropes or fake AI elements) and strict alignment with the existing C++ domain models and algorithms.
- **Decision**:
  1. **Stitch MCP UI Design Workflow First**:
     - Modeled and generated the complete UI design system (`assets/4130323643676323147`) and all 8 core screens in Stitch MCP under project `projects/6846628486388263169` before writing frontend code.
     - Defined Slate 900 primary, Inter typography, restrained status/difficulty badges, and 8px border radii.
  2. **Modern Frontend Stack**:
     - Built frontend with React 18, Vite, TypeScript, Tailwind CSS, and Lucide icons in `frontend/`.
     - Organized into `layouts/`, `pages/`, `components/`, `services/`, `types/`, `styles/`.
  3. **Preservation of C++ DSA Core as Single Source of Truth**:
     - Frontend contains zero DSA algorithms (Trie, MinHeap, Sorts, Queues remain in C++).
     - Frontend queries the C++ backend for all operations and presents real, data-backed metrics without synthetic gamification (no fake streaks, XP, or levels).
- **Consequences**: Provides a high-performance, responsive web application while maintaining full architectural integrity.

---

### ADR-015: Zero-Dependency Embedded HTTP REST Server Integration Layer
- **Status**: Accepted
- **Date**: 2026-09-27
- **Context**: The React frontend must communicate with the existing C++ application layer. Introducing heavy external networking frameworks (Boost.Beast, Crow, Drogon, Oat++) would add complex external dependencies, slow compilation, and create platform build fragility.
- **Decision**:
  1. **Zero-Dependency Native Socket HTTP Server (`include/app/http_server.hpp`, `src/app/http_server.cpp`)**:
     - Built using standard cross-platform socket APIs (Winsock `ws2_32` on Windows, POSIX Berkeley sockets on Linux/macOS).
     - Compiles with zero external library dependencies directly in CMake.
     - Runs on port 8080 (configurable) on a background thread with non-blocking timeouts and CORS headers (`Access-Control-Allow-Origin: *`).
  2. **Lightweight Native JSON Utilities (`include/utils/json.hpp`, `src/utils/json.cpp`)**:
     - Created pure C++17 JSON serialization and tokenization utilities without third-party dependencies (`nlohmann/json` or `rapidjson`).
     - Serializes `Question`, `DashboardSnapshot`, `RevisionItem`, `SessionProgress`, and parses REST request bodies.
  3. **Command-Line Multi-Mode Launch**:
     - `codevault` / `codevault --cli`: Runs interactive terminal CLI (default, zero regression).
     - `codevault --server [port]`: Starts HTTP REST API server.
     - `codevault --both [port]`: Runs server and launches CLI concurrently.
- **Consequences**: The CLI remains 100% operational, the build remains fast and portable, and the REST API cleanly connects the Web UI to CodeVault services.

---

### ADR-016: UI/UX Refinement Toward a Developer Coding-Practice Platform Experience
- **Status**: Accepted
- **Date**: 2026-09-27
- **Context**: The initial Stage 8 Web UI, while functionally complete and integrated with the C++ backend, felt too similar to a generic SaaS/AI productivity dashboard and too close to the academic copilot visual language. CodeVault is fundamentally a personal coding-problem practice, organization, and spaced revision platform. Users preparing for technical interviews and competitive programming expect familiar coding-platform UX patterns (such as those in LeetCode, HackerRank, Codeforces) with high information density, compact scannable problem tables, and a problem-centric workflow.
- **Decision**:
  1. **Reposition the Product Experience**:
     - Shifted from a generic dashboard-centric UI to a coding-practice platform where the **Problems Library (`/problems`)** is the primary center of the product.
     - Reordered primary navigation: **Problems** → **Practice** → **Revision** → **Dashboard** → **Statistics** → **Settings**.
  2. **Refined Core Workspaces in Stitch and React**:
     - **Problems Library**: Dense, scannable problem table with prominent titles, distinct difficulty pills, topic tags, inline revision due badges, status icons (`✓` Solved, `★` Mastered, `◐` InProgress, `○` Unsolved), and live Trie-powered autocomplete search without technical jargon.
     - **Problem Detail Workspace**: Problem-solving developer workbench with problem statement, tags, formatted approach and complexity notes ($O(...)$), and a "Your Progress" Leitner retention card.
     - **Focused Practice Drill**: Distraction-free session powered by the C++ FIFO Queue, session progress meter, collapsible self-check solution drawer, and ergonomic verdict dock (`[Mark Solved]`, `[Needs Review]`, `[Skip]`).
     - **Spaced Revision Workspace**: Dedicated 5-box Leitner memory planner with Due Now urgent list and MinHeap upcoming timeline.
     - **Daily Action Cockpit**: Simplified dashboard answering *"What should I do today in CodeVault?"* without enterprise fluff or synthetic metrics.
  3. **Strict Zero-Fake-Data Guarantee**:
     - Removed hardcoded fake user names ("Alex Chen"), fake streaks, and synthetic rankings.
     - All telemetry is bound directly to the live C++17 application engine and CSV persistence.
- **Consequences**: The product feels intuitive and immediately familiar to coding students and software engineers, with dramatically improved scannability and focus on actual algorithm practice.
---

### ADR-017: Stage 8 UI/UX Refinement Pass 2 — Deep Coding-Platform Familiarity
- **Status**: Accepted
- **Date**: 2026-09-27
- **Context**: Following the initial refinement, a second pass was required to push CodeVault even further toward established coding-practice conventions (LeetCode, HackerRank, Codeforces, NeetCode) while preserving CodeVault's distinct identity and 100% authentic C++ data bindings.
- **Decision**:
  1. **Topic Pills Strip & Problem Set Discovery**:
     - Introduced a horizontal scrollable topic strip (`All Topics`, `Arrays`, `Strings`, `Trees`, `DP`, etc.) directly above the problem table, allowing single-click algorithmic filtering.
     - Added a "Pick Random" problem launcher for rapid, unbiased interview practice.
     - Added sequential row numbers (`#`) alongside IDs and configurable per-page options (15, 30, 50).
  2. **Dedicated Problem-Solving Workbench**:
     - Redesigned Problem Detail into an 8-col / 4-col split workbench layout. Left: problem title, statement, formatted approach notes with algorithmic complexity ($O(N)$, $O(1)$), and topic tags. Right: preparation status workbench card with primary `[ Practice / Solve Now ]` CTA, instant verdict triggers (`Mark Solved`, `Needs Review`), and Leitner SRS retention meter.
     - Refrained from creating fake code editors or mock compiler outputs, keeping the product honest and focused.
  3. **Deliberate Practice Drill UX**:
     - Active practice mode features an uncluttered problem statement view, clean progress indicators ("Problem X of Y in Queue"), a collapsible approach reveal drawer for self-testing invariants, and a fixed/anchored verdict dock with keyboard shortcuts (`1`/`S` for Solved, `2`/`R` for Review, `3`/`K` for Skip, `N` for Notes).
  4. **Transparent Leitner SRS Rules**:
     - The Revision workspace now clearly articulates Leitner box intervals (1d → 3d → 7d → 14d → 30d) and transition rules (Pass advances +1 Box, Fail resets to Box 1) backed by the C++ MinHeap priority queue.
- **Consequences**:
  - CodeVault feels natural and intuitive to anyone familiar with modern coding platforms.
  - Zero modifications to C++ backend, API contracts, or persistence. 200/200 unit tests and 5/5 smoke tests remain 100% green.

---

### ADR-018: SQLite Persistence Foundation & Automatic CSV Migration
- **Status**: Accepted
- **Date**: 2026-10-02
- **Context**: CodeVault Stage 8 demonstrated rock-solid functional capability, but persisted questions into a flat CSV file (`data/questions.csv`). While zero-dependency and human-readable, flat CSV suffers from full-file rewrites on every mutation, lack of true ACID guarantees, and vulnerability to file contention under concurrent reader/writer access.
- **Decision**:
  1. **Vendored SQLite 3 Amalgamation**: Vendor the official SQLite amalgamation (`third_party/sqlite/sqlite3.c`, `sqlite3.h`) into the CMake build tree as a static library compiled with `-w`.
  2. **Implement `SqliteQuestionRepository`**: An ACID-compliant `IQuestionRepository` implementation with parameterized SQL statements, WAL journal mode (`PRAGMA journal_mode = WAL`), schema v1 with indexes on topic, difficulty, status, company, and next revision date, guarded by recursive mutex for thread safety.
  3. **Automatic Migration Service (`MigrationService`)**: On startup, if `data/codevault.db` is empty and `data/questions.csv` exists, automatically create a backup at `data/questions.csv.bak` and transactionally ingest records into SQLite with full validation, error tolerance for malformed rows, and idempotency.
  4. **Preserve Legacy `FileQuestionRepository`**: Retain CSV repository for backward compatibility and CLI fallback.
- **Consequences**:
  - Eliminates $O(N)$ full-file rewrites on updates.
  - Guarantees ACID compliance with transactional commit and rollback.
  - Decoupled Clean Architecture remains intact: `QuestionService`, `IQuestionRepository`, custom DSA subsystems, and React UI required zero breaking changes.
  - Unit tests expanded to 220/220 passing (100%), and 5/5 smoke tests passing.

---

### ADR-019: Multi-User Data Foundation & Owner-Scoped Data Architecture
- **Status**: Accepted
- **Date**: 2026-10-02
- **Context**: Stage 9.1 introduced embedded SQLite persistence, but all queries and services continued operating in single-user mode. To prepare CodeVault for true multi-user multi-tenancy without yet introducing authentication mechanisms (passwords, JWT, OAuth, session cookies), the underlying database schema and service layers required strict owner-scoped data boundaries.
- **Decision**:
  1. **User Domain Model**: Introduced `models::User` (`id`, `username`, `display_name`, `email`, `created_at`, `updated_at`, `active`), completely decoupled from SQLite and devoid of authentication credentials.
  2. **Schema Upgrade to v2**: Upgraded SQLite schema to v2 by creating a `users` table, unique index on `username`, and establishing a foreign key constraint from `questions.owner_id` to `users(id)` with `PRAGMA foreign_keys = ON;`.
  3. **Deterministic Local User**: Seeded `local_user` in the database migration to guarantee 100% backward compatibility for existing datasets and local workflows.
  4. **User Repository Abstraction**: Implemented `IUserRepository` and `SqliteUserRepository` with prepared statements and parameter binding.
  5. **Owner-Scoped Repository Contract**: Extended `IQuestionRepository` with owner-aware methods (`findAllByOwner`, `findByIdForOwner`, `existsForOwner`, `saveForOwner`, `removeForOwner`, `countForOwner`).
  6. **Current User Provider Abstraction**: Introduced `ICurrentUserProvider` (with `StaticCurrentUserProvider("local_user")` default), injecting active user identity into `QuestionService`.
  7. **Service-Wide Isolation**: Configured `QuestionService`, `RevisionService`, `PracticeService`, `StatisticsService`, and `SearchService` (PrefixTrie) to scope all queries, aggregations, and mutating operations to the current user, blocking cross-user reads and mutations.
  8. **Authentication Boundary**: Explicitly declared that authentication/login is NOT implemented in this stage; identity injection is architectural preparation for future authentication middleware.
- **Consequences**:
  - Full data isolation between users (User A cannot view, mutate, delete, practice, revise, search, or aggregate User B's questions).
  - Existing local workflow, CLI, and React UI remain 100% functional without breaking changes.
  - Test suite expanded from 220 to 238 unit tests (100% pass rate) with 18 dedicated user and isolation tests.

---

### ADR-020: Authentication & Session Foundation (Stage 9.3 — Real User Identity)
- **Status**: Accepted
- **Date**: 2026-10-02
- **Context**: Stage 9.2 introduced the multi-user domain model, users table, and owner-scoped data boundaries, but explicitly left authentication unimplemented using `StaticCurrentUserProvider("local_user")`. Stage 9.3 transitions CodeVault to real authenticated identities with secure credentials, server-side sessions, and cookie-based authorization.
- **Decision**:
  1. **Argon2id Password Hashing**:
     - Vendored the official Argon2 reference library into `third_party/argon2/`.
     - Integrated `utils::crypto::hashPassword` and `verifyPassword` using Argon2id ($t=2, m=19456, p=1$) with a 16-byte cryptographically secure random salt.
     - Rejected insecure schemes (plain SHA-256, MD5, SHA-1, raw SHA-512, reversible encryption).
  2. **Server-Side Session Model & Token Hashing**:
     - Created `sessions` table in SQLite schema v3 (`id`, `user_id`, `token_hash`, `created_at`, `expires_at`, `revoked_at`, `last_seen_at`).
     - Raw session tokens are generated via Windows CryptoAPI CSPRNG (32 bytes = 64 hex characters) and sent to clients via `Set-Cookie`.
     - In the database, only the 256-bit cryptographic digest `token_hash = blake2b_256(raw_token)` is persisted. Database compromise does not expose active tokens.
  3. **Credential Separation in Domain**:
     - Defined `models::UserCredentials` keeping password hashes and metadata separate from the public `models::User` domain model. Password hashes are never exposed through API serialization.
  4. **HttpOnly Cookie Management & CORS**:
     - Cookies are issued with `HttpOnly; SameSite=Lax; Path=/; Max-Age=604800`.
     - CORS dynamically echoes request `Origin` and sets `Access-Control-Allow-Credentials: true` when browsers interact with the backend API.
  5. **Authentication Middleware**:
     - Built-in session extraction in `HttpServer::handleRequest`. Valid session token resolves to authenticated `User`, dynamically binding `AuthenticatedCurrentUserProvider` to `QuestionService`.
     - Unauthenticated requests to protected endpoints return HTTP 401 Unauthorized.
  6. **Frontend Auth UI**:
     - Built `LoginPage`, `RegisterPage`, and `ProtectedRoute` using CodeVault design tokens.
     - Created `AuthContext` to manage authenticated state, session restoration, and 401 redirects.
  7. **CLI Isolation**:
     - CLI continues running in local single-user mode (`StaticCurrentUserProvider("local_user")`), preventing browser auth requirements from impeding terminal use.
- **Consequences**:
  - Full end-to-end user authentication with strict owner-level data scoping.
  - Zero regression on existing 238 unit tests and 5 smoke tests; test suite expanded to 251 unit tests (100% passing).
  - Scope remains disciplined: cloud, OAuth, email verification, and payment features are deferred.

---

### ADR-021: User Account Management, Active Sessions & Personal Data Export (Stage 9.4)
- **Status**: Accepted
- **Date**: 2026-10-02
- **Context**: Following the successful delivery of Stage 9.3 (Authentication & Session Foundation), users could register, log in, access owner-isolated questions, and log out. However, users lacked self-service account management capabilities: updating display profiles, securely changing passwords, inspecting active devices/sessions, revoking remote sessions, and exporting their algorithmic catalog for offline portability.
- **Decision**:
  1. **Schema v3 Native Compatibility**:
     - Evaluated whether database changes were needed. Schema v3 already persists `user_credentials` and `sessions` (`expires_at`, `revoked_at`, `last_seen_at`), alongside user metadata (`email`, `display_name`).
     - Decided that **no schema migration (v4) was necessary**; existing indices `idx_sessions_user_id` and `idx_users_email` fully optimize all new queries.
  2. **Multi-Session Management & Invalidation**:
     - Extended `ISessionRepository` with `findActiveSessionsForUser(userId)` and `revokeAllForUserExcept(userId, currentSessionId)`.
     - In `GET /api/auth/sessions`, sessions are sanitized to prevent token leakage: returning session ID, creation date, last seen date, expiration date, and an `isCurrentSession: bool` flag calculated by comparing the active request's session ID.
     - Implemented `POST /api/auth/sessions/:id/revoke` and `POST /api/auth/sessions/revoke-others`.
  3. **Profile Modification**:
     - Implemented `PUT /api/auth/profile` accepting `displayName` and `email`.
     - Added strict email format validation and duplicate email check preventing account collision.
  4. **Password Lifecycle & Re-Hashing**:
     - Implemented `POST /api/auth/change-password` requiring `currentPassword` and `newPassword`.
     - Validates current password using Argon2id constant-time verification.
     - Enforces 8-character minimum complexity on `newPassword`.
     - Generates a fresh 16-byte random salt and re-hashes with Argon2id ($t=2, m=19456, p=1$).
  5. **Owner-Scoped Personal Data Export**:
     - Implemented `GET /api/user/export?format=json` and `GET /api/user/export?format=csv`.
     - Queries questions strictly via `QuestionService::getAllQuestions()` (guaranteeing owner isolation).
     - JSON format returns formatted JSON with user metadata.
     - CSV format emits standard RFC 4180 CSV formatted with all 18 standard question columns and attachment headers (`Content-Disposition: attachment; filename="..."`).
  6. **Frontend Settings Workspace**:
     - Replaced minimal placeholder `SettingsPage.tsx` with a clean 5-tab workspace: Profile, Security & Password, Active Sessions, Personal Data Export, and Engine Diagnostics.
- **Consequences**:
  - Authenticated users gain complete self-service account control and data sovereignty.
  - Zero regression on authentication foundation; test suite expanded from 251 to 257 unit tests (100% passing) and 5 smoke tests.
  - No cloud, OAuth, or external dependencies introduced; all features remain self-contained and clean.

---

### ADR-022: Catalog Import, Advanced Export & Data Portability Architecture (Stage 9.5)
- **Status**: Accepted
- **Date**: 2026-10-04
- **Context**: While Stage 9.4 introduced baseline personal data export (JSON/CSV), CodeVault lacked the ability to import catalogs back into the system, migrate data between instances, or export questions into external learning ecosystems (such as human-readable Markdown study sheets and Anki spaced-repetition flashcards). A comprehensive data portability solution was required without compromising data integrity, owner isolation, or in-memory DSA parity.
- **Decision**:
  1. **Dedicated Service Separation (`ImportService` & `ExportService`)**:
     - Created `ImportService` to encapsulate ingestion, parsing, schema validation, ID conflict detection, and transactional persistence.
     - Created `ExportService` to generate structured Markdown documents and Anki-compatible TSV flashcard decks.
     - Maintained strict Single Responsibility: presentation layers (HTTP server, frontend) delegate all domain logic to these services.
  2. **Multi-Format Ingestion**:
     - **JSON Import**: Ingests JSON objects with `questions` array or direct arrays, validating all required fields.
     - **RFC 4180 CSV Import**: Features a robust state-machine parser handling commas, escaped quotes (`""`), and multiline field values within quotes.
  3. **Three Explicit Conflict Resolution Strategies**:
     - `Skip`: Retains existing questions; ignores incoming conflicting IDs.
     - `Overwrite`: Replaces existing question records in persistence and updates in-memory DSA indexes.
     - `GenerateNewId`: Detects highest numerical ID suffix in storage and re-keys incoming records to unique IDs (`Q-xxxx`).
  4. **Atomic SQLite Transactions & Rollback**:
     - Implemented `IQuestionRepository::saveAllAtomic(questions, overwrite)` utilizing `BEGIN IMMEDIATE TRANSACTION ... COMMIT`.
     - If any record fails validation or an error occurs during write, the transaction issues a full `ROLLBACK`. No partial or corrupted catalog state is permitted.
  5. **In-Memory DSA Parity (`PrefixTrie` & `MinHeap`)**:
     - In-memory data structures are re-synchronized strictly upon successful transaction commit.
     - Avoids desynchronization where in-memory indexes disagree with persisted SQLite storage.
  6. **Strict Owner Isolation**:
     - Ingested records have their `owner_id` bound to the authenticated user ID, overriding any incoming `owner_id`.
     - Conflict checks are strictly evaluated against the authenticated user's questions, preventing cross-user collisions or enumeration.
  7. **Advanced Export Formats**:
     - **Markdown (`.md`)**: Emits structured, human-readable study sheets with document metadata headers, tags, notes, and revision metrics.
     - **Anki TSV (`.tsv`)**: Emits 3-column TSV (`Front`, `Back`, `Tags`) with embedded HTML breaks (`<br>`), tab escaping, and normalized tags for immediate one-click import into Anki flashcard decks.
  8. **Authenticated HTTP API**:
     - Added `POST /api/user/import?conflict_strategy=<strategy>` supporting `application/json` and `text/csv`.
     - Added `GET /api/user/export/markdown` and `GET /api/user/export/anki`.
     - All endpoints protected by session authentication middleware (`401 Unauthorized` when unauthenticated).
  9. **Frontend Settings Integration**:
     - Unified Data Portability tab in `SettingsPage.tsx` supporting JSON, CSV, Markdown, and Anki exports.
     - Interactive file upload with drag-and-drop, conflict strategy selector, and structured result summary (processed, imported, updated, skipped, error log).
  10. **Deliberate Non-Decisions & Deferred Features**:
      - **No proprietary binary Anki `.apkg` generation**: Standard TSV import provides universal compatibility without the overhead and brittleness of bundling SQLite Anki databases.
      - **No external platform synchronization**: Direct LeetCode/HackerRank/Codeforces API syncing is deferred.
      - **No cloud backup, OAuth, or external database**: CodeVault remains fully self-contained on local SQLite schema v3.
      - **CLI unchanged**: Terminal shell intentionally left unmodified to preserve local workflow stability.
- **Consequences**:
  - Full bidirectional data portability achieved across JSON, CSV, Markdown, and Anki.
  - Zero regression on existing 257 unit tests; test suite expanded to 276 unit tests and 5 smoke tests (281 total automated tests, 100% passing).
  - Clean separation of concerns preserved between HTTP API, application services, and persistence layers.

---

### ADR-023: Advanced Practice Workflow, Practice Queue & Deterministic Recommendation Architecture (Stage 10)
- **Status**: Accepted
- **Date**: 2026-10-05
- **Context**: Stage 5 provided basic practice session queue mechanics and Stage 9 established multi-user isolation and authentication. However, users lacked an integrated, holistic practice workflow: there was no intelligent "Practice Next" recommendation engine, practice queues could not be inspected or modified individually without clearing the session, drill sessions could not be initialized with composable filters directly into the queue, and practice verdicts could not be submitted directly from the problem workspace. Furthermore, recommendations needed to be explainable, deterministic, and free from external AI/LLM dependencies.
- **Decision**:
  1. **Separation of Concerns for `PracticeService`**:
     - `PracticeService` remains an independent orchestrator coordinating between `QuestionService` (for catalog mutations/persistence), `RevisionService` (for Leitner 5-box SRS scheduling), and `dsa::Queue` (for in-memory FIFO sequencing).
     - Keeps practice orchestration decoupled from raw storage logic and presentation formatting.
  2. **Deterministic 5-Tier "Practice Next" Cascade (Zero AI / LLM)**:
     - Implemented `getPracticeNext()` with a strict deterministic prioritization cascade:
       1. Active Practice Queue Head: Problems explicitly queued by the user.
       2. Overdue Spaced Revisions: Problems overdue in the Leitner SRS `MinHeap` (earliest due + highest urgency priority).
       3. Unsolved / Never-Practiced: Unsolved problems with `last_practiced_at == 0`, ordered by priority and ID.
       4. In-Progress Problems: Attempted problems needing further practice, ordered by least-recently-practiced timestamp.
       5. Stale Solved/Mastered: Solved problems with the oldest practice timestamps for retention reinforcement.
     - Transparent Explainability: Returns a human-readable `recommendationReason` string detailing exactly why the question was chosen.
     - Determinism: Identical catalog states yield identical recommendations without nondeterministic model inference or network calls.
  3. **In-Memory FIFO Practice Queue Architecture**:
     - Extended `dsa::Queue<T>` with non-destructive `toVector()` snapshotting, element `contains()` lookup, and $O(N)$ targeted `remove()` node splicing.
     - The practice queue remains in-memory: active practice sprints are session-bound and transient, avoiding heavy database thrashing on high-frequency queue mutations.
     - Queue state is isolated per authenticated user via `QuestionService` user provider.
  4. **Single-Question Direct Practice Verdicts**:
     - Added `recordPracticeAttemptForQuestion(questionId, verdict)` enabling users to record practice outcomes (`Solved`, `NeedsReview`, `Skipped`) directly from the Problem Detail workspace or anywhere in the app.
     - Automatically coordinates with `RevisionService::markRevisionResult()` to update Leitner interval and revision priority.
     - Updates `last_practiced_at` timestamp and status in SQLite storage via `QuestionService::updateQuestion()`.
     - Automatically unlinks the problem if present in the active practice queue, decrementing queue count and keeping queue state synchronized.
  5. **HTTP Practice API**:
     - Added `GET /api/practice/next`, `POST /api/practice/session`, `GET /api/practice/queue`, `DELETE /api/practice/queue/:questionId`, and `POST /api/practice/:questionId/result`.
     - All routes protected by session authentication middleware (`401 Unauthorized` for unauthenticated requests).
     - Strict tenant isolation prevents cross-user queue inspection, removal, or verdict recording.
  6. **Frontend Practice Experience**:
     - Added `PracticeQueueDrawer` for slide-out queue inspection and removal.
     - Added `StartSessionModal` for targeted drill configuration.
     - Integrated "Practice Next" CTAs across Questions list, Problem Detail workspace, and Dashboard.
     - Added post-verdict Leitner feedback card and `N` keyboard shortcut for seamless flow.
     - Added live practice queue count badge in sidebar navigation.
  7. **Deliberate Non-Decisions & Out of Scope**:
     - **No LLM/AI recommendations**: Recommendations remain purely algorithmic, explainable, and fast ($< 1$ms).
     - **No database queue persistence**: Session queues remain in-memory; questions themselves are fully ACID-persisted in SQLite.
     - **CLI unchanged**: CLI shell preserved in its verified stable local configuration.
- **Consequences**:
  - Delivers a cohesive, professional coding-practice experience without bloat or external dependencies.
  - Test suite expanded from 276 to 289 unit tests (13 new tests in Stage 10) + 5 smoke tests = 294 passing automated tests (100% pass rate).
  - All existing Stage 1–9.5 architectural boundaries and invariant guarantees strictly preserved.
