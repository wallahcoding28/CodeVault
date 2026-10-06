# CodeVault — Testing Strategy & Verification Plan

## 1. Quality Assurance Philosophy

CodeVault is engineered with testability as a primary requirement. Because it houses both critical algorithmic data structures and real-world question management services, defects can manifest either as memory corruption / incorrect pointer state in the DSA engine, or as logic errors in the application services.

Our multi-layered testing strategy guarantees deterministic validation across all tiers:
1. **Unit Testing**: Isolated verification of individual functions, comparators, and data model validations.
2. **DSA Engine Testing**: Invariant, boundary, and stress tests for custom data structures (`Trie`, `MinHeap`, `Queue`, `Stack`, `DoublyLinkedList`).
3. **Integration Testing**: End-to-end validation of workflows (e.g. adding a question $\to$ persisting to disk $\to$ indexing in Trie $\to$ retrieving via SearchService).
4. **Smoke Testing**: Fast, dependency-free sanity checks run after every build to confirm basic operational integrity.

---

## 2. Testing Levels & Matrix

| Tier | Target Scope | Execution Frequency | Tools / Framework |
| :--- | :--- | :--- | :--- |
| **Smoke Tests** | Application binary launch, repository load, shell greeting | Every commit & build | Custom C++ Smoke Runner (`smoke_test`, 5 checks) |
| **Stage 2 Core Unit Tests** | Domain models, validation rules, repository CRUD, disk persistence roundtrip, QuestionService | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 32 tests) |
| **Stage 3 Custom DSA Tests** | PrefixTrie, MinHeap, Queue, Stack, DoublyLinkedList & Service Integrations | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 51 tests) |
| **Stage 4 Search, Filter & Sort Tests** | Custom MergeSort, QuickSort, Prefix/Keyword Search, Multi-Criteria Filtering, Tie-Breaking, Workflows | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 39 tests) |
| **Stage 5 Spaced Revision & Practice Tests** | Deterministic intervals, MinHeap 3-key tie-breaks, due/upcoming logic, FIFO Practice Queue, verdicts, restart persistence | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 30 tests) |
| **Stage 6 Statistics & Dashboard Tests** | Zero-division protection, empty/single datasets, distributions, boundary percentages, revision/practice metrics, immutability, progress bars | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 18 tests) |
| **Stage 7 Hardening & Edge Case Tests** | Datetime epoch boundaries, non-existent/empty/corrupted CSV files, quote round-trip, duplicate IDs, DSA underflow exceptions, copy/move assignment, PrefixTrie long strings & punctuation, title length boundaries (255/256), priority limits, practice/revision boundaries | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 14 tests) |
| **Stage 7 End-to-End Integration Workflows** | Realistic multi-service workflows: Workflow A (Create -> Persist -> Reload -> Search -> Filter -> Sort), Workflow B (Create -> Practice -> Solved -> Schedule -> Persist -> Reload), Workflow C (Needs Review -> 1-Day Reset & Urgent Priority -> Persist -> Reload), Workflow D (Practice Queue Skip -> Solve -> Exit -> State & Persistence), Workflow E (Modify Question -> Dashboard Stats Update -> Source Immutability), Workflow F (Multi-Service Pipeline -> Restart Parity Consistency) | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 6 tests) |
| **Stage 8 Web Integration & JSON Tests** | JSON string escaping, Question entity serialization roundtrip, DashboardSnapshot completeness, RevisionItem & SessionProgress serialization, REST request body parsing, live HttpServer REST endpoints (/api/dashboard, /api/questions, /api/practice, /api/revision, /api/settings/diagnostics) | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 10 tests) |
| **Stage 9 SQLite Persistence Tests** | Embedded SQLite 3 engine, schema v1, WAL mode, ACID transactions, atomic rollback, CSV-to-SQLite migration, corruption tolerance | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 20 tests) |
| **Stage 9.2 Multi-User Isolation Tests** | User domain model, user repo CRUD, schema v2 migration, owner-scoped queries, cross-user isolation across all services | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 18 tests) |
| **Stage 9.3 Authentication & Session Tests** | Argon2id password hashing, CSPRNG tokens, Blake2b digest, sessions table, cookie auth middleware, 401 route protection | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 13 tests) |
| **Stage 9.4 Account & Export Tests** | Profile updates, password modification, active sessions listing, device revocation, owner-scoped JSON and CSV data exports | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 6 tests) |
| **Stage 9.5 Import & Export Services Tests** | Conflict strategies (skip, overwrite, generate_new_id), atomic rollback, owner-id isolation, PrefixTrie/MinHeap sync, Markdown/Anki exports | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 11 tests) |
| **Stage 9.5 HTTP API & Route Protection Tests** | POST /api/user/import, GET /api/user/export/markdown, GET /api/user/export/anki, session cookie validation, 401 rejection, Content-Disposition headers | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 8 tests) |
| **Stage 10 Practice Queue & Domain Tests (Phase 1)** | dsa::Queue mutations (toVector, contains, remove), PracticeService queue inspection, filtered session init, verdicts & unlinking, deterministic cascade, multi-user isolation | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 6 tests) |
| **Stage 10 HTTP API & Integration Tests (Phase 2)** | GET /api/practice/next, POST /api/practice/session, GET/DELETE /api/practice/queue, POST /api/practice/:id/result, 401 route protection, cross-user isolation | Every commit & build | CTest / Unit Test Runner (`unit_tests`, 7 tests) |
| **Total Test Count** | **Comprehensive Automated Verification** | **Continuous** | **289 Unit Tests + 5 Smoke Checks = 294 Tests Passing (100% Pass Rate)** |

