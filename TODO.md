# CodeVault — Engineering Roadmap & Staged Task Tracker

> **Working Guideline**: Tasks must be completed strictly in sequence across designated stages. Do not begin work on subsequent stages without completing and validating the current stage.

---

## Stage 1 — Foundation (Completed & Verified)
- [x] Inspect workspace and initialize project repository structure
- [x] Formulate high-level architecture specification (`ARCHITECTURE.md`)
- [x] Author technical context and project rules (`PROJECT_CONTEXT.md`)
- [x] Design domain entities and data model (`DATA_MODEL.md`)
- [x] Author DSA design document and educational mapping (`DSA_DESIGN.md`)
- [x] Record Architectural Decision Records (`DECISIONS.md`)
- [x] Establish testing strategy and test plan (`TESTING.md`)
- [x] Define contributor guidelines and conventions (`CONTRIBUTING.md`)
- [x] Configure `.gitignore` for C++ / CMake development
- [x] Create root `CMakeLists.txt` targeting C++17
- [x] Structure source code tree (`src/`, `include/`, `tests/`, `data/`)
- [x] Implement minimal runnable CLI application shell
- [x] Create realistic sample dataset under `data/sample/questions.csv`
- [x] Configure and execute initial smoke test runner
- [x] Verify clean build and test execution

---

## Stage 2 — Core Question Management (Completed & Verified)
- [x] Implement core domain models (`Question`, `Difficulty`, `Status`, `Topic`, `Platform`)
- [x] Implement `IQuestionRepository` interface with `exists`, `findById`, `findAll`, `save`, and `remove`
- [x] Implement `FileQuestionRepository` with CSV parsing, atomic file flush persistence, and corruption guardrails
- [x] Implement `QuestionService` handling validation, CRUD operations, and ID generation:
  - [x] Add new question with comprehensive business-rule validations
  - [x] Update existing question metadata, notes, URLs, and tags
  - [x] Delete question by ID with existence verification
  - [x] Mark question as Unsolved / InProgress / Solved / Mastered
  - [x] Toggle Favorite flag
  - [x] Auto-generate deterministic sequential Question IDs (`Q-1001`, `Q-1010`)
- [x] Connect CLI interactive menu to `QuestionService` CRUD actions with input validation
- [x] Add comprehensive 32-case unit test suite (`unit_tests`) covering domain model, validation, repository, and persistence roundtrip

---

## Stage 3 — DSA Engine Implementation (Completed & Verified)
- [x] Implement `PrefixTrie` (`include/dsa/trie.hpp`, `src/dsa/trie.cpp`):
  - [x] Node structure with `std::unique_ptr` and child maps (`std::unordered_map<char, ...>`)
  - [x] Safe string handling: case-normalization, special characters, whitespace, and empty strings
  - [x] Keyword/title prefix insertion, removal with leaf pruning, and contains check
  - [x] Autocomplete and prefix match traversal (`autocomplete`, `getWordsWithPrefix`)
  - [x] Dedicated 12-case unit test suite
- [x] Implement `MinHeap` Priority Queue (`include/dsa/min_heap.hpp`):
  - [x] Generic contiguous binary heap over dynamic vector buffer (without `std::priority_queue`)
  - [x] Custom comparator support (`std::less<T>` and `RevisionItemComparator`)
  - [x] `push`, `pop`, `top`, `extractMin`, `buildHeap` operations with $O(\log N)$ bounds
  - [x] Duplicate priorities handling and underflow safety guarantees
  - [x] Dedicated 8-case unit test suite
- [x] Implement `Queue` (`include/dsa/queue.hpp`):
  - [x] Custom linked-node FIFO queue with head and tail pointers (without `std::queue`)
  - [x] $O(1)$ `enqueue`, `dequeue`, `front`, and `back` without element shifting
  - [x] Rule of 5 compliance (deep copy, move semantics, clean RAII destruction)
  - [x] Dedicated 6-case unit test suite
- [x] Implement `Stack` (`include/dsa/stack.hpp`):
  - [x] Custom linked-node LIFO stack with top pointer (without `std::stack`)
  - [x] $O(1)$ `push`, `pop`, and `top` operations
  - [x] Rule of 5 compliance (deep copy, move semantics, clean RAII destruction)
  - [x] Dedicated 5-case unit test suite
- [x] Implement `DoublyLinkedList` (`include/dsa/doubly_linked_list.hpp`):
  - [x] Bidirectional linked list with `prev` and `next` pointers (without `std::list`)
  - [x] $O(1)$ `pushFront`, `pushBack`, `popFront`, and `popBack` operations
  - [x] $O(N)$ index-based `insert` and `removeAt` ($O(1)$ node-level splicing)
  - [x] Bidirectional forward/reverse iterators and range-based for loop compatibility
  - [x] Rule of 5 compliance (deep copy, move semantics, clean RAII destruction)
  - [x] Dedicated 12-case unit test suite
- [x] Integrate DSA Engines into CodeVault Architecture:
  - [x] `SearchService`: Trie-backed question title indexing synchronized with `QuestionService` CRUD
  - [x] `RevisionService`: MinHeap-backed spaced revision scheduling using `RevisionItem`
  - [x] `PracticeService`: Queue-backed FIFO study sessions with skip-to-back cycling
  - [x] `RecentHistoryService`: Stack-backed navigation tracking with consecutive duplicate suppression
  - [x] `ProblemPlaylistService`: DoublyLinkedList-backed bidirectional study walkthrough
  - [x] Dedicated 8-case service integration test suite (total 83 unit & integration tests)

---

## Stage 4 — Search, Filtering & Sorting (Completed & Verified)
- [x] Implement `SearchService`:
  - [x] Fast prefix search & autocomplete using `PrefixTrie` (`searchByTitlePrefix`, `searchTitlesByPrefix`)
  - [x] Full-field keyword search across Title, Description, Company, Topic, and Tags (`searchByKeyword`)
  - [x] Multi-criteria filtering via `QuestionFilter` (Topic, Difficulty, Status, Company, Favorite)
  - [x] Conjunction filtering with logical `AND` semantics and safe whitespace handling
- [x] Implement Custom Algorithmic Sorting (`include/dsa/sorting.hpp`):
  - [x] Custom `mergeSort`: Stable recursive divide-and-conquer ($O(N \log N)$ worst-case, $O(N)$ extra space)
  - [x] Custom `quickSort`: In-place partition with median-of-three pivot selection and tail-call optimization ($O(\log N)$ stack frames)
  - [x] Multi-field ordering: Title, Difficulty ($1 \to 3$), Topic, Status ($1 \to 4$), Company, Created Date, Updated Date, Revision Priority
  - [x] Deterministic secondary tie-breaking by Question ID (`getId()`)
- [x] Integrate Search, Filter & Sort workflows into CLI shell:
  - [x] Interactive dedicated submenu (Option 2)
  - [x] Columnar formatted table display (`printQuestionsTable`) with safe truncation
  - [x] User-friendly empty set messages ("No questions matched the selected criteria.")
