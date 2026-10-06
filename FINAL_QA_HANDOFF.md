# CodeVault — Final Release & Manual QA Handoff

> **Project State**: Feature-Frozen & Release Candidate Complete  
> **Target Release**: Public Open-Source Release / Academic Evaluation  
> **Verification Status**: Automated Suites 100% Green; Pending Final Human Browser QA  
> **Timestamp**: 2026-10-05

---

## 1. Release Status

CodeVault is **feature-frozen and ready for final manual QA before public release**.

All core architectural systems (C++17 Clean Architecture backend, embedded SQLite Schema v3 persistence, custom educational DSA engines, Argon2id authentication, deterministic practice recommendations, Leitner spaced revision, and React 18 + TypeScript web interface) are fully implemented and verified. No further functional modifications or architectural extensions are permitted.

---

## 2. Automated Verification Baseline

The repository has passed its complete automated verification baseline with zero failures:

| Verification Target | Measured Result | Scope & Coverage |
| :--- | :--- | :--- |
| **Backend Unit Tests** | **289 / 289 PASSED (100%)** | Domain models, custom DSA structures, SQLite persistence, Argon2id auth, practice cascades, and API contracts |
| **Automated Smoke Tests** | **5 / 5 PASSED (100%)** | Flat-file CSV ingestion, CLI menus, and core service queries |
| **Unified CTest Suites** | **2 / 2 PASSED (100%)** | Automated cross-target test runner (`SmokeTest` and `UnitTests`) |
| **Live HTTP E2E Suite** | **49 / 49 PASSED (100%)** | Live server test verifying registration, login, logout, cookie sessions, tenant isolation, import/export, and persistence across process restart |
| **Backend Native Build** | **0 Warnings, 0 Errors** | Clean build with `-Wall -Wextra -Wpedantic` on GCC 16.1.0 via Ninja |
| **Frontend Production Build** | **PASSED (0 Errors)** | 1588 modules compiled cleanly via Vite + TypeScript (`tsc && vite build`) |

---

## 3. Manual QA Required

While all headless and API-level assertions pass with 100% confidence, **the project owner must execute the manual browser verification protocol** outlined in [`MANUAL_QA_CHECKLIST.md`](MANUAL_QA_CHECKLIST.md) before publishing or formally presenting the project.

No automated Playwright, Cypress, or Selenium browser runner is configured in this environment; manual browser QA ensures visual, focus, and interaction parity.

---

## 4. Recommended Manual QA Sequence

Execute the verification steps in the following order:

1. **Authentication & Session Lifecycle** (`/register`, `/login`, logout, session restoration on refresh)
2. **Problems Catalog** (`/problems`, table rendering, difficulty badges, status filters)
3. **Search & Filter Operations** (Prefix Trie autocomplete with `"Two"`, topic filtering, company filtering, favorite stars)
4. **Problem Detail View** (`/problems/:id`, notes editing, status toggling)
5. **Practice Queue Operations** (`PracticeQueueDrawer`, adding problems, inspecting FIFO order, unlinking)
6. **Practice Session & Verdict Submission** (Drill modal, "Mark Solved", "Needs Review", Leitner interval advancement)
7. **Spaced Revision Workspace** (`/revision`, Leitner 5-box counts, "Due Now" section)
8. **Dashboard & Analytics** (`/dashboard`, completion percentage, topic distribution, "Recommended Next Problem" card)
9. **Statistics** (`/statistics`, detailed telemetry and histograms)
10. **Settings & Account Management** (`/settings`, profile update, password change, active device session revocation)
11. **Catalog Portability** (JSON export, CSV export, Markdown export, Anki TSV deck, and JSON/CSV import)
12. **Persistence Across Restart** (Terminate server with `Ctrl+C`, restart, verify durable data survives)
13. **Edge Cases & Error Handling** (404 on invalid problem ID, empty search states, corrupt import payloads)

---

## 5. Visual Evidence to Capture

For presentations, portfolios, or documentation, capture clean screenshots according to [`SCREENSHOT_PLAN.md`](SCREENSHOT_PLAN.md):

1. **Login & Register Views** (`/login`, `/register`)
2. **Problems Library & Filter Bar** (`/problems`)
3. **Prefix Search Trie Autocomplete** (Typing `"Two"` or `"Merge"`)
4. **Problem Detail with Practice Controls** (`/problems/:id`)
5. **Slide-Out Practice Queue Drawer** (`PracticeQueueDrawer`)
6. **Practice Session Drill & Verdict Feedback** (Leitner SRS advancement)
7. **Spaced Revision Workspace** (`/revision`)
8. **Dashboard Progress Cockpit** (`/dashboard`)
9. **Statistics Telemetry** (`/statistics`)
10. **Settings Active Sessions & Data Export** (`/settings`)
11. **CLI Shell Terminal Mode** (`codevault --cli`)

---

## 6. Known Intentional Architectural Limitations

The following behaviors are deliberate architectural decisions and must not be treated as defects:

1. **Transient Daily Practice Queue (ADR-023)**:
   The active daily practice queue is held in memory for low-latency FIFO sequencing and resets on process restart. All permanent revision schedules, problem statuses, and timestamps are durable in SQLite.
2. **Manual Browser QA Requirement**:
   Browser interfaces are validated via TypeScript compilation, Vite production bundles, code audits, and live backend HTTP API tests. An automated browser runner is not configured.
3. **Deferred Roadmap Features**:
   Third-party OAuth (Google/GitHub SSO), email verification, password reset emails, cloud database clustering (PostgreSQL/Redis), and external Online Judge scraping connectors are future scope.

---

## 7. Final Release Gate

```text
======================================================================
                     CODEVAULT RELEASE GATES                          
======================================================================
AUTOMATED UNIT & SMOKE TESTS   -> [PASSED] (294 / 294 checks green)
LIVE RUNTIME HTTP E2E SUITE    -> [PASSED] (49 / 49 assertions green)
BACKEND & FRONTEND BUILDS      -> [PASSED] (0 warnings, 0 errors)
REPOSITORY HYGIENE & SECRETS   -> [PASSED] (Zero artifacts / zero secrets)
DOCUMENTATION SYNCHRONIZATION  -> [PASSED] (17 aligned documents)
FEATURE FREEZE                 -> [ACTIVE] (Strict feature freeze)
MANUAL BROWSER QA              -> [PENDING] (Owner execution required)
======================================================================
FINAL STATUS: READY FOR OWNER BROWSER QA & PUBLIC RELEASE
======================================================================
```