---

## 3. Implemented Test Scenarios Breakdown

### 3.1 Domain Model & Business Validation (14 tests)
- Constructor, getters, and mutability setters
- Strict validation rules: empty ID/title, ID character whitelist, unknown difficulty, invalid URLs, priority range limits
- Enum conversions and case-insensitive parsing

### 3.2 Repository & Local Disk Roundtrip (10 tests)
- In-memory CRUD operations (`exists`, `findById`, `findAll`, `save`, `remove`)
- Disk persistence roundtrip across instances with RFC-4180 delimiter escaping
- Boundary resilience: empty files, missing files, corrupted CSV lines

### 3.3 QuestionService Orchestration (8 tests)
- Entity creation, ID duplicate guards, validation blocking
- Question update with non-existent ID rejection
- Deterministic ID auto-generation (`Q-1001`, `Q-1010`)
- Aggregation counters by difficulty and status
- Deletion verification

### 3.4 Custom PrefixTrie Engine (12 tests)
- Empty state and contains/startsWith invariants
- Case-insensitive normalization (`"Merge Sort"` vs `"merge sort"`)
- Special characters, numbers, parentheses, and spaces
- Prefix search and deterministic sorted autocomplete output
- Leaf pruning and recursive node cleanup upon word removal
- Rule of 5 deep copy validation

### 3.5 Custom MinHeap Engine (8 tests)
- Underflow exception guarantees (`std::underflow_error` on empty `top()` and `pop()`)
- Single and multi-element insertion preserving minimum at root
- Pop sequence strictly ordered in ascending order
- Duplicate priority handling without infinite loops
- Range-based constructor (`buildHeap`)
- Custom `RevisionItemComparator` (timestamp primary, priority urgency tie-breaker)

### 3.6 Custom Queue Engine (6 tests)
- Underflow exception guarantees on empty `front()`, `back()`, `dequeue()`
- Strict FIFO ordering
- Large batch stress test (500 elements enqueued and dequeued)
- Rule of 5 deep copy and move semantics

### 3.7 Custom Stack Engine (5 tests)
- Underflow exception guarantees on empty `top()`, `pop()`
- Strict LIFO ordering
- Rule of 5 deep copy and move semantics

### 3.8 Custom DoublyLinkedList Engine (12 tests)
- Underflow guarantees on empty `front()`, `back()`, `popFront()`, `popBack()`
- $O(1)$ front and back insertions and removals
- Index-based middle insertion and deletion
- Value-based item removal
- Bidirectional forward and reverse traversals
- Range-based `for` loop compatibility via bidirectional iterators
- Rule of 5 deep copy and move semantics

### 3.9 DSA Service Integration Tests (8 tests)
- Automatic Trie indexing on QuestionService ingestion
- Sub-millisecond prefix search resolving to full Question domain models
- Dynamic index synchronization on question title modification and deletion
- RevisionService MinHeap priority scheduling and extraction
- PracticeService Queue FIFO completion and skip-to-back problem cycling
- RecentHistoryService Stack tracking with consecutive duplicate suppression
- ProblemPlaylistService DoublyLinkedList bidirectional navigation and in-place deletion