- [x] Add comprehensive 39-case unit and integration test suite (total 122 unit tests + 5 smoke tests passing 100%)
- [x] Guarantee collection immutability: sorting and filtering operates on isolated copies, preserving CSV persistent storage

---

## Stage 5 — Spaced Revision & Practice Queue (Completed & Verified)
- [x] Implement deterministic spaced revision interval progression (1, 3, 7, 14, 30-day schedule)
- [x] Implement `RevisionService` (`include/services/revision_service.hpp`, `src/services/revision_service.cpp`):
  - [x] Custom `MinHeap` scheduling with 3-key deterministic tie-breaking (nextRevisionAt, priority urgency, questionId)
  - [x] Due questions query (`next_revision_at <= now()`)
  - [x] Upcoming revisions query (`next_revision_at > now()`) sorted chronologically
  - [x] Scheduling and rescheduling operations without heap duplicate accumulation
  - [x] Automatic calculation of next interval, urgency priority, and status upon practice verdict
  - [x] Testable `IClock` abstraction with `SystemClock` and `MockClock`
- [x] Implement `PracticeService` (`include/services/practice_service.hpp`, `src/services/practice_service.cpp`):
  - [x] Custom `Queue` FIFO practice session execution
  - [x] Start sessions by filter (all unsolved, topic, difficulty, favorites, due revisions)
  - [x] Practice verdicts: `Solved`, `NeedsReview`, `Skipped`
  - [x] Front-to-back question cycling on `Skipped`
  - [x] Session progress metrics (total, completed, remaining, skipped)
- [x] Integrate CLI presentation workflows:
  - [x] Practice Submenu (Option 3) with question prompts, verdict selection, and session summary
  - [x] Revision Submenu (Option 4) with due table, upcoming table, next revision inspect, schedule question, and direct due session start
- [x] Maintain CSV persistence across application restart using standard 18 fields
- [x] Add comprehensive 30-case unit test suite (total 152 unit tests + 5 smoke tests passing 100%)
- [x] Author dedicated documentation: `REVISION_AND_PRACTICE.md`

---

## Stage 6 — Dashboard & Progress Statistics (Completed & Verified)
- [x] Create strongly typed statistics domain models (`include/models/statistics_models.hpp`):
  - [x] `OverallStatistics` (total, solved, in-progress, unsolved, mastered, favorites, due, upcoming, completion percentage)
  - [x] `DifficultyStatistics` and `DifficultyCount` (Easy, Medium, Hard breakdown with safe percentages)
  - [x] `TopicStatistics` and `TopicCount` (all 14 curriculum topics with distinct topic counts)
  - [x] `StatusStatistics` (Unsolved, InProgress, Solved, Mastered breakdown)
  - [x] `RevisionStatistics` (due count, upcoming count, unscheduled count, priorities 1..5, levels 1..5)
  - [x] `PracticeStatistics` (practiced count, unpracticed count, latest practice epoch)
  - [x] `DashboardSnapshot` (unified immutable snapshot with timestamp)
- [x] Implement `StatisticsService` (`include/services/statistics_service.hpp`, `src/services/statistics_service.cpp`):
  - [x] Pure deterministic calculations without CLI dependencies
  - [x] Strict zero-division protection across all formulas
  - [x] Full collection and persistent repository immutability
  - [x] Service integration coordinating `QuestionService` and `RevisionService`
- [x] Integrate CLI Dashboard (Option 5 in Main Menu):
  - [x] Dedicated dashboard view rendering overall completion, difficulty, status, topic coverage, revision urgency/level breakdown, and practice metrics
  - [x] Terminal-portable progress bar rendering (`[##########----------] 50.0%`)
  - [x] Graceful empty catalog handling
- [x] Comprehensive automated test coverage:
  - [x] 18 new unit tests added in `tests/unit/unit_tests.cpp` (Tests 153–170)
  - [x] Total suite expanded to 170 unit tests + 5 smoke checks = 175 tests passing (100% pass rate)
- [x] Author comprehensive documentation:
  - [x] Dedicated `STATISTICS_AND_DASHBOARD.md` architectural specification
  - [x] Recorded `ADR-012` in `DECISIONS.md`
  - [x] Updated `README.md`, `PROJECT_CONTEXT.md`, `ARCHITECTURE.md`, `TESTING.md`


---

## Stage 7 — Testing, Hardening & Quality (Completed & Verified)
- [x] Comprehensive unit test suite covering 100% of custom DSA modules and services (190 unit tests + 5 smoke checks = 195 tests passing 100%)
- [x] Dedicated End-to-End Integration Workflows (A through F):
  - [x] Workflow A: Create 4 questions -> Persist -> Reload -> Prefix/Keyword Search -> Multi-Filter -> Difficulty Sorting
  - [x] Workflow B: Create question -> Practice -> Mark Solved -> 1-Day Schedule Generated -> Persist -> Reload -> Due Detection
  - [x] Workflow C: Question Needs Review -> 1-Day Reset & Urgent Priority 1 Escalation -> Persist -> Reload -> Due at Highest Priority
  - [x] Workflow D: Multi-question practice session -> Skip (cycles to back) -> Solve -> Exit -> Session Summary & Persistence
  - [x] Workflow E: Modify Question in collection -> Re-compute Dashboard Snapshot -> Immutability Verification
  - [x] Workflow F: Search/Filter/Sort -> Dashboard -> Revision -> Multi-Service Pipeline Parity across Application Restart
- [x] Boundary and edge case testing:
  - [x] CSV resilience: empty files, header-only files, non-existent files, malformed rows, missing columns, corrupt timestamps/priorities
  - [x] CSV escaping: RFC-4180 quoted fields with embedded commas, quotes (`""`), semicolons
  - [x] Duplicate IDs: idempotent storage preserving single key
  - [x] Custom DSA underflow exceptions: `front()`, `back()`, `popFront()`, `popBack()`, `top()`, `pop()`, `dequeue()`, `extractMin()`
  - [x] Custom DSA copy and move semantics verified
  - [x] PrefixTrie: empty string invariants, 300-char strings, punctuation/hyphenated keys
  - [x] Domain validation boundaries: 255/256 char title limits, whitespace rejection, illegal ID characters, priority 1..5 range
  - [x] Revision & practice boundaries: exact due timestamp (`now == dueAt`), empty practice sessions, single-item skip cycling
  - [x] Progress bar presentation: negative/overflow percentage clamping, width fallback
- [x] Memory safety and resource management review:
  - [x] Rule of 5 verified across DoublyLinkedList, Stack, Queue, MinHeap, and PrefixTrie
  - [x] Toolchain limitation documented: WinLibs MinGW GCC 16.1.0 on Windows lacks `libasan`; manual lifetime analysis & 195 tests verify safety
- [x] Compiler and diagnostic cleanliness:
  - [x] 0 compiler errors, 0 compiler warnings with `-Wall -Wextra -Wpedantic`
  - [x] 0 clangd IDE diagnostics across all translation units
  - [x] Fixed `CliShell::renderProgressBar` visibility architecture and single declaration guarantee
  - [x] Removed unused `#include <cassert>` in `smoke_test.cpp` and `unit_tests.cpp`
  - [x] Added unit tests for `utils/datetime.hpp` resolving unused include warning

