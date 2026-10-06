# CodeVault — Manual Browser QA Checklist

> **Purpose**: Practical manual verification protocol for the CodeVault Web UI and native HTTP backend. Execute this checklist in modern desktop browsers (Chrome, Firefox, Edge, Safari) prior to formal release.

---

## Pre-Requisites

1. **Start Backend Server**:
   ```bash
   ./build/bin/codevault.exe --server 8080 data/codevault.db
   ```
2. **Start Frontend Dev Server**:
   ```bash
   cd frontend
   npm run dev
   ```
3. Open browser at `http://localhost:5173`.

---

## 1. Authentication & Session Lifecycle

- [ ] **Registration**:
  - [ ] Navigate to `/register`.
  - [ ] Submit form with valid username, email, display name, and password ($\ge 8$ chars). Verify automatic redirect to `/dashboard` (primary cockpit) and authenticated state in top bar.
  - [ ] Attempt registration with duplicate username. Verify clear error message is displayed.
  - [ ] Attempt registration with invalid email format. Verify form validation catches error.
- [ ] **Login & Default Landing**:
  - [ ] Log in with valid credentials. Verify user is redirected by default to `/dashboard`.
  - [ ] Attempt login with incorrect password. Verify "Invalid credentials" error banner appears without page reload.
  - [ ] Direct protected-route navigation test: While logged out, navigate directly to `http://localhost:5173/problems`. Verify redirect to `/login?from=/problems`. After logging in, verify successful redirection back to the requested destination (`/problems`), preserving return-to-route behavior.
- [ ] **Session Restoration**:
  - [ ] Hard refresh (`Ctrl+F5` or `Cmd+Shift+R`) while logged in on `/dashboard`. Verify user session remains intact without redirecting to `/login`.
- [ ] **Route Protection**:
  - [ ] Log out, then manually navigate to `http://localhost:5173/questions`. Verify immediate redirect to `/login?from=/questions`.

---

## 1.1 User Account Menu & Theme Controls

- [ ] **User Account Menu**:
  - [ ] In the top-right header, click the user account chip (avatar initials, name, and chevron).
  - [ ] Verify the accessible dropdown opens with user identity details: display name, `@username`, and email.
  - [ ] Test click-outside dismissal: click anywhere outside the dropdown and verify the menu closes.
  - [ ] Test keyboard dismissal: open the menu and press `Escape`. Verify the dropdown closes immediately.
  - [ ] In the account menu, click "Profile": verify navigation to `/settings?tab=profile`.
  - [ ] In the account menu, click "Settings": verify navigation to `/settings`.
  - [ ] In the account menu, click "Sign out": verify session is terminated and user is redirected to `/login`.
- [ ] **Theme Switching (Light / Dark / System)**:
  - [ ] Open the account menu and locate the Theme selector (Light, Dark, System buttons).
  - [ ] Select **Light**: verify the application immediately adopts the light theme (clean light backgrounds, dark text, visible borders, legible cards/badges).
  - [ ] Select **Dark**: verify the application immediately adopts the dark theme (slate dark backgrounds, light text, legible cards/badges).
  - [ ] Verify foreground/text contrast and visibility across all major views in Dark mode (ensure headings, labels, metadata, inputs, badges, and empty states have WCAG AA legible contrast against dark backgrounds).
  - [ ] Select **System**: verify the application synchronizes with the OS/browser color scheme.
  - [ ] Refresh the page while Light or Dark is selected. Verify the selected theme persists without flashing the opposite theme on initial load.
  - [ ] Verify theme controls on `/settings` (Profile tab $\to$ Appearance & Theme card) remain synchronized with the account menu.

---

## 2. Question Management (CRUD, Search, Filter, Sort, Pagination)

- [ ] **Question List**:
  - [ ] Navigate to `/problems` (or `/questions`). Verify catalog table renders problem rows with badges.
- [ ] **Search Autocomplete (Prefix Trie)**:
  - [ ] Type a known prefix (e.g. "Two") into the search input. Verify instant Trie-backed matching dropdown.
  - [ ] Click an autocomplete suggestion. Verify direct navigation to the selected problem.
  - [ ] Type non-existent query (e.g. "ZzzNonExistent"). Verify clean "No questions found" empty state with "Reset Filters" action.