### 3.10 Custom Sorting Algorithms (12 tests)
- MergeSort: Ascending, descending, duplicate keys, already sorted, reverse sorted, empty range, single element
- MergeSort: Mathematical stability guarantee verified on (Key, InsertionIndex) pairs
- QuickSort: Ascending, descending, duplicate keys, already sorted, reverse sorted, empty range, single element
- QuickSort: In-place median-of-three Lomuto partitioning on positive and negative elements

### 3.11 SearchService Prefix & Keyword Tests (8 tests)
- Exact and prefix matching via PrefixTrie
- Case-insensitive autocomplete suggestions
- Non-matching prefix handling
- Empty search string returning all questions
- Dynamic synchronization: removed question disappears from Trie; edited title updates prefix suggestions
- Multi-field keyword matching across title, description, tags, and company

### 3.12 Multi-Criteria Filtering (8 tests)
- Individual criteria filters: Topic, Difficulty, Status, Company substring, Favorites only / Non-favorites only
- Multi-criteria conjunction: Topic + Difficulty + Status simultaneously
- Non-matching combination returning empty result set
- Empty criteria specification returning full catalog

### 3.13 Question Sorting & Tie-Breaking (8 tests)
- Title ascending & descending
- Difficulty ascending ($1 \to 3$) and descending ($3 \to 1$)
- Topic alphabetical ordering
- Company alphabetical ordering
- Status workflow ordering ($Unsolved \to InProgress \to Solved \to Mastered$)
- Deterministic secondary tie-breaking by Question ID
- Algorithm parity: MergeSort and QuickSort producing identical sorted question vectors

### 3.14 Combined Workflows & Repository Safety (3 tests)
- Keyword search $\to$ Topic filter $\to$ Difficulty sort
- Company & Difficulty filter $\to$ Title sort
- Repository and catalog immutability: views and sorts operate on isolated copies, preserving persistent storage

### 3.15 Spaced Revision Schedule & Progression (10 tests)
- Deterministic interval calculation for Levels 1–5 (1, 3, 7, 14, 30 days)
- Initial progression for unpracticed questions upon `Solved`
- Multi-step sequential progression across intervals ($1 \to 3 \to 7 \to 14 \to 30$ days)
- Level 5 capping at 30 days and automatic status promotion to `Mastered`
- `NeedsReview` verdict resetting interval to 1 day and setting urgency to Priority 1
- `Skipped` verdict preserving question state, timestamps, and status intact

### 3.16 MinHeap Ordering, Tie-Breaking & Due Queries (9 tests)
- Primary ordering by earliest `nextRevisionAt` timestamp
- Secondary tie-breaking by urgency priority ($1 = \text{Urgent}$ before $5$)
- Tertiary deterministic tie-breaking by question ID
- Due question detection ($\text{next\_revision\_at} \le \text{now}$)
- Future question exclusion ($\text{next\_revision\_at} > \text{now}$)
- Unscheduled question exclusion ($\text{next\_revision\_at} \le 0$)
- Upcoming revision extraction in ascending chronological order
- Rescheduling replacing existing heap entries without duplicate accumulation
- Removal from MinHeap queue upon question deletion

### 3.17 Practice Session Queue Mechanics (6 tests)
- Empty practice session invariants
- Strict FIFO enqueue and dequeue order preservation
- Skip operation cycling front question to back of queue in $O(1)$
- Single-item queue skip boundary handling
- Session progress tracking (total, completed, remaining, skipped)
- Filtered practice session creation (unsolved, topic, difficulty, favorites)

### 3.18 Service Coordination & Practice Verdict Processing (3 tests)
- `recordPracticeAttempt(Solved)` updating domain state and advancing queue
- `recordPracticeAttempt(NeedsReview)` setting 1-day interval and priority 1
- `recordPracticeAttempt(Skipped)` preserving entity state and cycling queue

### 3.19 Persistence Across Restarts & End-to-End Workflow (2 tests)
- Revision & practice state surviving complete process restart via CSV
- Full end-to-end integration workflow (Create $\to$ Schedule $\to$ Due $\to$ Solved $\to$ Persist $\to$ Reload)

