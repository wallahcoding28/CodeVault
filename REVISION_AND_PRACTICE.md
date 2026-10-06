# CodeVault — Spaced Revision & Practice System (Stage 5)

## 1. Overview & Objectives

Stage 5 implements the complete Revision and Practice subsystem of CodeVault. It bridges core educational Data Structures & Algorithms (DSA) with serious coding-practice management capabilities:
- **Deterministic Spaced Revision**: Scheduling based on clear intervals (1, 3, 7, 14, 30 days) rather than heuristic approximations.
- **Custom MinHeap Priority Queue**: High-performance priority queueing for revision scheduling without relying on `std::priority_queue`.
- **Custom FIFO Practice Queue**: Strict First-In First-Out session execution powered by CodeVault's custom `Queue`, avoiding `std::queue`.
- **Practice Verdict Evaluation**: Standardized handling for `Solved`, `NeedsReview`, and `Skipped`.
- **Persistent State Across Restarts**: Preserving practice history, revision deadlines, status, and urgency priorities across application restarts using the standard 18-column CSV schema.
- **Testable Time Abstraction**: Injected clock interface (`IClock`) enabling deterministic unit testing of due dates and intervals without dependence on real-world system clocks.

---

## 2. Architecture & Service Boundaries

Stage 5 preserves CodeVault's Clean Architecture layer separation:

```
┌────────────────────────────────────────────────────────┐
│                   Presentation Layer                   │
│      CliShell: Practice & Revision Submenus / Prompts  │
└───────────────────────────┬────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│                    Application Layer                   │
│  PracticeService      RevisionService   QuestionService│
│   (Session Queue)      (MinHeap Queue)  (Entity Sync)  │
└─────────────┬─────────────────────┬──────────────┬─────┘
              │ Coordinates         │ Coordinates  │ Operates on
┌─────────────▼─────────┐ ┌─────────▼────────────┐ ┌▼──────────────┐
│        Custom Queue   │ │     Custom MinHeap   │ │ Domain Models │
│   (FIFO Practice)     │ │ (Revision Scheduling)│ │ (Question etc)│
└───────────────────────┘ └──────────────────────┘ └───────────────┘
                                                           │
                                            ┌──────────────▼───────┐
                                            │     Persistence      │
                                            │ FileQuestionRepo     │
                                            └──────────────────────┘
```

### Clean Layer Responsibilities
- **`QuestionService`**: Manages entity lifecycle, persistence synchronization, and updates underlying data stores. It optionally notifies `RevisionService` when questions are modified or deleted.
- **`RevisionService`**: Encapsulates all revision logic: priority heap maintenance, schedule calculations, interval progressions, and due-item queries.
- **`PracticeService`**: Manages the transient practice session state, FIFO question queue, attempt tracking, and user verdicts.
- **`CliShell`**: Provides user input/output formatting and menu navigation. Contains **zero** scheduling math or business logic.

---

## 3. Revision Domain Design & Intervals

### 3.1 Domain Fields on Question Entity
Revision state is fully captured within the existing `Question` model without introducing schema bloat or external tables:
- `last_practiced_at`: Epoch timestamp (seconds) of the most recent practice attempt.
- `next_revision_at`: Epoch timestamp (seconds) when the question is due for revision.
- `revision_priority`: Urgency tier from 1 (highest urgency / urgent) to 5 (lowest urgency).
- `status`: Workflow state (`Unsolved`, `Todo`, `InProgress`, `Solved`, `Mastered`).

### 3.2 Deterministic Revision Interval Algorithm
CodeVault uses an initial deterministic revision schedule:
- **Level 1**: 1 Day ($86,400\text{ s}$)
- **Level 2**: 3 Days ($259,200\text{ s}$)
- **Level 3**: 7 Days ($604,800\text{ s}$)
- **Level 4**: 14 Days ($1,209,600\text{ s}$)
- **Level 5+**: 30 Days ($2,592,000\text{ s}$)

*Note: Documented specifically as CodeVault's initial deterministic revision schedule rather than a claim of scientific or adaptive optimality.*

### 3.3 Practice Verdict Evaluation

| Verdict | Level Progression | Next Revision Date | Priority / Urgency | Status Update |
| :--- | :--- | :--- | :--- | :--- |
| **`Solved`** | Current Level $+ 1$ (up to Level 5) | `currentTime + interval(Level + 1)` | Decreases urgency (e.g. Priority 4 or 5) | `InProgress` $\rightarrow$ `Solved`; Level 5 $\rightarrow$ `Mastered` |
| **`NeedsReview`** | Resets to Level 1 | `currentTime + 1 day` | Priority 1 (Urgent) | Remains `InProgress` |
| **`Skipped`** | Unchanged | Unchanged | Unchanged | Unchanged |

---

## 4. Custom Data Structure Integration

### 4.1 Custom `MinHeap` Revision Queue
CodeVault uses its custom `codevault::dsa::MinHeap<RevisionItem, RevisionItemComparator>` as the persistent scheduling engine.