- [ ] **Filtering**:
  - [ ] Filter by Difficulty (`Easy`, `Medium`, `Hard`). Verify table updates accordingly.
  - [ ] Filter by Algorithmic Topic (e.g. `Arrays`, `Graphs`, `DynamicProgramming`). Verify filtered results match.
  - [ ] Filter by Status (`Unsolved`, `InProgress`, `Solved`, `Mastered`).
  - [ ] Filter by Target Company (e.g. `Google`, `Amazon`) if present.
  - [ ] Toggle "Favorites Only" star filter. Verify only starred problems render.
- [ ] **Sorting**:
  - [ ] Sort by Title (Ascending / Descending).
  - [ ] Sort by Difficulty and Last Practiced. Verify ordering changes deterministically.
  - [ ] Toggle sorting algorithm preference (`Merge Sort` vs `Quick Sort`). Verify list reorders correctly.
- [ ] **Pagination**:
  - [ ] Change page size (e.g. 15, 25, 50). Verify table page size updates.
  - [ ] Navigate across pages (Next / Prev). Verify pagination controls reflect current bounds.
- [ ] **Question Detail & Edit**:
  - [ ] Click on a problem row to open `/problems/:id`.
  - [ ] Verify title, difficulty badge, topic tag, company tags, and problem description render correctly.
  - [ ] Update status to `Solved`. Verify badge reflects the change immediately.
  - [ ] Toggle Favorite star on detail view. Verify favorite state updates.
  - [ ] Edit and save personal notes. Verify updated notes persist upon refresh.
  - [ ] Click "Edit Problem" (`/problems/:id/edit`). Update title/description, save, and verify catalog updates.

---

## 3. Practice Workflow & Practice Queue

- [ ] **Practice Next CTA**:
  - [ ] On `/questions` or `/dashboard`, click "Practice Next Problem".
  - [ ] Verify the system navigates to the recommended problem.
  - [ ] Verify the deterministic recommendation rationale card explains why this problem was selected.
- [ ] **Practice Queue Operations**:
  - [ ] Click "Queue" button on any question to add it to the practice queue.
  - [ ] Open the slide-out `PracticeQueueDrawer`. Verify the problem appears in FIFO order with its index badge.
  - [ ] Click the remove icon on a queued problem. Verify the queue count decrements and the item leaves the drawer.
- [ ] **Practice Session Drill**:
  - [ ] Click "Start Practice Session" / "Drill Session" to open `StartSessionModal`.
  - [ ] Select a topic filter (e.g. `Arrays`) and initialize session.
  - [ ] Verify problems populate the active queue.
- [ ] **Recording Verdicts**:
  - [ ] On Question Detail page, click "Mark Solved". Verify Leitner interval schedule updates (e.g. Level 1 $\to$ +1 day).
  - [ ] On another problem, click "Needs Review". Verify interval resets to 1 day and priority escalates.
  - [ ] Verify the problem automatically unlinks from the active practice queue upon verdict submission.
  - [ ] Press keyboard shortcut `N` to navigate to the next recommended problem.

---

## 4. Spaced Revision Workspace

- [ ] Navigate to `/revision`.
- [ ] Verify Leitner 5-box breakdown cards render with correct question counts.
- [ ] Verify "Due Now" section highlights overdue problems.
- [ ] Verify upcoming scheduled problems display correct dates and relative timestamps.

---

## 5. Dashboard & Analytics
 
- [ ] Navigate to `/dashboard`.
- [ ] Verify Curriculum Completion progress bar renders percentage and solved counts.
- [ ] Verify Difficulty distribution bar charts (Easy, Medium, Hard) match catalog counts.
- [ ] Verify 14-topic coverage cards reflect correct problem counts.
- [ ] Verify "Recommended Next Problem" card displays explainable rationale string.
- [ ] Test zero-data empty state: Log in as a newly registered user with 0 questions. Verify dashboard renders clean empty state without crashing or throwing console errors.
- [ ] Navigate to `/statistics`.
- [ ] Verify comprehensive curriculum telemetry, difficulty bar charts, and topic histograms render cleanly with error boundaries.

---

## 6. Settings, Profile & Security

- [ ] **Profile Management**:
  - [ ] Navigate to `/settings` $\to$ Profile tab.
  - [ ] Update Display Name and Email. Click "Save Changes". Verify success feedback.
- [ ] **Security & Password**:
  - [ ] Navigate to `/settings` $\to$ Security tab.
  - [ ] Enter wrong current password. Verify error message appears.
  - [ ] Change password with valid current password and $\ge 8$-char new password. Verify success banner.