---

## Stage 8 — Professional Web UI Foundation + Stitch Design + Real Integration (Completed & Verified)
- [x] **Repository & Architecture Inspection**:
  - [x] Inspected C++17 Clean Architecture, domain models, services, persistence, and tests
  - [x] Authored `UI_ARCHITECTURE.md`, `UI_DESIGN_SYSTEM.md`, and `API_CONTRACT.md`
- [x] **Stitch MCP UI Design Workflow**:
  - [x] Created Stitch Project: `projects/6846628486388263169`
  - [x] Created Stitch Design System: `assets/4130323643676323147` ("CodeVault Design System", Light, Slate 900 primary, Inter)
  - [x] Generated initial screens in Stitch (Dashboard, Catalog, Detail, Form, Practice, Revision, Statistics, Settings)
- [x] **Backend Integration Layer**:
  - [x] Implemented zero-dependency JSON serializer and parser (`include/utils/json.hpp`, `src/utils/json.cpp`)
  - [x] Implemented cross-platform native socket HTTP server (`include/app/http_server.hpp`, `src/app/http_server.cpp`)
  - [x] Implemented REST endpoints with CORS support on port 8080
  - [x] Integrated command-line arguments in `main.cpp`: `--server [port]`, `--cli`, `--both [port]`, `--help`
  - [x] Fully preserved existing interactive CLI (`CliShell`) with 0 regressions
  - [x] Added 10 new unit tests (Tests 191–200) covering JSON serialization and API handling
  - [x] 205 total tests passing (200 unit tests + 5 smoke checks, 100% pass rate)

---

## Stage 8 Refinement — Coding-Practice Platform Experience (Completed & Verified)
- [x] **Product UX Repositioning**:
  - [x] Repositioned UI mentally as a personal coding-problem practice, organization, and revision platform (not a generic SaaS/AI dashboard)
  - [x] Shifted Information Architecture to make **Problems** the primary entry point and centerpiece of the product
  - [x] Navigation reordered: **Problems** → **Practice** → **Revision** → **Dashboard** → **Statistics** → **Settings**
  - [x] Strict zero-fake-data policy: removed fake usernames ("Alex Chen"), fake streaks, and synthetic rankings
- [x] **Stitch MCP Design Refinement**:
  - [x] Refined Problems Library (`1fe8a780b6c549fb90c2e3ce585f18b0`)
  - [x] Refined Problem Detail Workspace (`8c6a65e714a741c0bad11c7dc7fef391`)
  - [x] Refined Focused Practice Drill Session (`79e9d337251746d2a03ac892b49fa642`)
  - [x] Refined Spaced Revision Workspace (`43ea6a18f80c48ffbb559a1be4dd2a8f`)
- [x] **Frontend Screen Refactorings**:
  - [x] `App.tsx` & `AppLayout.tsx`: Updated routes and developer-first navigation with C++17 engine telemetry
  - [x] `QuestionsPage.tsx`: High-density scannable rows, dominant problem titles, status icons, live autocomplete suggestions, and inline revision state
  - [x] `QuestionDetailPage.tsx`: Problem-solving workbench with description, formatted notes, tags, and "Your Progress" Leitner retention card
  - [x] `PracticePage.tsx`: Distraction-free recall drill, FIFO Queue runner, self-check solution drawer, and ergonomic verdict dock
  - [x] `RevisionPage.tsx`: Leitner 5-box distribution ribbon, Due Now table with instant Pass/Fail controls, and MinHeap upcoming timeline
  - [x] `DashboardPage.tsx`: Daily action cockpit answering "What should I do today in CodeVault?" with real progress metrics
  - [x] `StatisticsPage.tsx`: Developer-oriented curriculum analytics without oversized marketing cards
- [x] **Verification & Validation**:
  - [x] C++ build: 0 errors, 0 warnings with MinGW GCC 16.1.0 + Ninja
  - [x] C++ test suite: 200/200 unit tests passing, 5/5 smoke checks passing (205/205 total checks, 100% pass rate)
  - [x] Frontend build: `npm.cmd run build` passes with 0 TypeScript/Vite errors
  - [x] Preserved zero-dependency C++ core, CLI, and REST gateway integrity

---

## Stage 8 — UI/UX Refinement Pass 2 (Coding-Practice Platform Familiarity) (Completed & Verified)
- [x] **Stitch MCP Screen Refinement (Pass 2)**:
  - [x] Generated high-density Problems Library Screen (`d9fb6890285c4a9485f076e94e194d22`) adhering to coding-practice platform conventions
  - [x] Preserved CodeVault Design System (`4130323643676323147`) with slate-900 primary, restrained accents, and monospace metrics
- [x] **Problems Page (The Core Experience)**:
  - [x] Horizontal scrollable topic pills strip (`All Topics`, `Arrays`, `Strings`, `Trees`, `Graphs`, `DP`, etc.)
  - [x] Quick "Pick Random" problem launcher for rapid practicing
  - [x] Compact rows with sequential numbering (`#`), status geometric icons (`✓` Solved, `★` Mastered, `◐` InProgress, `○` Unsolved), dominant title, difficulty badge, topic badge, inline revision state, favorite star, and quick actions
  - [x] Configurable page size selector (15, 30, 50 problems per page)
- [x] **Problem Detail Workspace**:
  - [x] 8-col / 4-col split workbench layout separating problem statement and approach from preparation status
  - [x] Formatted approach notes and time/space complexity invariants ($O(N)$, $O(1)$)
  - [x] "Preparation Status" workbench card with primary `[ Practice / Solve Now ]` CTA, instant verdict toggles (`Mark Solved`, `Needs Review`), Leitner SRS retention meter, and reschedule modal
  - [x] No fake code editors or synthetic compilation output
- [x] **Practice Page (Solving-Focused Mode)**:
  - [x] Focused problem-solving canvas with dedicated progress bar ("Problem X of Y")
  - [x] Collapsible approach and invariant reveal drawer for quick self-check
  - [x] Fixed bottom verdict dock (`Mark Solved`, `Needs Review`, `Skip`) with keyboard shortcuts (`1`/`S`, `2`/`R`, `3`/`K`, `N`)
- [x] **Revision Page (Obvious Spaced-Repetition Model)**:
  - [x] Leitner 5-Box SRS distribution ribbon with explicit transition rules (`Pass` advances +1 Box, `Fail` resets to Box 1)
  - [x] Immediate action controls (`Pass (+1 Box)`, `Fail (Reset)`, `Reschedule`) on Due Now items
  - [x] Clean upcoming revision timeline ordered by C++ priority MinHeap
- [x] **Full Stack Validation & Hardening**:
  - [x] Frontend build: `npm.cmd run build` passes with 0 errors
  - [x] C++ build: `ninja -C build` passes with 0 errors, 0 warnings
  - [x] Unit test suite: 200/200 tests passing
  - [x] Smoke test suite: 5/5 tests passing
  - [x] 0 changes to C++ core, DSA implementations, persistence, CLI, or API contracts

