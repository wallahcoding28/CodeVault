# CodeVault

> **High-Performance Smart Coding Question Management & Spaced Practice Platform**  
> *A unified desktop workspace for software engineers and competitive programmers to organize, search, revise, and master algorithmic problems with transparent custom Data Structures & Algorithms (DSA).*

[![Standard: C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build: CMake](https://img.shields.io/badge/Build-CMake-orange.svg)](https://cmake.org/)
[![Database: SQLite 3](https://img.shields.io/badge/Database-SQLite%203%20(Schema%20v3)-003B57.svg)](https://www.sqlite.org/)
[![Frontend: React 18 + Vite](https://img.shields.io/badge/Frontend-React%2018%20%7C%20TypeScript-61DAFB.svg)](https://react.dev/)
[![Status: Release Candidate](https://img.shields.io/badge/Status-Release%20Candidate-brightgreen.svg)](RELEASE_NOTES.md)
[![License: MIT](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

---

## 1. Problem Statement

Aspiring software engineers preparing for technical interviews, campus placements, and competitive programming face critical organizational friction:
- **Scattered Catalogs**: Problems solved across LeetCode, Codeforces, HackerRank, and GeeksforGeeks are rarely unified in one place.
- **Inadequate Spaced Repetition**: Retention of algorithmic patterns degrades rapidly without disciplined, spaced revision intervals.
- **Black-Box Tools**: Standard spreadsheets or generic trackers disconnect learners from the foundational Data Structures & Algorithms (DSA) they are actively studying.

**CodeVault** solves this by providing a unified, local-first platform where genuine, educational data structures (Tries, Min-Heaps, Queues, Hash Maps, Doubly Linked Lists) power the search, scheduling, and revision workflows.

---

## 2. Key Features

- **Multi-Platform Problem Catalog**: Unified problem CRUD with difficulty levels, topics, target companies, platforms, URLs, notes, and favorite flags.
- **Sub-Millisecond Prefix Search**: Custom `PrefixTrie` provides instant autocomplete across problem titles and tags.
- **Algorithmic Spaced Repetition (SRS)**: Leitner 5-box spaced intervals (1, 3, 7, 14, 30 days) prioritized in $O(\log N)$ by a custom `MinHeap`.
- **Deterministic "Practice Next" Cascade**: 5-tier rule-based problem recommendation cascade explaining exactly why each problem was selected (100% deterministic, zero LLM dependencies).
- **FIFO Daily Practice Queue**: In-memory `Queue<T>` with non-destructive introspection and single-question verdict unlinking.
- **Secure Multi-User Tenant Isolation**: Argon2id password hashing, Blake2b-256 session token digests, `HttpOnly` session cookies, and strict per-user catalog isolation.
- **Catalog Portability & Ingestion**: Transactional batch import/export supporting structured JSON, RFC 4180 CSV, formatted Markdown, and Anki-compatible 3-column TSV flashcard decks.
- **Dual Presentation Interfaces**: Native interactive terminal CLI (`codevault --cli`) and modern desktop Web UI (React 18, Vite, TypeScript, Tailwind CSS) featuring default post-login Dashboard navigation, accessible user account menu, and full Light / Dark / System theme support.

---

## 3. Data Structures & Algorithms (DSA) Architecture

In CodeVault, every data structure has a genuine, defensible product responsibility. Data structures are never decorative:

| Data Structure | Implementation Type | Product Responsibility in CodeVault | Computational Complexity |
| :--- | :--- | :--- | :--- |
| **Prefix Trie** | Custom `PrefixTrie` (`include/dsa/trie.hpp`) | Instant autocomplete and prefix matching on problem titles and tag keywords. | $O(L)$ where $L$ is key length |
| **Min-Heap** | Custom `MinHeap<T>` (`include/dsa/min_heap.hpp`) | Spaced revision scheduler extracting the earliest scheduled problem with 3-key tie-breaking (`next_revision_at`, `priority`, `id`). | $O(\log N)$ insert / extraction |
| **FIFO Queue** | Custom Linked `Queue<T>` (`include/dsa/queue.hpp`) | Sequences problems queued for active daily practice drill sessions; supports targeted $O(N)$ node unlinking on verdict submission. | $O(1)$ push / pop |
| **LIFO Stack** | Custom Linked `Stack<T>` (`include/dsa/stack.hpp`) | Navigation history tracking recently inspected problems. | $O(1)$ push / pop |
| **Doubly Linked List** | Custom `DoublyLinkedList<T>` (`include/dsa/doubly_linked_list.hpp`) | Problem playlist walkthrough with bidirectional step navigation and $O(1)$ node-level splicing. | $O(1)$ insertion / removal |
| **Hash Map** | `std::unordered_map` | Fast multi-index lookups by ID, topic, company, and platform. | Average $O(1)$ lookup |
| **Merge Sort** | Custom `mergeSort` (`include/dsa/sorting.hpp`) | Stable recursive divide-and-conquer sorting by title, difficulty, company, or practice date. | $O(N \log N)$ worst-case |
| **Quick Sort** | Custom `quickSort` (`include/dsa/sorting.hpp`) | In-place sorting with median-of-three pivot selection and tail-call optimization. | Average $O(N \log N)$ |

---

## 4. System Architecture

CodeVault follows strict Clean Architecture boundaries:

```
┌────────────────────────────────────────────────────────────────────────┐
│                          Presentation Layer                            │
│    (Interactive CLI Shell)          (React 18 + TypeScript Web UI)     │
└──────────────────┬─────────────────────────────────┬───────────────────┘
                   │                                 │ (HTTP REST / JSON)
                   │                    ┌────────────▼───────────────────┐
                   │                    │    Zero-Dependency Native      │
                   │                    │        HTTP Server             │
                   │                    └────────────┬───────────────────┘
                   │                                 │
┌──────────────────▼─────────────────────────────────▼───────────────────┐
│                           Application Layer                            │
│   (QuestionService, SearchService, RevisionService, PracticeService)  │
└─────────────┬────────────────────────────────────────────┬─────────────┘
              │ Coordinates                                │ Queries
┌─────────────▼──────────────┐                ┌───────────▼─────────────┐
│       Core / DSA Layer     │                │       Domain Models     │
│  (Trie, Min-Heap, Queue,   │                │  (Question, User,       │
│   Stack, DoublyLinkedList) │                │   Session, Enums)       │
└────────────────────────────┘                └───────────▲─────────────┘
                                                          │ Maps
┌─────────────────────────────────────────────────────────┴─────────────┐
│                           Persistence Layer                           │
│     (IQuestionRepository, SqliteQuestionRepository, SqliteUserRepo)   │
└───────────────────────────────────────────────────────────────────────┘
```

---

## 5. Technology Stack

### Backend
- **Language**: C++17
- **Build System**: CMake (>= 3.16) & Ninja
- **Compiler**: GCC (>= 9), Clang (>= 10), or MSVC (>= 2019)
- **Database Engine**: Embedded SQLite 3 (WAL mode, foreign keys, secondary indices, Schema v3)
- **Cryptography**: Argon2id password hashing and Blake2b-256 token digests (via bundled `third_party/argon2`)
- **Networking**: Native socket-based zero-dependency cross-platform HTTP REST server

### Frontend
- **Framework**: React 18 & TypeScript 5
- **Tooling**: Vite 5
- **Styling**: Tailwind CSS 3 (coding-platform-native dark aesthetic)
- **Icons**: Lucide React

---

## 6. Repository Layout

```text
CodeVault/
├── CMakeLists.txt              # Top-level CMake configuration
├── README.md                   # Public project overview & instructions
├── RELEASE_NOTES.md            # Release Candidate notes and capabilities
├── MANUAL_QA_CHECKLIST.md      # Manual browser QA protocol
├── ARCHITECTURE.md             # System architecture and layer contracts
├── API_CONTRACT.md             # Complete REST API specification
├── DATA_MODEL.md               # Domain entities schema and enums
├── DSA_DESIGN.md               # DSA mapping, complexity, and custom specs
├── DECISIONS.md                # Architecture Decision Records (ADR-001 to ADR-023)
├── QUALITY_AND_HARDENING.md    # Quality metrics, audits, and verification
├── TESTING.md                  # Test specifications and execution commands
├── TODO.md                     # Staged development roadmap & status
│
├── include/                    # C++ Public Header Files
│   ├── app/                    # Presentation and CLI shell
│   ├── dsa/                    # Custom data structures (Trie, Heap, Queue, Stack, DLL)
│   ├── models/                 # Domain entities (Question, User, Session, Enums)
│   ├── persistence/            # SQLite & file repository abstractions
│   ├── services/               # Question, Search, Revision, Practice, Auth services
│   └── utils/                  # JSON parser, datetime, string utilities
│
├── src/                        # C++ Implementation Files
│   ├── app/                    # CLI shell implementation
│   ├── dsa/                    # Custom DSA implementations
│   ├── models/                 # Domain entity implementations
│   ├── persistence/            # SQLite repository & migration service
│   ├── services/               # Core business services
│   └── main.cpp                # Unified application entrypoint (CLI & Server)
│
├── third_party/                # Bundled Zero-Dependency Libraries
│   ├── sqlite/                 # SQLite 3 amalgamation
│   └── argon2/                 # Argon2id password hashing reference library
│
├── frontend/                   # Modern Desktop Web UI
│   ├── src/                    # React components, pages, services, types
│   ├── package.json            # Node.js dependencies
│   ├── tsconfig.json           # TypeScript configuration
│   └── vite.config.ts          # Vite build configuration
│
├── tests/                      # Automated Verification Suites
│   ├── unit/                   # 289 unit and integration test cases
│   └── smoke/                  # 5-check smoke test runner
│
├── test_live_e2e.ps1           # 49-check live runtime HTTP E2E PowerShell suite
└── data/
    ├── questions.csv           # Seed catalog
    └── sample/
        └── questions.csv       # Pristine sample dataset
```

---

## 7. Developer Onboarding & Quick Start

### 7.1 Prerequisites
- **Backend**: C++17 compiler (GCC $\ge$ 9, Clang $\ge$ 10, or MSVC $\ge$ 2019) and CMake $\ge$ 3.16.
- **Frontend**: Node.js (v18+ or v20+) and npm.

### 7.2 Build Instructions

```bash
# 1. Clone repository
git clone https://github.com/yourusername/CodeVault.git
cd CodeVault

# 2. Configure build with CMake
cmake -B build -S .

# 3. Compile backend executables
cmake --build build --config Release
```

### 7.3 Running the Application

CodeVault supports both a terminal-native CLI and a desktop browser interface:

#### Option A: Terminal CLI
```bash
# Launch interactive terminal shell (local single-user mode)
./build/bin/codevault --cli
```

#### Option B: Web UI (Full Experience)
```bash
# Terminal 1: Start native C++ HTTP server on port 8080
./build/bin/codevault --server 8080 data/codevault.db

# Terminal 2: Start frontend development server
cd frontend
npm install
npm run dev
```
Open your browser at `http://localhost:5173`.

---

## 8. Verification & Quality Assurance

CodeVault is thoroughly verified across multiple automated testing layers:

```bash
# 1. Run unit test suite (289 tests)
./build/bin/unit_tests.exe

# 2. Run automated smoke tests (5 checks)
./build/bin/smoke_test.exe data/sample/questions.csv

# 3. Run unified CTest runner (2 suites)
ctest --test-dir build --output-on-failure

# 4. Run live HTTP server runtime E2E suite (49 assertions)
powershell -ExecutionPolicy Bypass -File .\test_live_e2e.ps1

# 5. Build frontend production bundle
cd frontend
npm run build
```

| Verification Dimension | Result | Status |
| :--- | :--- | :--- |
| **Backend Unit Tests** | **289 / 289 PASSED** | 100% Green |
| **Automated Smoke Tests** | **5 / 5 PASSED** | 100% Green |
| **Unified CTest Runner** | **2 / 2 PASSED** | 100% Green |
| **Live Runtime E2E Suite** | **49 / 49 PASSED** | 100% Green |
| **Total Automated Assertions** | **343 / 343 PASSED** | 100% Green |
| **Frontend Production Build** | **0 Errors, 0 Warnings** | Passed (Vite + TypeScript) |
| **Compiler / Linter** | **0 Warnings, 0 Errors** | Clean build with `-Wall -Wextra -Wpedantic` |

---

## 9. Known Architectural Boundaries

1. **Transient Daily Practice Queue (ADR-023)**:
   The in-memory FIFO practice queue resets on server process restart for ultra-fast session slicing. All permanent question statuses, practice timestamps, notes, and Leitner SRS schedules are durably stored in SQLite.
2. **Browser Testing Methodology**:
   Browser interfaces are validated via strict TypeScript compilation, Vite production bundles, code audits, and live backend HTTP API tests. A headless automated browser runner (Playwright/Cypress) is omitted; refer to [MANUAL_QA_CHECKLIST.md](MANUAL_QA_CHECKLIST.md) for manual browser verification.
3. **Out-of-Scope Capabilities**:
   Third-party OAuth, cloud database clusters (PostgreSQL/Redis), and direct Online Judge scrapers are deferred to post-release stages.

---

## 10. Documentation Index

- [RELEASE_NOTES.md](RELEASE_NOTES.md) — Official Release Candidate notes, capabilities, and highlights.
- [MANUAL_QA_CHECKLIST.md](MANUAL_QA_CHECKLIST.md) — Step-by-step browser testing protocol.
- [ARCHITECTURE.md](ARCHITECTURE.md) — Comprehensive architectural layer separation and data flows.
- [API_CONTRACT.md](API_CONTRACT.md) — Complete REST API endpoint reference and payload contracts.
- [DSA_DESIGN.md](DSA_DESIGN.md) — Custom data structures design, invariants, and complexity proofs.
- [DECISIONS.md](DECISIONS.md) — Architecture Decision Records (ADR-001 through ADR-023).
- [QUALITY_AND_HARDENING.md](QUALITY_AND_HARDENING.md) — Detailed verification and hardening specification.
- [TESTING.md](TESTING.md) — Test plan, suite breakdowns, and execution guide.

---

## 11. License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

