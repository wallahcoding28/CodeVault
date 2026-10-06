# CodeVault — Engineering Portfolio Summary

> **Project Title**: CodeVault  
> **Repository**: [github.com/yourusername/CodeVault](https://github.com/yourusername/CodeVault)  
> **Tech Stack**: C++17, CMake, Embedded SQLite (Schema v3), Argon2id, React 18, TypeScript, Tailwind CSS, Vite  
> **Standard**: MIT License

---

## 1. One-Line Summary (GitHub Card / Headline)

A desktop-first coding practice and question management platform engineered with custom C++ data structures, Leitner spaced repetition, and full tenant isolation.

---

## 2. Short Summary (2–3 Sentences for Resumes & Portfolio Pages)

CodeVault is a unified workspace for software engineers and competitive programmers to organize, search, revise, and master algorithmic problems from multiple judges. It replaces opaque spreadsheets with custom, inspectable C++ data structures (Prefix Trie, Min-Heap, FIFO Queue, Doubly Linked List) powering sub-millisecond search autocomplete, 5-tier deterministic practice recommendations, and automated spaced revision. Backed by embedded SQLite and Argon2id security, it features an interactive terminal CLI and a modern React 18 + TypeScript web cockpit with default post-login Dashboard navigation, accessible user account controls, and Light / Dark / System theme support.

---

## 3. Technical Summary (Detailed Technical Profile)

- **Authentic DSA Engineering**: Implements custom header-only data structures from raw pointers adhering to Rule-of-5 copy/move semantics—including a `PrefixTrie` for $O(L)$ search autocomplete, a `MinHeap<T>` priority queue for spaced revision scheduling, a custom linked `Queue<T>` with $O(N)$ node unlinking for practice drills, and a `DoublyLinkedList<T>` for problem playlists.
- **Embedded ACID Persistence**: Engineered over an embedded SQLite 3 engine (Schema v3) using WAL mode, parameterized queries, and idempotent schema migrations, eliminating external database dependencies.
- **Modern Cryptography & Security**: Employs Argon2id memory-hard password hashing ($t=2, m=19456\text{ KiB}$) with CSPRNG salts, stores only Blake2b-256 session token digests, transports sessions via `HttpOnly` cookies, and enforces strict owner-scoped multi-user tenant isolation across all services.
- **Deterministic Recommendation Engine**: Features a 5-tier rule-based "Practice Next" recommendation cascade prioritizing active queue heads $\to$ overdue Leitner revisions $\to$ fresh unsolved problems, returning explainable rationale strings with zero LLM hallucination risk.
- **Full Data Portability**: Supports transactional batch import and export across structured JSON, RFC 4180 CSV, Markdown catalogs, and Anki-compatible 3-column TSV flashcard decks.
- **Rigorously Tested Baseline**: Backed by 289 backend unit tests, 5 automated smoke tests, 2 CTest suites, 49 live HTTP runtime E2E assertions, and a clean Vite production build (0 warnings/errors).

---

## 4. Key Metrics & Engineering Highlights

| Metric | Measured Value | Significance |
| :--- | :--- | :--- |
| **Backend Unit Tests** | 289 / 289 (100% Passed) | Comprehensive domain, DSA, service, and SQLite test coverage |
| **Automated Smoke Tests** | 5 / 5 (100% Passed) | Quick sanity verification of CSV ingestion and core query pipeline |
| **Live Runtime E2E Suite** | 49 / 49 (100% Passed) | Live HTTP server verification of auth, owner isolation, practice, and persistence |
| **Compiler Warnings** | 0 Warnings | Clean build under `-Wall -Wextra -Wpedantic` on GCC 16.1.0 |
| **Frontend Production Build** | 0 Errors | 1588 modules compiled via Vite + TypeScript in ~5 seconds |
| **Search Autocomplete Latency** | $< 1$ ms | Custom Prefix Trie character traversal |
| **Revision Heap Extraction** | $O(\log N)$ | Dynamic priority scheduling without database table scans |