### 3.20 Statistics & Dashboard Subsystem (18 tests)
- Empty dataset handling and zero-division protection across all percentages (0.0%)
- Single question dataset calculation accuracy
- All questions solved (100.0% completion)
- All questions unsolved (0.0% completion)
- Mixed statuses (Unsolved, InProgress, Solved, Mastered)
- Difficulty distribution counts and mathematical percentage integrity
- Topic distribution counts, distinct topic counters, and lexicographical topic sorting
- Favorite question count and favorite percentage calculation
- Revision due vs upcoming counts relative to reference timestamps
- Revision priority distribution across urgency buckets (Priority 1 through 5)
- Revision level distribution across interval stages (Level 1 through 5)
- Practice history metrics: practiced count, unpracticed count, latest practice epoch
- Mastered status inclusion in overall completion percentage: `(solved + mastered) / total * 100`
- Boundary percentage handling (0%, 100%, single questions)
- Repeated calculation determinism (idempotent calculations on constant data)
- Collection immutability: verifying statistics computations never mutate underlying question data
- Live service integration: end-to-end `StatisticsService` coordinating `QuestionService` and `RevisionService`
- Terminal progress bar rendering: clamping ($<0\%$ and $>100\%$), width proportionality, and character layout

### 3.21 Hardening, Edge Cases & Robustness (14 tests)
- `datetime.hpp` timestamp and date boundary formatting (epoch 0, negative values, valid timestamps)
- Persistence resilience: non-existent file path loads count 0 safely
- Persistence resilience: empty CSV file and header-only CSV file load with 0 records without errors
- Persistence resilience: corrupted CSV lines (missing columns, empty ID, non-numeric timestamps/priority) skip gracefully
- Persistence roundtrip: RFC-4180 complex quoted fields with commas, internal quotes (`""`), and semicolons
- Persistence deduplication: duplicate IDs preserve a single entry with deterministic latest update
- Custom `DoublyLinkedList`: copy/move assignment correctness, `front()`, `back()`, `popFront()`, `popBack()` empty underflow exceptions, out-of-range `removeAt()` exception
- Custom `Stack`: copy/move assignment correctness, `top()`, `pop()` empty underflow exceptions
- Custom `Queue`: copy/move assignment correctness, `front()`, `back()`, `dequeue()` empty underflow exceptions
- Custom `MinHeap`: copy/move assignment correctness, `top()`, `pop()`, `extractMin()` underflow exceptions, multi-element identical key stability
- Custom `PrefixTrie`: empty string insert/contains/startsWith/remove invariants, 300-character long strings, punctuation/hyphenated keys
- Domain validation boundaries: exact 255 character title acceptance, 256 character title rejection, whitespace rejection, invalid ID rejection, priority range (1 to 5) limits
- Revision scheduling boundaries: exact due timestamp (`nextRevisionAt == now`) is due, future timestamp is upcoming, unscheduled (`nextRevisionAt <= 0`) excluded
- Practice session boundaries: empty session operations fail safely, single-item session skip prevention

### 3.22 Stage 7 End-to-End Integration Workflows (6 tests)
- **Workflow A**: Create 4 questions $\to$ persist to disk $\to$ reload fresh repository $\to$ prefix search $\to$ keyword search $\to$ multi-field topic filtering $\to$ difficulty descending sorting.
- **Workflow B**: Create question $\to$ start practice session $\to$ mark solved $\to$ verify automatic level 1 (+1 day) scheduling $\to$ persist $\to$ reload fresh repository $\to$ verify upcoming $\to$ advance clock past due timestamp $\to$ verify due detection.
- **Workflow C**: Question previously at level 4 (+14 days) $\to$ practice verdict NeedsReview $\to$ verify interval reset to 1 day (+86400s) $\to$ verify priority escalated to 1 (Urgent) $\to$ status set to InProgress $\to$ persist $\to$ reload fresh repository $\to$ verify top-priority due state.
- **Workflow D**: Multi-question practice session $\{Q_1, Q_2, Q_3\}$ $\to$ skip $Q_1$ (cycles to back) $\to$ solve $Q_2$ (dequeued) $\to$ inspect remaining $\{Q_3, Q_1\}$ $\to$ exit early $\to$ verify session summary counts $\to$ verify persistence (only $Q_2$ marked Solved).
- **Workflow E**: Modify question in collection $\to$ re-compute dashboard snapshot $\to$ verify updated completion and solved percentages $\to$ assert original vector attributes were not mutated by statistics calculations.
- **Workflow F**: Full multi-service pipeline (search $\to$ filter $\to$ sort $\to$ dashboard snapshot $\to$ revision due query) executed across an application restart $\to$ assert exact tuple parity between pre-restart and post-restart pipeline outcomes.