---

## Stage 8 — Final Functional UX Audit & Polish (Completed & Verified)
- [x] **Full User Journey Functional Audit**:
  - [x] Problems library search, Trie autocomplete suggestions, topic pills filtering, difficulty/status filters, and sorting
  - [x] Problem Detail workbench: loaded from URL, metadata, external links, notes, and preparation status
  - [x] Practice drill queue: single-problem and batch queuing, FIFO queue runner, progress bar, verdict recording, and queue cycling on skip
  - [x] Leitner Spaced Revision: due items, upcoming items timeline, Leitner 5-box distribution, and reschedule modal
  - [x] Dashboard daily cockpit and Statistics curriculum analytics without synthetic or simulated metrics
  - [x] Settings diagnostics: runtime C++ DSA subsystem status and file storage path
- [x] **Genuine Functional & Usability Fixes**:
  - [x] `RevisionPage.tsx`: Wired genuine Leitner interval progression and resets into `handleQuickVerdict` so "Pass" advances intervals (+1 Box) and "Fail" resets to Box 1 Urgent in MinHeap & CSV persistence
  - [x] `QuestionDetailPage.tsx`: Fixed `handleRecordVerdict` to advance/reset the Leitner schedule and update the SRS card; fixed `handleStartPracticeThis` to start a practice queue for that specific problem (`questionIds: [id]`)
  - [x] `QuestionsPage.tsx`: Added `searchParams` URL query sync effect for cross-page navigation from Dashboard topic links and Header search; hooked row `Practice Drill` button to queue that specific problem; clamped pagination index
  - [x] `QuestionFormPage.tsx`: Fixed redirect on save to point to canonical `/problems/:id` route
  - [x] `PracticePage.tsx`: Added `questionId` param support; protected keyboard shortcuts against modifier keys (Ctrl/Cmd/Alt) and `contentEditable` / textbox inputs
  - [x] `ConfirmModal.tsx`, `RevisionPage.tsx`, `QuestionDetailPage.tsx`: Added `Escape` key listeners and backdrop click dismissal for modal accessibility
- [x] **Persistence & Process Restart Verification**:
  - [x] Verified full persistence round-trip across backend process termination and restart against `data/questions.csv`
- [x] **Final Full-Stack Validation**:
  - [x] Frontend build: `npm.cmd run build` passes with 0 errors
  - [x] C++ build: `ninja -C build` passes with 0 errors, 0 warnings
  - [x] Automated test suites: 200/200 unit tests passing, 5/5 smoke tests passing (205/205 total checks, 100% pass rate)

---

## Stage 9 — Persistence Architecture Upgrade (Stage 9 Phase 1 Completed & Verified)
- [x] **Vendored SQLite 3 Engine**: Vendored SQLite amalgamation into `third_party/sqlite` and integrated static library target into CMake.
- [x] **SQLite Repository Implementation (`SqliteQuestionRepository`)**:
  - [x] Schema v1 with `schema_version` tracking and indexes (`idx_questions_topic`, `idx_questions_difficulty`, etc.)
  - [x] WAL journal mode (`PRAGMA journal_mode = WAL`) and ACID transaction management
  - [x] Full CRUD support mapping all 18 `Question` domain attributes
  - [x] Thread-safe execution via `std::recursive_mutex` and RAII statement handling
- [x] **Automatic CSV-to-SQLite Migration (`MigrationService`)**:
  - [x] Automatic zero-loss migration on startup from `data/questions.csv` to `data/codevault.db`
  - [x] Automatic pre-migration backup generation at `data/questions.csv.bak`
  - [x] Robust RFC 4180 parsing, entity validation, and transactional batch insertion
  - [x] Row count invariant verification and re-run idempotency
- [x] **Backward Compatibility**: Preserved `FileQuestionRepository` for legacy workflows.
- [x] **HTTP & Diagnostics Integration**: Updated `/api/settings/diagnostics` to report `storageType: sqlite`, schema version, and database file size.
- [x] **Live Persistence & Restart Verification**: Verified mutations via REST API persist cleanly across process termination and reboot.
- [x] **Testing & Validation**:
  - [x] Unit test suite expanded from 200 to 220 tests (220/220 passing, 100% pass rate).
  - [x] Smoke test suite: 5/5 passing.
  - [x] CTest suite: 2/2 suites passing (100%).
  - [x] Frontend production build: `npm.cmd run build` passes with 0 errors.

---

## Stage 9.2 — Multi-User Data Foundation (Completed & Verified)
- [x] **Audit Existing Owner Id**: Audited dormant `owner_id` field across storage, services, and queries.
- [x] **User Domain Model (`models::User`)**: Created clean User entity with `id`, `username`, `display_name`, `email`, timestamps, and active status, decoupled from SQLite.
- [x] **Schema v2 Migration**:
  - [x] Upgraded SQLite schema to v2 with `users` table and indexes.
  - [x] Established foreign key `questions.owner_id -> users(id)` with `PRAGMA foreign_keys = ON;`.
  - [x] Seeded deterministic default user (`local_user`) for backward compatibility.
- [x] **User Repository Abstraction (`IUserRepository` & `SqliteUserRepository`)**:
  - [x] Implemented user CRUD, username uniqueness, email lookup, and user deactivation with prepared statements.
- [x] **Current User Abstraction (`ICurrentUserProvider`)**:
  - [x] Created `ICurrentUserProvider` with `StaticCurrentUserProvider("local_user")` default.
  - [x] Injected active user provider into `QuestionService`.
- [x] **Owner-Scoped Repository Contract**:
  - [x] Extended `IQuestionRepository`, `SqliteQuestionRepository`, and `FileQuestionRepository` with `findAllByOwner`, `findByIdForOwner`, `existsForOwner`, `saveForOwner`, `removeForOwner`, `countForOwner`.
- [x] **Service-Wide Isolation**:
  - [x] `QuestionService`, `RevisionService`, `PracticeService`, `StatisticsService`, and `SearchService` (PrefixTrie) enforce current-user scoping.
  - [x] Implemented dynamic scoping refresh on user switch (`refreshUserScope()`).
- [x] **Security Principle Enforced**: HTTP endpoints override client-supplied `owner_id` with verified identity.
- [x] **Testing & Verification**:
  - [x] Expanded unit tests from 220 to 238 (100% pass rate, 238/238 passing).
  - [x] 18 new tests covering user model, user repo, v1->v2 schema migration, and 10 isolation vectors.
  - [x] Smoke test suite: 5/5 passing.
  - [x] CTest: 100% passing.
  - [x] Frontend production build: `npm.cmd run build` passing in 5.83s.
  - [x] Live database migration and restart verification verified against `data/codevault.db`.

---

## Stage 9.3 — Authentication & Session Foundation (Completed & Verified)
- [x] **Vendored Argon2id Library**:
  - [x] Official reference implementation (`third_party/argon2/`) vendored into CMake build tree as static library target `argon2`.
  - [x] OWASP-recommended parameters: $t=2$ iterations, $m=19456$ KiB (19 MiB), $p=1$ thread, CSPRNG 16-byte salt, 32-byte hash.