- [ ] **Active Sessions**:
  - [ ] Navigate to `/settings` $\to$ Active Sessions tab.
  - [ ] Verify current session is highlighted with "Current Session" badge.
  - [ ] Open a second private/incognito browser window and log in. Refresh sessions list in first window. Verify second session appears.
  - [ ] Click "Revoke" on second session. In the incognito window, verify subsequent API requests receive 401.

---

## 7. Catalog Portability (Import / Export)

- [ ] **Exporting Data**:
  - [ ] Navigate to `/settings` $\to$ Data Export tab.
  - [ ] Click "Export JSON". Verify `.json` file downloads and contains valid array of question objects.
  - [ ] Click "Export CSV". Verify RFC 4180 CSV downloads with escaped fields.
  - [ ] Click "Export Markdown". Verify `.md` file downloads with categorized problem list.
  - [ ] Click "Export Anki TSV". Verify `.tsv` file downloads with 3-column format.
- [ ] **Importing Data**:
  - [ ] Upload a valid JSON question file using "Skip" conflict strategy. Verify success message and imported question counts.
  - [ ] Upload an RFC 4180 CSV file. Verify questions appear in the catalog.
  - [ ] Attempt uploading an invalid/corrupted JSON file. Verify clear error message without partial database pollution.

---

## 8. Multi-User Isolation & Security Boundaries

- [ ] **Tenant Isolation**:
  - [ ] User A logs in and notes the ID of a private question (e.g. `Q-1005`).
  - [ ] User B logs in on another browser/session and enters the direct URL `http://localhost:5173/questions/Q-1005`.
  - [ ] Verify User B receives a 404 Not Found error and cannot view or edit User A's question.
  - [ ] Verify User B's practice queue and recommendations never contain User A's questions.

---

## 9. Persistence & Server Restart Verification

- [ ] **Account & Question Durability**:
  - [ ] Record a new problem or mark an existing problem `Solved` with custom notes.
  - [ ] Terminate the backend HTTP server process (`Ctrl+C` in Terminal 1).
  - [ ] Restart the server: `./build/bin/codevault.exe --server 8080 data/codevault.db`.
  - [ ] Refresh the browser. Log in again if session cookie expired.
  - [ ] Verify the question remains `Solved` with the exact updated notes and timestamps.
- [ ] **Transient Practice Queue Reset (ADR-023)**:
  - [ ] Add 2 problems to the practice queue before server termination.
  - [ ] Restart the server and refresh the browser.
  - [ ] Verify the practice queue count resets to 0 as designed per ADR-023.

---

## 10. Edge Cases & Error Boundary Handling

- [ ] **Invalid/Unknown Question ID**:
  - [ ] Manually navigate to `http://localhost:5173/problems/Q-99999`. Verify clean 404 error state appears without unhandled exceptions.
- [ ] **Empty Search / Filter Results**:
  - [ ] Search for a non-existent title with active filters. Verify empty state renders with "Reset Filters" action.
- [ ] **Empty Queue State**:
  - [ ] Open `PracticeQueueDrawer` when no problems are queued. Verify informative empty state with "Browse Catalog" CTA.
- [ ] **Empty Revision List**:
  - [ ] Log in with an account having no overdue questions. Verify `/revision` displays "All Caught Up!" message.
- [ ] **Corrupt Import Payload**:
  - [ ] Attempt uploading an invalid JSON structure. Verify error notification appears and catalog remains intact without partial commits.
- [ ] **Unauthorized Access Rejection**:
  - [ ] In private browsing window, attempt direct URL navigation to `/settings`. Verify immediate redirection to `/login`.

---

## 11. Visual & Responsive Sanity

- [ ] **Desktop & Laptop Widths**:
  - [ ] Test on 1920x1080 (Desktop), 1366x768 (Laptop), and 1024x768 (Tablet/Small Laptop).
  - [ ] Confirm no horizontal scrollbar on body, no overlapping text, and responsive sidebar navigation.
- [ ] **Visual Theme**:
  - [ ] Verify consistent dark palette (`slate-900`/`slate-800`), clean typography, crisp borders, and restrained developer aesthetics.
- [ ] **Console Inspection**:
  - [ ] Open Browser DevTools Console (`F12`).
  - [ ] Navigate across all pages. Verify zero uncaught JavaScript exceptions and zero React render-phase warnings.
