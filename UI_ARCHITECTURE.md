# CodeVault — UI & Presentation Architecture Specification
## Stage 8 UI/UX Refinement: Coding-Practice Platform Experience

## 1. Executive Presentation Architecture Summary

CodeVault's architecture is built on Clean Architecture and Domain-Driven Design principles. The core business logic, algorithmic structures (Trie, MinHeap, Queue, Stack, DoublyLinkedList, MergeSort, QuickSort), domain models, and persistence repositories are completely decoupled from any user interface.

With the Stage 8 UI/UX Refinement, CodeVault repositions the user experience directly into a **personal coding-problem practice, organization, and revision platform**. 

Rather than presenting an enterprise or academic AI productivity dashboard, CodeVault adopts familiar, intuitive coding-platform interaction patterns with an original, developer-first visual identity.

```
┌────────────────────────────────────────────────────────┐
│           Web UI (React 18 + Vite + TypeScript)        │
│   Problems (Primary) | Practice | Revision | Dashboard │
└───────────────────────────┬────────────────────────────┘
                            │ HTTP JSON / API Layer (/api)
┌───────────────────────────▼────────────────────────────┐
│      Application Server / Adapter Boundary (C++17)      │
│         (Native Sockets HTTP REST Gateway :8080)       │
└───────────────────────────┬────────────────────────────┘
                            │ Invokes Clean Service Interfaces
┌───────────────────────────▼────────────────────────────┐
│             Application Services Layer                 │
│  QuestionService | SearchService | RevisionService     │
│  PracticeService | RecentHistoryService | StatisticsSvc│
└─────────────┬────────────────────────────┬─────────────┘
              │ Coordinates                │ Queries
┌─────────────▼──────────────┐ ┌───────────▼─────────────┐
│       Core / DSA Layer     │ │       Domain Models     │
│  PrefixTrie | MinHeap      │ │  Question, Difficulty,  │
│  Queue | Stack | SortEngine│ │  Status, Topic, Snapshot│
└─────────────┬──────────────┘ └───────────▲─────────────┘
              │ Reads/Writes               │ Serializes
┌─────────────▼────────────────────────────┴─────────────┐
│              Persistence Repository Layer              │
│     IQuestionRepository -> FileQuestionRepository      │
│                 (data/questions.csv)                   │
└────────────────────────────────────────────────────────┘
```

---

## 2. Why the UI Direction Was Refined

The initial Stage 8 foundation was technically robust and fully integrated with the C++ backend, but its layout and visual presentation felt too similar to a generic SaaS analytics dashboard and too close to the academic copilot visual language.

### Core Product Repositioning:
- **What CodeVault is**: A personal coding-problem practice, organization, and spaced revision platform.
- **What CodeVault is NOT**: A generic enterprise analytics dashboard or AI copilot.
- **The Core User Journey**:
  ```
  Problems (Catalog)
    ↓
  Search / Filter (Autocomplete & Tags)
    ↓
  Select Problem
    ↓
  Understand Problem (Workspace & Invariants)
    ↓
  Practice (Deliberate Drill Session)
    ↓
  Mark Solved / Needs Review / Skip
    ↓
  Revision (Leitner 5-Box Spaced Schedule)
    ↓
  Track Progress (Authentic Progress Metrics)
  ```

---

## 3. Revised Information Architecture

The Problems catalog is now the **primary center and default entry point** of the application. The Dashboard serves as a daily action cockpit rather than dominating the navigation.

### Primary Navigation Order:
1. **Problems (`/problems` or `/questions`)**: Primary catalog, dense scannable rows, instant autocomplete search, status and revision visibility.
2. **Practice (`/practice`)**: Distraction-free drill environment powered by the C++ FIFO Queue.
3. **Revision (`/revision`)**: Leitner 5-box memory workspace driven by the C++ priority MinHeap.
4. **Dashboard (`/dashboard`)**: Daily action cockpit answering *"What should I do today in CodeVault?"*.
5. **Statistics (`/statistics`)**: Developer curriculum analytics and coverage matrix.
6. **Settings (`/settings`)**: C++ engine telemetry and CSV persistence diagnostics.

---

## 4. Key Screen Architectural Hierarchies