### 3.23 Stage 8 Web Integration & JSON Tests (10 tests)
- **JSON String Escaping**: Verifies proper escape sequences for quotes (`\"`), backslashes (`\\`), tabs (`\t`), newlines (`\n`), and control characters.
- **Question Entity Serialization Roundtrip**: Full serialization of `Question` model into JSON and deserialization back into entity fields.
- **DashboardSnapshot Completeness**: Serializes composite snapshot object containing overall, difficulty, topic, status, revision, and practice metrics without missing sub-objects.
- **RevisionItem & SessionProgress Serialization**: Formats priority queue and FIFO session metrics into compliant JSON payloads.
- **REST Request Body Parser**: Extracts strings, booleans, integers, and string arrays with error detection for malformed payloads.
- **HTTP API - /api/dashboard**: Verifies GET request generates correct JSON with 200 OK and accurate calculated percentages.
- **HTTP API - /api/questions**: Verifies GET query filtering (topic, difficulty, status, company, favorite), prefix search, and keyword search.
- **HTTP API - /api/practice**: Verifies POST `/api/practice/start`, GET `/api/practice/current`, and POST `/api/practice/verdict` transitions.
- **HTTP API - /api/revision**: Verifies GET `/api/revision/due` and POST `/api/revision/schedule` operations.
- **HTTP API - /api/history & /api/settings/diagnostics**: Verifies LIFO inspection history and engine invariant diagnostic responses.

### 3.24 Stage 9 SQLite Persistence, Transactions & Migration Tests (20 tests, Tests 201–220)
- **SQLite Engine Initialization**: File creation, WAL mode configuration, schema table initialization, and index verification.
- **Full CRUD & Entity Invariants**: All 18 domain attributes persisted and loaded with zero field truncation or data type distortion.
- **ACID Transactions**: Atomic commits with `executeTransaction`; automatic rollback on errors leaving database state unmodified.
- **CSV Ingestion Migration**: Automatic migration from `data/questions.csv` into SQLite, backup file generation (`.csv.bak`), and re-run idempotency.
- **Service Integration**: Verification of `QuestionService`, `RevisionService`, and `PracticeService` operating on SQLite persistence.

### 3.25 Stage 9.2 Multi-User & Owner Isolation Tests (18 tests, Tests 221–238)
- **User Domain Model**: User validation, getters, defaults, and deactivation.
- **User Repository**: User creation, username uniqueness constraints, email lookups, and profile mutations.
- **Schema v2 Migration**: Automatic table creation (`users`), foreign key relationship (`questions.owner_id -> users.id`), and `local_user` seeding.
- **Multi-Tenant Isolation**: Strict cross-user barrier testing: User A cannot read, mutate, delete, practice, schedule, or search questions owned by User B.
- **Dynamic Scoping**: Verifying that active user switching dynamically repoints in-memory Trie and MinHeap indexes.

### 3.26 Stage 9.3 Authentication & Session Foundation Tests (13 tests, Tests 239–251)
- **Argon2id Cryptographic Security**: Memory-hard password hashing and constant-time verification with CSPRNG salts.
- **Session Tokens**: 256-bit CSPRNG token generation and Blake2b digest hashing (`token_hash`).
- **Auth Service & Lifecycle**: Registration, login, session token validation, and logout revocation.
- **HTTP Route Protection**: Unauthenticated requests to protected endpoints strictly return HTTP 401 Unauthorized.
- **Zero Credential Leakage**: Asserting password hashes and raw session tokens are never emitted in API responses or logs.

### 3.27 Stage 9.4 User Account Management, Active Sessions & Personal Data Export (6 tests, Tests 252–257)
- **Profile Updates**: Display name and email updates with email format checks and cross-account uniqueness enforcement.
- **Password Modification**: Secure password change requiring Argon2id verification of current password and fresh salt re-hashing.
- **Active Sessions Management**: Multi-device session enumeration (`isCurrentSession` calculation) and selective revocation (`revokeSession`, `revokeAllForUserExcept`).
- **Personal Catalog Export**: Owner-scoped JSON and RFC 4180 CSV generation with download headers.

