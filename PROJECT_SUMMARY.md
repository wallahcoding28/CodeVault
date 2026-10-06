# CodeVault — College Project & Viva Summary

> **Academic Project Reference**: DSA Lab & Software Engineering Coursework  
> **Course / Level**: B.Tech Computer Science & Engineering (2nd / 3rd Year)  
> **Subject Area**: Data Structures & Algorithms, Object-Oriented Software Design, Clean Architecture, Systems Programming

---

## 1. Project Title & Overview

**Title**: CodeVault — Smart Coding Question Management & Practice Platform with Transparent Data Structures & Algorithms

**Overview**: CodeVault is a desktop-first software platform designed to help students and competitive programmers systematically catalog, search, revise, and practice algorithmic problems from multiple platforms (LeetCode, Codeforces, HackerRank, GeeksforGeeks). Unlike commercial trackers or spreadsheets that treat data storage as an opaque black box, CodeVault is designed so that foundational data structures (Tries, Min-Heaps, Queues, Stacks, Linked Lists, Hash Maps) actively drive the core product capabilities.

---

## 2. Problem Statement & Motivation

During technical placement preparation, students solve hundreds of algorithmic problems across disparate platforms. This creates three critical problems:
1. **Catalog Fragmentation**: Solutions, notes, and progress are scattered across browser bookmarks and unstructured spreadsheets.
2. **Revision Degradation (The Forgetting Curve)**: Without disciplined spaced repetition intervals, pattern retention decays quickly.
3. **Academic-Practical Disconnect**: Students study data structures theoretically in coursework but rarely implement them from scratch to solve concrete, practical software engineering problems.

CodeVault bridges this gap by unifying problem management while turning its own engine into a transparent, living demonstration of DSA in action.

---

## 3. Core Objectives

- Develop a production-quality desktop application in **C++17** adhering to Clean Architecture principles.
- Implement custom, generic data structures from raw pointers without relying on standard library container wrappers (`std::queue`, `std::priority_queue`, `std::list`).
- Implement an automated, Leitner-based spaced repetition scheduling engine prioritized by a binary **Min-Heap**.
- Implement a sub-millisecond prefix search and autocomplete engine using a **Prefix Trie**.
- Implement a deterministic 5-tier recommendation cascade ("Practice Next") explaining exactly why each problem is recommended.
- Implement secure multi-user tenant isolation using **Argon2id** password hashing and **Blake2b-256** session digests over an embedded **SQLite 3** persistence layer.
- Provide dual presentation layers: an interactive terminal **CLI shell** and a modern desktop **React 18 + TypeScript Web UI**.

---

## 4. Technologies & Tools Used

| Layer | Technology | Role |
| :--- | :--- | :--- |
| **Core Systems Language** | C++17 | Business services, domain entities, custom DSA implementations |
| **Build System** | CMake (>= 3.16) & Ninja | Cross-platform build orchestration and parallel compilation |
| **Persistence Engine** | Embedded SQLite 3 (Schema v3) | ACID transactional storage, foreign key enforcement, WAL mode |
| **Cryptography** | Argon2id & Blake2b-256 | Memory-hard password hashing and session token hashing |
| **Networking** | Native BSD/Winsock Sockets | Zero-dependency cross-platform native HTTP server |
| **Frontend Framework** | React 18 & TypeScript 5 | Desktop web application with strict type safety |
| **Styling & Icons** | Tailwind CSS 3 & Lucide React | Coding-platform-native dark aesthetic and responsive UI |
| **Testing Tools** | CTest, Custom Unit Test Framework, PowerShell | Unit, smoke, CTest, and live HTTP E2E verification suites |

---

## 5. Summary of Data Structures & Algorithms Implemented