- [x] **Cryptographic Utilities (`utils/crypto.hpp`, `src/utils/crypto.cpp`)**:
  - [x] `hashPassword` and `verifyPassword` using Argon2id.
  - [x] `generateSecureToken` using Windows CryptoAPI (`CryptGenRandom`) with fallback to `std::random_device`.
  - [x] `hashToken` using Blake2b 256-bit digest.
- [x] **Domain Models & Repositories**:
  - [x] `UserCredentials` entity decoupled from public `User` domain model.
  - [x] `Session` entity with expiration and revocation metadata.
  - [x] `ISessionRepository` and `SqliteSessionRepository` with prepared statements and foreign key cascade.
  - [x] `IUserRepository` updated with credential persistence (`saveCredentials`, `getCredentials`, `deleteCredentials`).
- [x] **Schema v3 Migration**:
  - [x] Created `user_credentials` and `sessions` tables with indexes on `token_hash` and `user_id`.
  - [x] Idempotent automatic migration from v1/v2 to v3 with 100% data and ownership preservation.
- [x] **AuthService (`services/auth_service.hpp`, `src/services/auth_service.cpp`)**:
  - [x] User registration with username, email, and password validation.
  - [x] User login with constant-time/generic failure responses preventing user enumeration.
  - [x] Token authentication with automatic `last_seen_at` updates.
  - [x] Server-side session revocation on logout.
- [x] **HttpServer Middleware & Cookies (`app/http_server.hpp`, `src/app/http_server.cpp`)**:
  - [x] Centralized authentication middleware resolving cookie -> session -> user -> `AuthenticatedCurrentUserProvider`.
  - [x] Endpoints: `POST /api/auth/register`, `POST /api/auth/login`, `POST /api/auth/logout`, `GET /api/auth/me`.
  - [x] `codevault_session` cookie issued with `HttpOnly`, `SameSite=Lax`, `Path=/`, `Max-Age=604800`.
  - [x] Dynamic CORS reflecting request `Origin` with `Access-Control-Allow-Credentials: true`.
  - [x] Public endpoints (`/api/auth/register`, `/api/auth/login`, `/api/settings/diagnostics`).
  - [x] All other `/api` endpoints protected with HTTP 401 Unauthorized if unauthenticated.
- [x] **CLI Isolation**:
  - [x] CLI continues operating in local single-user mode (`StaticCurrentUserProvider("local_user")`).
- [x] **Frontend Auth UI & API Client**:
  - [x] Updated `frontend/src/services/api.ts` with `credentials: 'include'` and 401 redirection event.
  - [x] Created `AuthContext` (`frontend/src/context/AuthContext.tsx`) for user state and session management.
  - [x] Created `LoginPage` and `RegisterPage` with CodeVault design tokens.
  - [x] Implemented `ProtectedRoute` wrapping application routes.
  - [x] Added user profile chip and sign out button in header and sidebar.
  - [x] Frontend production build passes cleanly (`tsc && vite build`).
- [x] **Testing & Validation**:
  - [x] Unit test suite expanded from 238 to 251 tests (251/251 passing, 100% pass rate).
  - [x] Smoke test suite: 5/5 passing.
  - [x] CTest suite: 2/2 suites passing (100%).
  - [x] Complete 18-step live API authentication flow verified end-to-end including server restart.

---

## Stage 9.4 — User Account Management, Active Sessions & Personal Data Export (Completed & Verified)
- [x] **User Profile Management**:
  - [x] Implemented `updateProfile` in `AuthService` (`include/services/auth_service.hpp`, `src/services/auth_service.cpp`).
  - [x] Validated display name and email format with duplicate email rejection across accounts.
  - [x] Endpoint: `PUT /api/auth/profile` updates profile and returns updated safe `User` entity.
- [x] **Password Management & Verification**:
  - [x] Implemented `changePassword` in `AuthService` verifying current password with Argon2id.
  - [x] Enforced $\ge 8$ character password complexity, generated fresh 16-byte CSPRNG salt, and saved new Argon2id hash.
  - [x] Endpoint: `POST /api/auth/change-password` rejects incorrect current passwords with generic error.
- [x] **Multi-Device Active Sessions Management**:
  - [x] Extended `ISessionRepository` and `SqliteSessionRepository` with `findActiveSessionsForUser` and `revokeAllForUserExcept`.
  - [x] Endpoint: `GET /api/auth/sessions` lists all active sessions, marking `isCurrentSession: true` without exposing raw tokens or token hashes.
  - [x] Endpoint: `POST /api/auth/sessions/:id/revoke` immediately revokes a specific device session in SQLite.
  - [x] Endpoint: `POST /api/auth/sessions/revoke-others` signs out all other devices while preserving the current active session.
- [x] **Personal Catalog Data Export & Backup**:
  - [x] Endpoint: `GET /api/user/export?format=json` exports the authenticated user's question catalog and metadata to structured JSON.
  - [x] Endpoint: `GET /api/user/export?format=csv` exports user's catalog as RFC 4180 CSV with standard 18 headers, attachment content disposition, and proper field escaping.
  - [x] Strict tenant isolation: exports never contain data belonging to other users.
- [x] **Frontend Settings Workspace**:
  - [x] Enhanced `SettingsPage.tsx` with clean tabs: Profile, Security & Password, Active Sessions, Data Export, and Engine Diagnostics.
  - [x] Added `updateProfile`, `changePassword`, `getSessions`, `revokeSession`, `revokeOtherSessions`, and `downloadExport` to `api.ts`.
  - [x] Real-time session revocation and "Sign Out Other Devices" action.
  - [x] Instant one-click file download for JSON and CSV backup formats.
- [x] **Testing & Verification**:
  - [x] Added 6 comprehensive test suites in `tests/unit/unit_tests.cpp` (Tests 252–257): profile updates, password change validation, multi-session listing, session revocation, JSON/CSV exports, and HTTP routes.
  - [x] Unit test suite expanded from 251 to 257 tests (257/257 passing, 100% pass rate).
  - [x] Smoke test suite: 5/5 passing.
  - [x] CTest: 2/2 suites passing (100%).
  - [x] Frontend production build: `npm.cmd run build` passing cleanly in 4.59s.
  - [x] Live end-to-end API verification confirmed via PowerShell script across profile, password, session revocation, and exports.

---