### 3.28 Stage 9.5 Catalog Import, Advanced Export & Backend Services (11 tests, Tests 258–268)
- **Conflict Strategy Model**: String parsing, validation, and serialization across `skip`, `overwrite`, and `generate_new_id`.
- **JSON Batch Ingestion**: Full structured catalog parsing, entity validation, and conflict resolution.
- **RFC 4180 CSV Import**: Multiline text parsing, escaped quotes (`""`), embedded commas, and column mapping.
- **Atomic Batch Rollback**: Guaranteed zero partial writes on parse or validation failure; database transaction cleanly rolls back.
- **Owner-Scoped Protection**: Ingestion overrides incoming `owner_id` with verified authenticated user ID; protects existing questions of other accounts against overwrite.
- **In-Memory DSA Synchronization**: Post-commit rebuild of `PrefixTrie` search indexes and `MinHeap` revision queues.
- **Markdown & Anki TSV Exports**: Deterministic Markdown problem catalog and Anki flashcard TSV generation with sanitized HTML tags.

### 3.29 Stage 9.5 HTTP API Integration & Route Protection (8 tests, Tests 269–276)
- **Route Protection**: HTTP 401 Unauthorized returned for unauthenticated calls to `/api/user/import`, `/api/user/export/markdown`, and `/api/user/export/anki`.
- **Content-Type Dispatch**: Automated format resolution from `application/json` and `text/csv` headers.
- **Conflict Strategy Handling**: Query parameter negotiation (`?conflict_strategy=skip|overwrite|generate_new_id`) and rejection of invalid values.
- **HTTP Export Headers**: Verification of `Content-Disposition`, attachment filenames (`codevault-questions.md`, `codevault-questions-anki.tsv`), and MIME types.
- **Malformed Payloads & Error Payloads**: HTTP 400 Bad Request responses containing structured `errors: [...]` arrays and atomic rollback.

### 3.30 Stage 9.5 Frontend Build & Live End-to-End Verification
- **Production Build**: Clean `tsc && vite build` passing with 0 TypeScript diagnostics and optimized bundle chunks.
- **Live Process Verification**: End-to-end integration verified against running `codevault.exe --server` validating authentication, question seeding, Markdown export, Anki export, JSON import (skip/overwrite), and CSV import.

### 3.31 Stage 10 Practice Queue & Domain Engine (6 tests, Tests 277–282)
- **`dsa::Queue` Invariant Extensions (Test 277)**: Validates non-destructive conversion to `std::vector`, `contains()` element lookup, and `remove()` mutation handling head node deletion, middle node splicing, tail node deletion with tail pointer update, and missing value queries.
- **`PracticeService` Queue Introspection & Unlinking (Test 278)**: Validates FIFO queue initialization from Question IDs, non-destructive question model hydration (`getQueueQuestions()`), membership queries (`isQuestionQueued()`), and individual removal (`removeQuestionFromQueue()`).
- **Targeted Practice Sessions (Test 279)**: Validates `startSessionWithFilter()` across topic, difficulty, and status criteria, plus `dueOnly` filtering that strictly loads questions currently due in Leitner spaced repetition.
- **Single-Question Practice Verdicts (Test 280)**: Validates `recordPracticeAttemptForQuestion()` across `Solved`, `NeedsReview`, and `Skipped` verdicts. Confirms Leitner interval computation via `RevisionService`, question model timestamp/status mutation via `QuestionService`, and automatic unlinking from active queue.
- **Deterministic "Practice Next" Cascade (Test 281)**: Exhaustively verifies the 5-tier recommendation hierarchy: (1) Active queue head, (2) Overdue revisions from MinHeap, (3) Unsolved never-practiced problems, (4) In-progress problems, (5) Stale solved/mastered problems. Verifies secondary tie-breaking by priority/timestamp/ID and explainable recommendation rationale string.
- **Practice Multi-User Boundary Enforcement (Test 282)**: Validates that active practice queues, verdict processing, and "Practice Next" recommendations never leak questions across user boundaries when `setCurrentUserProvider` switches tenants.