### 4.1 Problems Page (`QuestionsPage.tsx`)
- **Mental Model**: Problem library and master preparation catalog.
- **Density**: High information density, compact rows, dominant problem titles.
- **Search**: Clean `Search problems...` input with live Trie-powered autocomplete suggestions without exposing technical terms like "Trie prefix".
- **Columns**: Status Icon (`✓` Solved, `★` Mastered, `◐` InProgress, `○` Unsolved), Title & Platform, Difficulty pill, Topic tag, Revision state (`Due`, `Tomorrow`, `In Nd`, `Mastered`), Favorite toggle, Actions.

### 4.2 Problem Detail Page (`QuestionDetailPage.tsx`)
- **Mental Model**: Problem-solving developer workbench.
- **Hierarchy**:
  - Header: Back to Problems, Title, Difficulty, Topic, Platform, Company, Quick Actions (`Practice Drill`, `Favorite`, `Edit`, `Delete`).
  - Left Canvas: Problem Description, Tags, and Approach Invariants & Solution Notes.
  - Right Sidebar: "Your Progress" status selector, Leitner SRS Box 1–5 retention meter, next due date, and quick verdict triggers (`Mark Solved`, `Needs Review`, `Reschedule`).

### 4.3 Practice Session Page (`PracticePage.tsx`)
- **Mental Model**: Deliberate, focused recall drill.
- **Hierarchy**:
  - Minimal distraction-free top bar showing Queue item progress (`Problem X of Y in Queue (Z%)`) and Exit button.
  - Active problem canvas with statement and collapsible Approach & Invariants self-check drawer.
  - Ergonomic bottom verdict dock: `[ Mark Solved ]` (emerald), `[ Needs Review ]` (amber), `[ Skip Problem ]` (neutral outline).

### 4.4 Spaced Revision Workspace (`RevisionPage.tsx`)
- **Mental Model**: Leitner 5-box memory management workspace.
- **Hierarchy**:
  - Top action CTA: `[ Practice Due Revisions (X) ]`.
  - Leitner 5-Box Distribution Ribbon (Box 1: 1d, Box 2: 3d, Box 3: 7d, Box 4: 14d, Box 5: 30d Mastered).
  - Due Now table with instant `Pass (+1 Box)` and `Fail (Box 1)` verdict controls.
  - Upcoming schedule ordered chronologically by C++ MinHeap priority.

### 4.5 Dashboard Page (`DashboardPage.tsx`)
- **Mental Model**: Daily action cockpit.
- **Hierarchy**:
  - Answers *"What should I practice in CodeVault today?"*.
  - Authentic summary metrics: `[ 42 / 100 Solved ]`, `[ 5 Due for Revision ]`.
  - Direct quick actions: `[ Practice Due ]`, `[ Browse Problems ]`, `[ Practice Unsolved ]`.
  - Due for Revision quick list, Difficulty distribution, Topic progress, and Recent activity.
  - Zero synthetic data: no fake streaks, no fake ranks, no fake AI scores.

---

## 5. Distinction from AI Academic & Career Copilot

CodeVault maintains an original, distinct visual identity:
1. **Developer Tool Aesthetic**: Restrained colors (Slate 900 / Slate 50 with emerald/amber/rose semantic accents), crisp borders, and subtle elevation.
2. **Code-Oriented Typography**: Monospace typography for problem IDs, complexities $O(N)$, tags, and algorithm invariants.
3. **No Decorative Fluff**: Replaced oversized cards and generic SaaS widgets with high-density, scannable data grids.
4. **Authentic DSA Foundation**: Every metric, search result, queue sequence, and heap item is computed directly by the native C++17 engine.

---

## 6. Stitch Refinement Integration

The UI refinement was established using the CodeVault Stitch project (`6846628486388263169`):
- **Problems Library**: `1fe8a780b6c549fb90c2e3ce585f18b0`
- **Problem Detail Workspace**: `8c6a65e714a741c0bad11c7dc7fef391`
- **Focused Practice Drill**: `79e9d337251746d2a03ac892b49fa642`
- **Spaced Revision Workspace**: `43ea6a18f80c48ffbb559a1be4dd2a8f`

All frontend components directly implement these refined Stitch designs while preserving the zero-dependency C++17 architecture and CLI compatibility.