## Stage 9.5 — Catalog Import, Advanced Export & Data Portability (Completed & Verified)
- [x] **Phase 1: Backend Domain + Import/Export Services**:
  - [x] `ImportConflictStrategy` enum (`Skip`, `Overwrite`, `GenerateNewId`) with string parser and serializer.
  - [x] `ImportResult` domain model (`success`, `totalProcessed`, `importedCount`, `updatedCount`, `skippedCount`, `errors`).
  - [x] `ImportService` (`include/services/import_service.hpp`, `src/services/import_service.cpp`):
    - [x] RFC 4180 CSV parser handling multiline records, embedded commas, and quotes (`""`).
    - [x] JSON parser handling question entities, tags arrays, and status enums.
    - [x] Pre-validation pass verifying title, difficulty, URL formatting, and priority bounds before database mutation.
    - [x] 100% atomic transaction rollback using SQLite `BEGIN TRANSACTION` / `COMMIT` / `ROLLBACK`.
    - [x] Strict owner isolation: client cannot spoof `owner_id`; existing questions of other users can never be overwritten.
    - [x] Post-commit synchronization: `PrefixTrie` search indexes and `MinHeap` revision queues rebuilt only after commit succeeds.
  - [x] `ExportService` (`include/services/export_service.hpp`, `src/services/export_service.cpp`):
    - [x] Deterministic Markdown catalog generator with problem index table and topic/difficulty breakdown.
    - [x] Anki-compatible TSV flashcard generator with sanitized HTML `<br>` tags and 3-column layout.
    - [x] Shared CSV escaping utilities and JSON entity export helpers.
  - [x] 11 new unit tests (Tests 258–268) verifying import parsing, conflict strategies, owner isolation, rollback, and exports.
- [x] **Phase 2: HTTP API Integration & Route Protection**:
  - [x] Route: `POST /api/user/import?conflict_strategy=<strategy>` with `application/json` and `text/csv` support.
  - [x] Route: `GET /api/user/export/markdown` with `text/markdown; charset=utf-8` and `Content-Disposition`.
  - [x] Route: `GET /api/user/export/anki` with `text/tab-separated-values; charset=utf-8` and `Content-Disposition`.
  - [x] Strict session cookie authentication returning 401 Unauthorized for unauthenticated calls.
  - [x] Standardized error payload formatting and 400 Bad Request responses with atomic rollback.
  - [x] 8 new HTTP integration tests (Tests 269–276) verifying route protection, payload ingestion, conflict handling, and headers.
  - [x] 276 / 276 unit tests passing (100% pass rate).
- [x] **Phase 3: Frontend Data Portability Integration**:
  - [x] Added `ConflictStrategy` and `ImportResult` types to `frontend/src/types/index.ts`.
  - [x] Added `exportMarkdownCatalog()`, `exportAnkiCatalog()`, and `importCatalog()` to `frontend/src/services/api.ts`.
  - [x] Enhanced `SettingsPage.tsx` Tab 4 into unified **Data Portability** hub:
    - [x] 4 export format cards: JSON Entity Backup, RFC 4180 CSV, Markdown Catalog, and Anki TSV Flashcards.
    - [x] Interactive Import & Restore workspace with drag/drop file picker, client-side extension validation, and conflict strategy dropdown.
    - [x] Loading and disabled states preventing duplicate submissions.
    - [x] Real-time success summary pills (Processed, Imported, Updated, Skipped) and atomic rollback error list.
  - [x] Frontend production build passing (`tsc && vite build`) with 0 errors.
  - [x] Live end-to-end integration verified with Node.js test script across live backend process.
- [x] **Phase 4: Documentation Synchronization & Stage Sign-Off**:
  - [x] Synchronized `README.md` with Stage 9.5 features, endpoints, and test counts.
  - [x] Synchronized `TESTING.md` with 276-test suite breakdown and verification procedures.
  - [x] Synchronized `API_CONTRACT.md` with Stage 9.3/9.4 auth and Stage 9.5 data portability endpoint contracts.
  - [x] Recorded `ADR-022` in `DECISIONS.md`.
  - [x] Reviewed and updated `ARCHITECTURE.md`, `PROJECT_CONTEXT.md`, and `QUALITY_AND_HARDENING.md`.
  - [x] Completed full verification suite (276/276 unit tests, 5/5 smoke tests, 2/2 CTest suites, 0 compiler warnings/errors).

---

## Stage 10 — Advanced Practice Workflow & Practice Queue (Completed & Verified)
- [x] **Phase 1: Practice Domain/Service Enhancements**:
  - [x] Extended `dsa::Queue<T>` with non-destructive traversal (`toVector()`), membership testing (`contains()`), and element removal (`remove()`) supporting head, middle, tail, and missing cases.
  - [x] Extended `PracticeService` with queue introspection (`getQueueQuestionIds()`, `getQueueQuestions()`, `isQuestionQueued()`, `removeQuestionFromQueue()`).
  - [x] Implemented targeted practice session initialization with filters (`startSessionWithFilter()`) supporting topic, difficulty, status, company, and spaced revision due flags.
  - [x] Implemented single-question practice verdicts (`recordPracticeAttemptForQuestion()`) coordinating `RevisionService::markRevisionResult()`, `QuestionService::updateQuestion()`, and automatic unlinking from active queue.
  - [x] Implemented deterministic 5-tier recommendation engine (`getPracticeNext()`) prioritizing:
    1. Active practice queue head matching criteria
    2. Overdue spaced revisions from `MinHeap`
    3. Unsolved / never-practiced problems
    4. In-progress / attempted problems
    5. Stale solved/mastered retention reinforcement
  - [x] Enforced strict multi-user practice isolation boundaries across queues, verdicts, and recommendations.
  - [x] Added 6 unit tests (Tests 277–282) verifying queue mutations, filtering, verdicts, recommendation hierarchy, and tenant isolation.
- [x] **Phase 2: Practice HTTP/API Integration**:
  - [x] Implemented `GET /api/practice/next` supporting query parameter filtering (`topic`, `difficulty`, `includeDueRevisions`, `preferUnsolved`) with explainable rationale strings.
  - [x] Implemented `POST /api/practice/session` accepting JSON payload criteria for targeted practice session initialization.
  - [x] Implemented `GET /api/practice/queue` returning full question objects in FIFO sequence with count telemetry.
  - [x] Implemented `DELETE /api/practice/queue/:questionId` with 404 validation for missing questions or non-queued items.
  - [x] Implemented `POST /api/practice/:questionId/result` recording verdicts (`Solved`, `NeedsReview`, `Skipped`) with Leitner interval scheduling, status/timestamp updates, and queue unlinking.
  - [x] Centralized authentication middleware protection returning 401 Unauthorized for unauthenticated requests.
  - [x] Strict tenant isolation preventing cross-user queue inspection, removal, or verdict mutation.
  - [x] Added 7 HTTP integration tests (Tests 283–289) verifying route protection, recommendation cascades, queue lifecycle, verdicts, and cross-user isolation.
  - [x] Unit test suite expanded from 276 to 289 tests (289/289 passing, 100% pass rate).