### 3.32 Stage 10 Practice HTTP API Integration & Route Protection (7 tests, Tests 283–289)
- **Route Protection (Test 283)**: Verifies that unauthenticated requests to `/api/practice/next`, `/api/practice/session`, `/api/practice/queue`, and `/api/practice/:id/result` strictly return HTTP 401 Unauthorized.
- **GET `/api/practice/next` Recommendation (Test 284)**: Verifies deterministic recommendation response formatting, including `hasQuestion`, `question` payload, and explainable `recommendationReason`.
- **GET `/api/practice/next` Filter Negotiation (Test 285)**: Verifies query parameter handling (`topic`, `difficulty`, `includeDueRevisions`, `preferUnsolved`).
- **POST `/api/practice/session` Targeted Initialization (Test 286)**: Verifies JSON criteria parsing (`topic`, `difficulty`, `status`, `company`, `dueOnly`) and returns updated `SessionProgress`.
- **GET & DELETE `/api/practice/queue` (Test 287)**: Verifies queue listing (`queue: [...]`, `count: N`) and individual item deletion with 404 validation when question is missing or not queued.
- **POST `/api/practice/:questionId/result` (Test 288)**: Verifies single-question verdict dispatching (`Solved`, `NeedsReview`, `Skipped`), Leitner SRS interval update, status updates, and automatic queue unlinking.
- **Multi-User Security Enforcement (Test 289)**: Verifies that authenticated User B cannot inspect User A's queue, cannot delete questions from User A's queue, cannot record verdicts for User A's questions, and never receives User A's questions in recommendation endpoints.

### 3.33 Stage 10 Frontend Build & Live End-to-End Verification
- **Frontend Production Build**: Clean `tsc && vite build` passing with 0 TypeScript diagnostics, 0 compiler warnings, and optimized chunks.
- **Live Server E2E Verification**: 20/20 test assertions passed against running `codevault.exe --server` on a temporary SQLite database, verifying unauthenticated 401, registration, question seeding, Practice Next recommendation, targeted session startup, queue inspection, item removal, verdict submission, SRS interval advancement, queue unlinking, multi-user isolation, and clean server teardown.

### 3.34 Stage 11 Live Runtime E2E & Release Candidate Verification Suite (49 assertions)
- **PowerShell Analyzer Compliance**: Resolved all PSScriptAnalyzer notices (left-hand `$null` checks, meaningful `$schedRes` assertions, piped JSON stream parameters).
- **Authentication & Sessions (Section 1)**: Verified unauthenticated 401 rejection, user registration, authenticated session establishment via `codevault_session` cookie, absence of leaked credentials in `/api/auth/me`, duplicate username rejection (400), malformed payload rejection (400), invalid password failure (401), non-existent user failure (401), and valid login credentials refresh.
- **Practice & Workflow Integration (Section 2)**: Verified question seeding, revision schedule explicitly confirmed, deterministic Practice Next due revision recommendation, targeted session startup, FIFO queue inspection, item removal, 404 on already-removed item, Solved verdict status/timestamp updates, queue unlinking, NeedsReview urgent priority promotion, 400 on invalid verdict, and 404 on nonexistent question verdict.
- **Multi-User Tenant Isolation (Section 3)**: Verified that User 2 cannot read (404), modify (404), or delete (404) User 1's questions; User 2's practice queue is isolated; User 2 cannot remove User 1's queue items (404); User 2 cannot record verdicts for User 1's questions (404); and User 2 Practice Next returns no recommendation across tenant boundary.
- **Import / Export Data Portability (Section 4)**: Verified User 1 JSON export with all owned questions, empty catalog export returning 0 questions, deterministic Markdown export with table and problem details, Anki TSV export with exactly 3 tab-separated columns, User 2 JSON import with `generate_new_id` resolving owner isolation (all imported records mapped to User 2), safe rejection of malformed JSON with 400 Bad Request, and RFC 4180 CSV export roundtrip.
- **Logout & Invalidation (Section 5)**: Verified session revocation on logout, 401 on `/api/auth/me` with invalidated cookie, and 401 on `/api/questions` after logout.
- **Process Restart & Durability (Section 6)**: Verified durable credential persistence across process restart, question catalog persistence, status/timestamp preservation, transient practice queue reset to 0 (ADR-023), and User 2 imported question preservation.

---

## 4. How to Execute Tests

### 4.1 Running the Smoke Test
```bash
./build/bin/smoke_test data/sample/questions.csv
```

### 4.2 Running Full Test Suite via CMake / CTest
```bash
# Configure and build
cmake -B build -S .
cmake --build build

# Execute unified test suite
ctest --test-dir build --output-on-failure

# Alternatively, run unit test executable directly for verbose breakdown:
./build/bin/unit_tests
```

### 4.3 Running Live Runtime E2E Suite (PowerShell)
```powershell
powershell -ExecutionPolicy Bypass -File .\test_live_e2e.ps1
```
Executes 49 automated assertions against an active HTTP server instance covering authentication, session lifecycle, owner-isolation, import/export portability, practice queues, Leitner verdicts, and server restart persistence.
