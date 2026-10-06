# CodeVault — UI Design System Specification
## Stage 8 UI/UX Refinement: Coding-Practice Platform Visual Direction

## 1. Product Identity & Mental Model

- **Product Name**: **CodeVault**
- **Core Mental Model**: Personal coding-problem practice, organization, and revision platform.
- **Audience**: Students and software engineers actively preparing for technical interviews, DSA rounds, and competitive programming.
- **Aesthetic**: Developer tool, problem-solving workspace, high information density without clutter, clean tables and lists, compact navigation, clear problem metadata, strong typography, subtle borders, restrained semantic colors.
- **Design Rule**: Familiar coding-platform interaction patterns with an original, proprietary CodeVault visual identity. Never clone or copy trademarks, logos, or proprietary visual elements of external platforms.

---

## 2. Color System & Semantic Accents

CodeVault uses a restrained, high-contrast palette avoiding flashy neon or generic corporate blues.

### 2.1 Surfaces & Neutrals
| Token | Class / Hex | Usage |
| :--- | :--- | :--- |
| **`canvas-bg`** | `bg-slate-50` (`#f8fafc`) | Application background reducing eye fatigue during prolonged practice. |
| **`surface`** | `bg-white` (`#ffffff`) | Elevated cards, tables, workspace panels, input controls. |
| **`sub-surface`** | `bg-slate-100` (`#f1f5f9`) | Code blocks, table headers, badge backgrounds, tag chips. |
| **`border-subtle`**| `border-slate-200/90` (`#e2e8f0`) | Subtle structural dividers and compact table grid borders. |
| **`border-hover`** | `border-slate-300` (`#cbd5e1`) | Interactive hover states on buttons and inputs. |
| **`text-primary`** | `text-slate-900` (`#0f172a`) | Dominant problem titles, key numbers, primary headers. |
| **`text-secondary`**| `text-slate-700` (`#334155`) | Problem statements, approach notes, navigation labels. |
| **`text-muted`** | `text-slate-500` (`#64748b`) | Monospace IDs, timestamps, secondary metadata. |

### 2.2 Semantic Accents & Status Indicators
| Status / Category | Background | Border | Text | Visual Indicator |
| :--- | :--- | :--- | :--- | :--- |
| **Easy / Solved** | `bg-emerald-50` | `border-emerald-200` | `text-emerald-700` | `✓` Circle Checkmark (`#10b981`) |
| **Medium / Review** | `bg-amber-50` | `border-amber-200` | `text-amber-800` | `◐` Half-circle (`#f59e0b`) |
| **Hard / Urgent Due** | `bg-rose-50` | `border-rose-200` | `text-rose-700` | `Clock` / Due pill (`#f43f5e`) |
| **Mastered (Box 5)** | `bg-indigo-50` | `border-indigo-200` | `text-indigo-700` | `★` Star pill (`#4f46e5`) |
| **Unsolved / Backlog**| `bg-slate-100` | `border-slate-200` | `text-slate-600` | `○` Slate Circle outline (`#94a3b8`) |

> [!NOTE]
> Accessibility Guarantee: Color is never used as the sole status indicator. Every status row features an explicit text label or a distinct geometric icon.

---

## 3. Typography & Hierarchy

- **Interface Font**: `Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif`
- **Monospace Code Font**: `ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace`
- **Monospace Usage**: Problem IDs (`Q-001`), complexity notation ($O(N \log N)$), time intervals, table statistics, and algorithmic notes.

### Typographic Scale:
- **Problem Titles**: `text-base` or `text-2xl font-bold tracking-tight text-slate-900` (Visually dominant).
- **Section Headers**: `text-xs font-mono font-bold text-slate-400 uppercase tracking-wider`.
- **Table Cells**: `text-xs text-slate-700 font-sans`.
- **Metadata Chips**: `text-[10px] font-mono px-1.5 py-0.5 rounded`.

---

## 4. Information Density & Layout Strategy

1. **High Information Density**:
   - Compact table rows (`py-2.5 px-3`) allowing 15+ problems to be scanned without excessive scrolling.
   - Metadata is compact and inline (e.g. `Q-001 · LeetCode · #two-pointer`).
   - Revision state visible directly on the problems table row without opening problem detail.
2. **Compact Navigation**:
   - Slim 240px (`w-60`) developer sidebar.
   - Clean header bar with keyboard shortcut hint (`⌘K`) and instant `New Problem` / `Practice Drill` actions.
3. **Responsive Behavior**:
   - Desktop-first for optimal coding practice.
   - Mobile and tablet: tables scroll horizontally or collapse cleanly into responsive rows.

---

## 5. Stitch Refinement Mappings

| Screen | Stitch Screen ID | Key UX Pattern |
| :--- | :--- | :--- |
| **Problems Library (Pass 2)** | `d9fb6890285c4a9485f076e94e194d22` | Horizontal scrollable topic pills strip, high-density table, sequential problem numbering, instant autocomplete search. |
| **Problem Detail** | `8c6a65e714a741c0bad11c7dc7fef391` | Problem-solving workbench: Statement, Invariants & Notes, Preparation Status & Leitner retention meter. |
| **Practice Session** | `79e9d337251746d2a03ac892b49fa642` | Minimal distraction-free workspace, Queue progress, self-check solution drawer, ergonomic verdict dock. |
| **Revision Workspace** | `43ea6a18f80c48ffbb559a1be4dd2a8f` | Leitner 5-box distribution ribbon, Due Now quick pass/fail controls, MinHeap upcoming timeline. |