#### Item Ordering & Strict Weak Ordering
The heap comparator enforces strict, deterministic ordering:
1. **Primary Key**: `nextRevisionAt` ascending (earliest revision first).
2. **Secondary Key**: `priority` ascending ($1 = \text{Urgent}$ before $5$).
3. **Tertiary Key**: `questionId` ascending (lexicographical string tie-breaker).

#### Algorithmic Complexity
- **Insert / Enqueue (`insert`)**: $O(\log N)$
- **Extract Minimum (`extractMin`)**: $O(\log N)$
- **Peek Next Revision (`peek`)**: $O(1)$
- **Heap Construction (`buildHeap` / `loadFromQuestions`)**: $O(N)$

### 4.2 Custom `Queue` Practice Session
Practice sessions run entirely on CodeVault's custom FIFO `codevault::dsa::Queue<std::string>`.

#### Queue Operations
- **Enqueue (`enqueue`)**: $O(1)$ amortized
- **Dequeue (`dequeue`)**: $O(1)$
- **Peek (`front`)**: $O(1)$
- **Skip Cycling (`skipCurrentQuestion`)**: Takes the current question from the front and pushes it to the back of the queue in $O(1)$ time, allowing deferred review within the same session.

---

## 5. Due & Upcoming Logic

- **Due Condition**: A question is considered due for revision when:
  $$\text{next\_revision\_at} \le \text{current\_time}$$
- **Upcoming Revisions**: Items where $\text{next\_revision\_at} > \text{current\_time}$, extracted in sorted order.
- **Unscheduled Items**: Questions with $\text{next\_revision\_at} \le 0$ are not tracked in the active revision heap.

### Testable Clock Abstraction
To ensure deterministic testing without relying on `std::chrono::system_clock::now()`, CodeVault provides an injectable time provider:
```cpp
class IClock {
public:
    virtual ~IClock() = default;
    virtual int64_t now() const = 0;
};
```
- `SystemClock`: Default production implementation using epoch seconds.
- `MockClock`: Injected during unit tests to manipulate time deterministically.

---

## 6. Persistence & File Ingestion

All practice updates, timestamp changes, and priority modifications are saved through `FileQuestionRepository::flushToFile()`.
- **Format**: Standard CSV (18 columns).
- **Restart Guarantee**: On initialization, `RevisionService::loadFromQuestions()` rebuilds the `MinHeap` from the persisted question collection in $O(N)$ time.
- **Data Integrity**: Clean CSV field escaping prevents commas inside notes from distorting token indices.

---

## 7. CLI Submenus & Workflows

### 7.1 Revision Submenu (`Option 4`)
```
================================
            REVISION            
================================
1. Show Due Questions
2. Show Upcoming Revisions
3. Show Next Revision
4. Start Due Revision Session
5. Schedule Question
0. Back
--------------------------------
```

### 7.2 Practice Submenu (`Option 3`)
```
================================
        PRACTICE SESSION        
================================
1. Practice all unsolved
2. Practice by topic
3. Practice by difficulty
4. Practice favorites
5. Practice due revisions
0. Back
--------------------------------
```

### 7.3 Active Practice Session Display
```
--------------------------------
Practice Question (1 of 4, 4 remaining)
--------------------------------
ID: Q-1001
Title: Two Sum
Topic: Arrays
Difficulty: Easy
Company: Google
Platform: LeetCode
Status: Solved
Notes: Use hash map for O(n) time and O(n) space.

Description:
Find two numbers in array that add up to target

--------------------------------
1. Mark Solved
2. Needs Review
3. Skip
4. Exit Session
--------------------------------
```

---

## 8. Verification & Test Suite

Stage 5 expanded the unit test suite from 122 to **152 automated tests** across 17 dedicated test suites:
- **Suite 13**: Spaced Revision Schedule & Progression (Tests 123–132)
- **Suite 14**: MinHeap Ordering, Tie-Breaking & Due Queries (Tests 133–141)
- **Suite 15**: Practice Session Queue Mechanics (Tests 142–147)
- **Suite 16**: Service Coordination & Verdict Processing (Tests 148–150)
- **Suite 17**: Persistence Across Restarts & End-to-End Workflow (Tests 151–152)

Both `SmokeTest` and `UnitTests` pass with 100% success rate under CTest.

---

## 9. Scope Boundaries

### Implemented Now (Stage 5)
- Deterministic 1/3/7/14/30-day revision intervals.
- `MinHeap` revision scheduling with 3-level deterministic tie-breaking.
- `Queue` FIFO practice session with front-to-back cycling on skip.
- Testable time provider (`IClock` / `MockClock`).
- Rich practice verdicts (`Solved`, `NeedsReview`, `Skipped`).
- Full CLI menus for Revision and Practice workflows.
- Full CSV persistence across restarts.

### Future Scope (Stage 6+)
- Statistics and progress analytics reporting.
- Advanced metrics (streaks, retention curves, topic mastery distribution).
- Web / Desktop GUI (Qt / Web frontend) reusing `PracticeService` and `RevisionService`.
- Multi-user authentication and cloud synchronization.