| Data Structure / Algorithm | Source Path | Product Role in CodeVault | Time Complexity | Space Complexity |
| :--- | :--- | :--- | :--- | :--- |
| **Prefix Trie** | `include/dsa/trie.hpp` | Prefix search & instant autocomplete on titles and tags. | $O(L)$ where $L$ is key length | $O(\Sigma \cdot L \cdot N)$ |
| **Min-Heap** | `include/dsa/min_heap.hpp` | Spaced revision scheduler extracting earliest due problem. | $O(\log N)$ insert / extract | $O(N)$ contiguous vector |
| **FIFO Queue** | `include/dsa/queue.hpp` | Sequences daily practice drill blocks; supports targeted node unlinking. | $O(1)$ push / pop, $O(N)$ unlink | $O(N)$ linked nodes |
| **LIFO Stack** | `include/dsa/stack.hpp` | Navigation history tracking recently inspected problems. | $O(1)$ push / pop | $O(N)$ linked nodes |
| **Doubly Linked List** | `include/dsa/doubly_linked_list.hpp` | Problem playlist walkthrough with bidirectional navigation. | $O(1)$ push / pop / splice | $O(N)$ linked nodes |
| **Hash Map** | `std::unordered_map` | Fast multi-index lookups by ID, Topic, Company, and Platform. | Average $O(1)$ lookup | $O(N)$ buckets |
| **Merge Sort** | `include/dsa/sorting.hpp` | Stable sorting of problem tables across combined criteria. | $O(N \log N)$ worst-case | $O(N)$ auxiliary buffer |
| **Quick Sort** | `include/dsa/sorting.hpp` | In-place catalog sorting with median-of-three pivot selection. | Average $O(N \log N)$ | $O(\log N)$ call stack |

---

## 6. Architecture & Design Principles

- **Clean Layered Architecture**: Strict separation between Presentation (CLI / HTTP), Application Services (`QuestionService`, `RevisionService`, `PracticeService`, `SearchService`), Core DSA, Domain Entities, and Persistence.
- **Rule of 5 Compliance**: Custom data structures implement explicit copy constructors, copy assignment, move constructors, move assignment, and destructors for leak-free RAII memory management.
- **Decoupled DSA Core**: Data structures are completely UI-agnostic; they know nothing about terminal ANSI escapes, JSON serialization, or HTTP headers.
- **Zero-Mutation Analytics**: `StatisticsService` aggregates dashboard statistics without mutating source question vectors or heap entries.

---

## 7. Testing & Quality Assurance Baseline

CodeVault was engineered with strict verification gating across all development stages:
- **289 Backend Unit Tests**: 100% passing across domain logic, DSA memory safety, SQLite repositories, and auth middleware.
- **5 Automated Smoke Tests**: 100% passing on CSV ingestion and core query pipelines.
- **2 Unified CTest Suites**: 100% passing across all compiled test targets.
- **49 Live HTTP Runtime E2E Assertions**: 100% passing in live PowerShell server testing verifying authentication, tenant isolation, practice workflows, and persistence across process restart.
- **Frontend Production Build**: Clean Vite + TypeScript compilation (1588 modules, 0 warnings/errors).
- **Compiler Cleanliness**: 0 compiler warnings under `-Wall -Wextra -Wpedantic` on GCC 16.1.0.

---

## 8. Current Limitations

1. **Transient Daily Practice Queue Persistence (ADR-023)**:
   The active daily practice queue is maintained in memory for low-latency session drilling and resets on server restart. All durable question statuses, timestamps, and Leitner revision intervals remain stored in SQLite.
2. **Automated Headless Browser Runner**:
   Browser interfaces are validated via TypeScript compilation, Vite production bundles, code audits, and live backend HTTP API tests. A headless automated browser runner (Playwright/Cypress) is omitted; manual browser QA is documented in `MANUAL_QA_CHECKLIST.md`.

---

## 9. Future Scope (Roadmap Items)

The following items are documented roadmap items deferred to post-release development:
- **Third-Party OAuth / SSO**: Google and GitHub sign-in integration for public web deployments.
- **Online Judge API Connectors**: Direct synchronization with LeetCode and Codeforces profile submission APIs.
- **Cloud Database Clustering**: PostgreSQL / Cloud SQL and Redis session caching options.
- **Contextual Algorithmic Hints**: Rule-based or opt-in algorithmic hint assistant for stuck problems.
- **Mobile Responsive App**: Native mobile app wrapper using React Native or Flutter.