- [x] **Phase 3: Frontend Practice Experience**:
  - [x] Added TypeScript interfaces in `frontend/src/types/index.ts`: `PracticeNextCriteria`, `PracticeNextResult`, `PracticeQueueResponse`, `PracticeQueueRemoveResult`, `PracticeSessionFilterPayload`, `SinglePracticeResultResponse`.
  - [x] Added API client methods in `frontend/src/services/api.ts`: `getPracticeNext`, `startFilteredPracticeSession`, `getPracticeQueue`, `removeFromPracticeQueue`, `recordPracticeResultForQuestion`.
  - [x] Built `PracticeQueueDrawer.tsx`: Slide-out FIFO inspection drawer with indexed badges, topic/difficulty tags, quick "Start Drill" CTA, and individual removal buttons.
  - [x] Built `StartSessionModal.tsx`: Modal dialog for configuring targeted drill sessions with topic, difficulty, status, and due-only filters.
  - [x] Enhanced `QuestionsPage.tsx`: Integrated "Practice Next" CTA with direct navigation, "Queue" button with real-time count badge, and "Drill Session" launcher.
  - [x] Enhanced `QuestionDetailPage.tsx`: Added Active Queue slot status card, one-click queue removal, quick verdict buttons (Mark Solved, Needs Review), Leitner SRS interval update display, "Practice Next Problem" transition CTA, and `N` keyboard shortcut.
  - [x] Enhanced `PracticePage.tsx`: Integrated queue drawer toggle and direct Practice Next fallback.
  - [x] Enhanced `DashboardPage.tsx`: Added "Recommended Next Problem" card with deterministic rationale and "Solve This Next" button.
  - [x] Enhanced `AppLayout.tsx`: Added dynamic practice queue count badge to sidebar Practice navigation item.
  - [x] Frontend production build passing cleanly (`tsc && vite build`) with 0 errors.
- [x] **Phase 4: Final Verification, UX Audit, Regression Testing & Documentation Synchronization**:
  - [x] Executed full regression suite: 289/289 backend unit tests passed, 5/5 smoke tests passed, 2/2 CTest suites passed.
  - [x] Verified frontend production build with 0 TypeScript/compilation errors.
  - [x] Executed live end-to-end integration test across 20 points against running server process with temporary database.
  - [x] Conducted UX and coding-platform visual audit for consistency, restrained styling, and accessibility.
  - [x] Synchronized all project documentation (`README.md`, `TODO.md`, `TESTING.md`, `API_CONTRACT.md`, `ARCHITECTURE.md`, `PROJECT_CONTEXT.md`, `DECISIONS.md`, `QUALITY_AND_HARDENING.md`).
  - [x] Verified zero documentation drift and clean change boundary.

---

## Stage 11 — Production Readiness, Hardening & Release Packaging (Current Stage)
- [x] **Phase 1: Production Readiness Audit & Contract/Implementation Reconciliation**:
  - [x] Audited implementation vs API contract, documentation, and frontend services.
  - [x] Reconciled practice queue semantics for `DELETE /api/practice/queue/:questionId` across all 5 states (present, absent, nonexistent, cross-user, already removed).
  - [x] Reconciled and refined "atomic" / transaction claims to accurately describe sequential verdict synchronization.
  - [x] Audited authentication, session lifecycle, and multi-user isolation (including imported `owner_id` escalation prevention).
  - [x] Verified frontend <-> backend contracts across all Stage 10 practice endpoints.
  - [x] Cleaned repository hygiene: updated `.gitignore` for `scratch/`, `.cache/`, and frontend build artifacts; purged temporary phase test artifacts; verified zero hardcoded paths/secrets.
  - [x] Verified test baseline: 289 unit tests, 5 smoke tests, 2 CTest suites, 0 TS errors.
- [x] **Phase 2: Runtime E2E, Frontend Hardening & Release Candidate Verification**:
  - [x] Refactored and stabilized live E2E PowerShell script (`test_live_e2e.ps1`), resolving all PSScriptAnalyzer notices (left-hand `$null` comparisons, utilized `$schedRes` assertions, piped JSON stream parameters).
  - [x] Executed live HTTP authentication & session lifecycle verification: registration, duplicate blocking, malformed payload rejection, invalid credentials, valid login, `/api/auth/me`, logout, and unauthenticated 401 protection across all protected routes.
  - [x] Executed live multi-user tenant isolation verification: cross-user read/update/delete rejection (404), isolated practice queues, cross-user verdict rejection (404), recommendation isolation, and owner_id spoofing prevention on import.
  - [x] Executed live import/export data portability verification: structured JSON export/import, conflict strategies (`skip`, `overwrite`, `generate_new_id`), atomic rollback on malformed JSON, RFC 4180 CSV export/import, Markdown export, and Anki 3-column TSV export.
  - [x] Executed live practice workflow verification: Practice Next 5-tier recommendation cascade, FIFO practice queue inspection, queue item removal, Solved & NeedsReview verdicts, SRS Leitner interval advancement, and queue unlinking.
  - [x] Executed server process restart persistence verification: account credentials and question data durability confirmed; transient practice queue reset confirmed (ADR-023).
  - [x] Hardened frontend runtime error handling: replaced render-phase navigation with declarative router `<Navigate>` in `LoginPage` and `RegisterPage`; added error boundary cards with retry buttons in `DashboardPage` and `StatisticsPage` to eliminate infinite spinner states on backend communication failures.
  - [x] Purged temporary `.bak` files from `data/sample/` and added `*.bak` to `.gitignore`.
  - [x] Verified complete release candidate test baseline: 289/289 backend unit tests passing, 5/5 smoke tests passing, 2/2 CTest suites passing, 49/49 live E2E assertions passing, frontend production build passing with 0 errors.
- [x] **Phase 3: Release Candidate Final Audit, Packaging & Feature Freeze (Completed)**:
  - [x] Hardened E2E test teardown and cleanup logic in `test_live_e2e.ps1` to prevent stray server processes or `.db-wal` / `.db-shm` / `.bak` artifacts.
  - [x] Executed repository artifact audit: confirmed absence of `.tmp`, `.bak`, `.log`, or stale test databases in root and data directories.
  - [x] Executed security and secret final sweep: zero hardcoded credentials, secret keys, or host-specific paths across `src/`, `include/`, `frontend/src/`, and scripts.
  - [x] Synchronized all project documentation (`README.md`, `TESTING.md`, `TODO.md`, `QUALITY_AND_HARDENING.md`, `PROJECT_CONTEXT.md`).
  - [x] Authored official `RELEASE_NOTES.md` documenting Release Candidate capabilities, architecture, testing, and limitations.
  - [x] Authored comprehensive `MANUAL_QA_CHECKLIST.md` providing a structured browser QA protocol across all 8 user workflows.
  - [x] Verified full build and test stack: backend build clean, 289 unit tests, 5 smoke tests, 2 CTest suites, 49 live HTTP E2E assertions, 0 frontend build errors.
---

## Stage 12 — Public Release & GitHub Readiness (Current Stage)
- [x] **Phase 1: Public Release & GitHub Readiness (Completed)**:
  - [x] Conducted full repository public-readiness audit from the perspective of an onboarding developer.
  - [x] Re-architected and polished `README.md`: structured into a clean, concise, production-grade guide with badges, architecture diagrams, technology stack, directory map, onboarding steps, test matrix, and documentation index.
  - [x] Emphasized pedagogical and authentic DSA visibility: explicit table documenting the exact responsibility, implementation type, and algorithmic complexity of all 8 core data structures/algorithms.
  - [x] Documented developer onboarding with clear separation between C++17 backend (CMake/Ninja) and React 18 frontend (Vite/TypeScript).
  - [x] Synchronized repository structure documentation against actual directory trees.
  - [x] Verified full build and test stack: backend build clean, 289 unit tests, 5 smoke tests, 2 CTest suites, 49 live HTTP E2E assertions, 0 frontend build errors.
