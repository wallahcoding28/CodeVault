# CodeVault — Dashboard & Progress Statistics (Stage 6)

## 1. Overview & Objectives

Stage 6 implements the **Dashboard & Progress Statistics** subsystem of CodeVault. It aggregates question progress, curriculum difficulty distribution, topic coverage, revision schedules, and practice metrics into deterministic snapshot objects.

### Core Architectural Principles
1. **Clean Separation of Concerns**: Statistics and calculation logic are fully decoupled from terminal I/O. `StatisticsService` computes pure snapshot structures; `CliShell` merely formats and renders them.
2. **Deterministic, Derived Metrics**: Every single metric is computed strictly from persisted `Question` domain data and revision timestamps. No artificial scores, fake analytics, or vanity metrics are fabricated.
3. **Future Frontend Compatibility**: The `DashboardSnapshot` structure is designed to be directly serialized and consumed by upcoming Web or Desktop GUI interfaces without modifying calculation services or data models.
4. **Safety & Zero Mutation**: Dashboard queries operate on read-only question collections and guarantee 100% immutability of the underlying CSV catalog.

---

## 2. Architecture & Service Boundaries

```
┌────────────────────────────────────────────────────────┐
│                   Presentation Layer                   │
│      CliShell: Main Menu (Option 5), Progress Bars     │
└───────────────────────────┬────────────────────────────┘
                            │ Calls getDashboardSnapshot()
┌───────────────────────────▼────────────────────────────┐
│                    Application Layer                   │
│                    StatisticsService                   │
│  - computeOverallStatistics                            │
│  - computeDifficultyStatistics                         │
│  - computeTopicStatistics                              │
│  - computeStatusStatistics                             │
│  - computeRevisionStatistics                           │
│  - computePracticeStatistics                           │
└─────────────┬───────────────────────────┬──────────────┘
              │ Queries Questions         │ Queries Clock / State
┌─────────────▼─────────┐   ┌─────────────▼──────────────┐
│    QuestionService    │   │      RevisionService       │
│ (Entity Catalog Store)│   │  (MinHeap Revision Queue)  │
└───────────────────────┘   └────────────────────────────┘
```

### Layer Responsibilities
- **`StatisticsService`**: Stateless or injected orchestrator that aggregates metrics across `models::Question` vectors.
- **`DashboardSnapshot`**: Immutable domain structure packaging the computed metrics across all dimensions.
- **`CliShell`**: Consumes `DashboardSnapshot` and renders structured terminal output with text-based progress bars.

---

## 3. Dashboard Snapshot Data Model

