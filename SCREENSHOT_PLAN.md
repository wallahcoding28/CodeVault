# CodeVault — Screenshot & Visual Evidence Plan

> **Purpose**: Standardized capture plan for visual evidence, presentation slides, project portfolios, and GitHub repository media.  
> **Rule**: All screenshots must be captured manually from the running application with real, non-fabricated data.

---

## Pre-Capture Setup

1. **Start Backend**:
   ```bash
   ./build/bin/codevault.exe --server 8080 data/codevault.db
   ```
2. **Start Frontend**:
   ```bash
   cd frontend
   npm run dev
   ```
3. Set browser resolution to **1920x1080** (Full HD) with 100% zoom level.
4. Ensure dark theme is active and the browser console has no error banners.

---

## Recommended Screenshot Catalog

| # | Screenshot Name | Route / View | State / Visible Elements | Demonstration Purpose |
| :-: | :--- | :--- | :--- | :--- |
| **01** | `01_login_view.png` | `/login` | Clean login card, dark aesthetic, username/password inputs, "Sign In" button, link to registration. | Shows secure authentication entry point. |
| **02** | `02_questions_catalog.png` | `/questions` | Main table loaded with sample problems, difficulty badges (`Easy`, `Medium`, `Hard`), topic tags, and "Practice Next" banner. | Shows primary problem catalog and multi-platform organization. |
| **03** | `03_prefix_search_trie.png` | `/questions` | Search input with `"Two"` typed, active autocomplete suggestions matching prefix instantly. | Visual evidence of **Prefix Trie** in action. |
| **04** | `04_multi_filter_active.png` | `/questions` | Active topic filter (e.g. `Arrays`), difficulty filter (`Medium`), and table showing filtered problem count. | Demonstrates multi-criteria query and sorting pipeline. |
| **05** | `05_question_detail.png` | `/questions/Q-1001` | Full problem view: title, URL, topic/difficulty chips, problem description, personal notes, and verdict action buttons. | Shows problem inspection, notes editing, and practice controls. |
| **06** | `06_practice_queue_drawer.png` | Any page with drawer open | `PracticeQueueDrawer` slid out from right; numbered badges showing problems in FIFO order, with removal buttons. | Visual evidence of the custom **FIFO Queue** (`Queue<T>`). |
| **07** | `07_practice_next_rationale.png` | `/questions/:id` or `/dashboard` | "Recommended Next Problem" card displaying the deterministic `recommendationReason` banner. | Shows the 5-tier deterministic recommendation cascade. |
| **08** | `08_verdict_srs_feedback.png` | `/questions/:id` | Card displaying feedback immediately after clicking "Mark Solved" (e.g. "Leitner Level advanced to Box 2: Next due in 3 days"). | Demonstrates Spaced Repetition (SRS) interval advancement. |
| **09** | `09_revision_workspace.png` | `/revision` | 5 Leitner level boxes with question counts, "Due Now" urgent section, and upcoming review schedules. | Visual evidence of **Min-Heap** priority scheduling. |
| **10** | `10_dashboard_analytics.png` | `/dashboard` | Completion progress bar, difficulty breakdown charts, 14-topic coverage distribution, and activity counters. | Demonstrates analytical aggregation and progress tracking. |
| **11** | `11_settings_sessions.png` | `/settings` (Active Sessions) | Active sessions list showing current device badge, IP, user agent, and targeted revocation buttons. | Demonstrates multi-device security without token leakage. |
| **12** | `12_data_export_tab.png` | `/settings` (Data Export) | Export card with buttons for JSON, CSV, Markdown, and Anki TSV decks. | Demonstrates zero vendor lock-in and catalog portability. |
| **13** | `13_cli_terminal_mode.png` | Terminal | Interactive terminal shell running `codevault --cli` displaying the text-based main menu and question table. | Demonstrates CLI backward compatibility. |
| **14** | `14_account_menu_theme.png` | `/dashboard` (Header) | User account dropdown open, displaying user identity, Profile/Settings navigation links, and Light/Dark/System theme options. | Demonstrates accessible user account management and theme controls. |

---

## Storage & Archival Guideline

- Store captured images under `docs/screenshots/` (or repository wiki/assets).
- Save images in `.png` format with compression (e.g. OptiPNG or TinyPNG) to maintain fast repository clones.