- [x] **Phase 2: Portfolio Demo, Evidence & Presentation Readiness (Completed)**:
  - [x] Authored `DEMO_GUIDE.md`: 5–10 minute demonstration sequence, script, and viva talking points covering all 8 custom DSA structures with exact complexity bounds and file pointers.
  - [x] Authored `TECHNICAL_WALKTHROUGH.md`: Concise technical rationale for C++17, CMake, embedded SQLite v3, React/TypeScript, Argon2id security, deterministic recommendation cascades, and Leitner intervals.
  - [x] Authored `SCREENSHOT_PLAN.md`: Structured visual evidence capture plan covering 13 core views without fabricated data or broken links.
  - [x] Authored `DEMO_DATASET_PLAN.md`: Documented recommended 6-problem synthetic extension for full 14-topic dashboard demonstrations without modifying persistent storage.
  - [x] Authored `PROJECT_PORTFOLIO.md`: Professional project descriptions across one-line, short, and technical formats with key engineering metrics.
  - [x] Authored `PROJECT_SUMMARY.md`: Academic project summary detailing problem statement, objectives, DSA mapping, testing baseline, limitations, and future scope.
  - [x] Conducted verification language audit: ensured zero false claims across all documentation.
  - [x] Verified full build and test stack: backend build clean, 289 unit tests, 5 smoke tests, 2 CTest suites, 49 live HTTP E2E assertions, 0 frontend build errors.
- [x] **Phase 3: Final Manual QA Support, Repository Cleanup & Release Freeze (Completed)**:
  - [x] Audited `MANUAL_QA_CHECKLIST.md` against live routes, added dedicated `/statistics` verification step.
  - [x] Executed repository hygiene sweep: purged temporary sample backup file `data/sample/questions.csv.bak`, verified zero stray `.bak`, `.tmp`, `.log`, `.swp`, or test databases.
  - [x] Audited `.gitignore`: confirmed comprehensive coverage across `build/`, `node_modules/`, `dist/`, `.vite/`, `*.bak`, `*.log`, `scratch/`, `.cache/`, and local persistence.
  - [x] Conducted public safety scan: confirmed zero private keys, API secrets, hardcoded credentials, or host-specific paths across `src/`, `include/`, `frontend/src/`, and scripts.
  - [x] Synchronized documentation: verified precise testing terminology (289 unit tests, 5 smoke tests, 49 live HTTP E2E assertions, 2 CTest suites, Vite production build), in-memory transient practice queue behavior (ADR-023), and clear labeling of deferred future scope.
- [x] **Phase 4: Final Manual QA Execution & Release Evidence (Completed)**:
  - [x] Audited and expanded `MANUAL_QA_CHECKLIST.md` covering all 11 operational sections: Authentication, Problems (CRUD, search, filtering, sorting, pagination), Practice (queue, drills, verdicts, unlinking), Revision (Leitner 5-box), Dashboard, Statistics, Settings, Portability (import/export), Persistence (restart durability, transient queue reset), Edge Cases (404s, empty states, corrupt imports), and Visual Sanity.
  - [x] Validated 5–10 minute demonstration flow (`DEMO_GUIDE.md`) and screenshot evidence plan (`SCREENSHOT_PLAN.md`).
  - [x] Confirmed DSA viva evidence mapping across all 8 data structures (`TECHNICAL_WALKTHROUGH.md`, `DEMO_GUIDE.md`, `README.md`).
  - [x] Executed full automated verification baseline: 289 unit tests, 5 smoke tests, 2 CTest suites, 49 live HTTP E2E assertions, 0 frontend build errors, 0 compiler warnings.
  - [x] Confirmed repository hygiene: zero stray `.bak`, `.tmp`, or test database artifacts remaining in `data/`.
  - [x] Authored `FINAL_QA_HANDOFF.md` establishing release gates, recommended QA sequence, and explicit feature freeze.

---

## Post-QA First-Run UX Corrections (Completed & Verified)
- [x] **Login $\to$ Dashboard Default Landing**:
  - [x] Updated post-login redirection fallback to `/dashboard` in `LoginPage.tsx`.
  - [x] Preserved existing `location.state.from` return-to-route behavior for deep-linked protected navigation.
  - [x] Updated root index route in `App.tsx` and post-registration redirect to `/dashboard`.
- [x] **Clickable User Account Menu**:
  - [x] Converted top-right user chip in `AppLayout.tsx` into an accessible dropdown menu with avatar/initials, display name, `@username`, and email.
  - [x] Added menu options: Profile (`/settings?tab=profile`), Settings (`/settings`), Theme switcher, and Logout.
  - [x] Added click-outside dismissal (`useRef` + `mousedown` event listener) and keyboard dismissal (`Escape`).
  - [x] Added ARIA accessibility semantics (`aria-expanded`, `aria-haspopup="menu"`, `role="menu"`).
  - [x] Bound Logout action directly to existing `useAuth().logout()` without duplicate auth logic.
- [x] **Light / Dark / System Theme Support**:
  - [x] Implemented `ThemeContext.tsx` with `'light' | 'dark' | 'system'` modes.
  - [x] Enabled Tailwind `darkMode: 'class'` in `tailwind.config.js`.
  - [x] Added local persistence via `localStorage` under `codevault_theme`.
  - [x] Added inline `<script>` in `<head>` of `index.html` to eliminate flash of wrong theme on reload.
  - [x] Synchronized `'system'` mode with OS/browser `window.matchMedia('(prefers-color-scheme: dark)')`.
  - [x] Added theme selector controls in both the User Account Menu and Settings Profile tab.
  - [x] Applied consistent dark/light styling across sidebar, header, dashboard, problems table, badges, progress bars, and forms.
- [x] **Starter Catalog Deferral**:
  - [x] Explicitly deferred starter question catalog implementation pending independent legal review of dataset source and redistribution license.
- [x] **Verification Baseline Re-Confirmed**:
  - [x] Backend compilation: 0 errors, 0 warnings.
  - [x] 289 / 289 backend unit tests passed.
  - [x] 5 / 5 smoke tests passed.
  - [x] 2 / 2 CTest suites passed.
  - [x] 49 / 49 live HTTP E2E assertions passed.
  - [x] Frontend production build passed (`tsc && vite build`: 0 errors).

---

## Future Stages (Post-Authentication Horizon — NOT Started)
- [ ] Direct Online Judge API connectors (LeetCode, Codeforces)
- [ ] Contextual algorithmic hints and notes assistant
- [ ] OAuth / Google / GitHub SSO integration
- [ ] Email verification & password reset via email
- [ ] PostgreSQL / Cloud SQL persistence engine
- [ ] Redis session caching