The dashboard snapshot is structured into strongly typed categories defined in [`include/models/statistics_models.hpp`](file:///f:/CodeVault/include/models/statistics_models.hpp):

```
DashboardSnapshot
├── generatedAt (int64_t epoch seconds)
├── OverallStatistics
│   ├── totalQuestions
│   ├── solvedCount, masteredCount, inProgressCount, unsolvedCount, favoriteCount
│   ├── dueForRevisionCount, upcomingRevisionsCount, totalScheduledCount
│   └── completionPercentage, solvedPercentage, masteredPercentage, etc.
├── DifficultyStatistics
│   ├── easyCount, mediumCount, hardCount
│   └── easyPercentage, mediumPercentage, hardPercentage
├── TopicStatistics
│   ├── topicCounts (vector of TopicCount across all 14 curriculum topics)
│   ├── distinctTopicsCount
│   └── totalQuestions
├── StatusStatistics
│   ├── unsolvedCount, inProgressCount, solvedCount, masteredCount
│   └── respective percentages
├── RevisionStatistics
│   ├── dueCount, upcomingCount, scheduledCount, unscheduledCount
│   ├── priorityCounts[6] (Urgency Priority 1..5 breakdown)
│   ├── levelCounts[6] (Revision Level 1..5 breakdown)
│   └── duePercentageOfScheduled, scheduledPercentageOfTotal
└── PracticeStatistics
    ├── practicedCount, unpracticedCount
    ├── practicedPercentage
    └── lastPracticedTimestamp
```

---

## 4. Metric Formulas & Mathematical Definitions

All percentages are calculated using zero-division protected logic:
$$\text{calculatePercentage}(N, D) = \begin{cases} 0.0 & \text{if } D = 0 \\ \frac{N \times 100.0}{D} & \text{if } D > 0 \end{cases}$$

### 4.1 Overall Curriculum Progress
- **Total Questions ($T$)**: Count of questions in the repository catalog.
- **Solved ($S$)**: Count where $\text{status} = \text{Status::Solved}$.
- **Mastered ($M$)**: Count where $\text{status} = \text{Status::Mastered}$.
- **In Progress ($I$)**: Count where $\text{status} = \text{Status::InProgress}$.
- **Unsolved ($U$)**: Count where $\text{status} = \text{Status::Unsolved}$.
- **Favorites ($F$)**: Count where $\text{isFavorite}() = \text{true}$.
- **Completion Rate**:
  $$\text{Completion Percentage} = \frac{S + M}{T} \times 100.0$$
- **Solved Ratio**:
  $$\text{Solved Percentage} = \frac{S}{T} \times 100.0$$
- **Mastered Ratio**:
  $$\text{Mastered Percentage} = \frac{M}{T} \times 100.0$$

### 4.2 Difficulty Distribution
- **Easy Percentage**: $\frac{\text{easyCount}}{T} \times 100.0$
- **Medium Percentage**: $\frac{\text{mediumCount}}{T} \times 100.0$
- **Hard Percentage**: $\frac{\text{hardCount}}{T} \times 100.0$
- *Total represents $100\%$ across defined tiers without rounding overflow.*

### 4.3 Topic Coverage
- Evaluated across all 14 standardized `Topic` categories:
  Arrays, Strings, LinkedLists, StacksQueues, Trees, Graphs, DynamicProgramming, BinarySearch, RecursionBacktracking, Greedy, Heaps, BitManipulation, MathGeometry, Other.
- **Topic Percentage**: $\frac{\text{topicQuestionCount}}{T} \times 100.0$
- **Distinct Topics Count**: Count of topics where $\text{topicQuestionCount} > 0$.

### 4.4 Spaced Revision & Retention
- **Due for Revision ($D_{rev}$)**: $\text{next\_revision\_at} > 0 \land \text{next\_revision\_at} \le \text{currentTime}$.
- **Upcoming Scheduled ($U_{rev}$)**: $\text{next\_revision\_at} > \text{currentTime}$.
- **Total Scheduled ($S_{rev}$)**: $D_{rev} + U_{rev}$.
- **Unscheduled**: $\text{next\_revision\_at} \le 0$.
- **Due Percentage of Scheduled**: $\frac{D_{rev}}{S_{rev}} \times 100.0$.
- **Catalog Revision Coverage**: $\frac{S_{rev}}{T} \times 100.0$.
- **Priority Distribution**: Counts grouped by urgency priority tiers ($1 = \text{Urgent}$ to $5 = \text{Lowest}$).
- **Level Distribution**: Counts mapped to interval levels 1 through 5 using $\text{calculateLevelFromQuestion}(q)$.

### 4.5 Practice Activity
- **Practiced Count**: Questions where $\text{last\_practiced\_at} > 0$.
- **Unpracticed Count**: Questions where $\text{last\_practiced\_at} = 0$.
- **Practiced Ratio**: $\frac{\text{practicedCount}}{T} \times 100.0$.
- **Last Practice Attempt**: $\max(\text{last\_practiced\_at})$ across all questions.

---

## 5. CLI Presentation & Visualization

`CliShell` renders progress bars using universal text characters (`#` and `-`), guaranteeing zero character corruption or terminal encoding issues on Windows, Linux, and macOS:

```
Completion Rate (Solved + Mastered):
[#############-----------] 55.6% (5 of 9 questions)

DIFFICULTY DISTRIBUTION
Easy      : 4    (44.4 %) [#######---------]
Medium    : 5    (55.6 %) [#########-------]
Hard      : 0    (0.0  %) [----------------]
```

---

## 6. Edge Case Handling

1. **Empty Question Catalog ($T = 0$)**:
   - Zero-division guard in `calculatePercentage` prevents arithmetic exceptions.
   - All counts return 0; all percentages return `0.0%`.
   - CLI prints an informative message instead of empty/broken tables.
2. **Zero Solved / All Unsolved**:
   - Accurately reports $0.0\%$ completion without warnings.
3. **All Questions Having Same Difficulty / Topic**:
   - Reports $100.0\%$ for the matched attribute and $0.0\%$ for others.
4. **No Scheduled Revisions**:
   - Reports 0 due, 0 upcoming, $0.0\%$ due ratio, and $T$ unscheduled questions.
5. **Deterministic Invariance**:
   - Calling `computeDashboardSnapshot` repeatedly produces identical metrics without mutating inputs.

---

## 7. Scope Boundaries: Present vs. Future

### Implemented in Stage 6 (Supported by Stored Data)
- Curriculum completion and status breakdown.
- Topic distribution and coverage ratios across 14 categories.
- Difficulty distribution (counts and percentages).
- Due vs. upcoming revision distribution.
- Urgency priority and revision level histograms.
- Practiced vs. unpracticed ratios and latest practice timestamp.

### Deferred to Future Scope (Not Supported by Current Data)
- **Practice Streaks**: Requires persistent daily calendar event logs.
- **Average Solving Time**: Requires active stopwatch/timer tracking during practice sessions.
- **Accuracy / First-Attempt Solve Rate**: Requires append-only attempt logs distinguishing initial solves from repeat practice.
- **Retention Decay Curves**: Requires longitudinal review verdict histories.
