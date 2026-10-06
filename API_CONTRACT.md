# CodeVault — API Contract Specification

## 1. Overview & Architectural Boundary

The CodeVault API is a lightweight, local REST/JSON interface providing communication between the React web application and the native C++17 core engine.

- **Base URL**: `http://localhost:8080/api`
- **Content-Type**: `application/json; charset=utf-8`
- **Zero Business Logic in Frontend**: The frontend merely issues commands and displays query results. All Trie searches, MinHeap prioritization, Queue cycling, Stack tracking, and sorting algorithms execute in the C++ layer.

---

## 2. API Endpoints

### 2.1 Questions API

#### `GET /api/questions`
Query questions with optional filtering, prefix search, keyword search, and sorting.
- **Query Parameters**:
  - `prefix`: string (Trie prefix match against title)
  - `keyword`: string (Full-field search across title, description, company, tags)
  - `topic`: string (Enum name, e.g., `Arrays`, `DynamicProgramming`, etc.)
  - `difficulty`: string (`Easy`, `Medium`, `Hard`)
  - `status`: string (`Unsolved`, `InProgress`, `Solved`, `Mastered`)
  - `company`: string (Substring match)
  - `favorite`: boolean (`true` / `false`)
  - `sortBy`: string (`id`, `title`, `difficulty`, `topic`, `status`, `revisionPriority`, `createdAt`)
  - `sortDir`: string (`asc` / `desc`)
  - `sortAlgo`: string (`mergeSort` / `quickSort`)
- **Response**: `200 OK`
  ```json
  [
    {
      "id": "Q-1001",
      "title": "Two Sum",
      "description": "Given an array of integers nums and an integer target...",
      "topic": "Arrays",
      "difficulty": "Easy",
      "company": "Google, Amazon, Meta",
      "platform": "LeetCode",
      "source_url": "https://leetcode.com/problems/two-sum/",
      "status": "Solved",
      "is_favorite": true,
      "notes": "Optimal Approach: One-pass Hash Map. O(N) time and space.",
      "created_at": 1728700000,
      "updated_at": 1728710000,
      "last_practiced_at": 1728710000,
      "next_revision_at": 1728796400,
      "revision_priority": 1,
      "tags": ["hash-table", "two-pointers", "array"],
      "owner_id": "local_user"
    }
  ]
  ```

#### `GET /api/questions/:id`
Fetch single question by ID and record inspection onto the LIFO history stack.
- **Response**: `200 OK` (Question object) or `404 Not Found`.

#### `POST /api/questions`
Create a new question record with full business-rule validation.
- **Request Body**:
  ```json
  {
    "title": "Subarray Sum Equals K",
    "description": "Given an array of integers nums and an integer k...",
    "topic": "Arrays",
    "difficulty": "Medium",
    "company": "Meta, Amazon",
    "platform": "LeetCode",
    "source_url": "https://leetcode.com/problems/subarray-sum-equals-k/",
    "status": "Unsolved",
    "is_favorite": false,
    "notes": "Use prefix sums with hash map counting frequencies.",
    "revision_priority": 2,
    "tags": ["hash-table", "prefix-sum", "array"]
  }
  ```
- **Response**: `201 Created` with generated ID (e.g. `{"id": "Q-1011", ...}`) or `400 Bad Request` with `{"error": "Validation failed: ..."}`.

#### `PUT /api/questions/:id`
Update an existing question.
- **Request Body**: Question fields to update.
- **Response**: `200 OK` or `400 Bad Request` / `404 Not Found`.

#### `DELETE /api/questions/:id`
Remove question from persistent storage, Trie, and MinHeap.
- **Response**: `200 OK` (`{"success": true}`) or `404 Not Found`.

---

### 2.2 Dashboard & Statistics API

#### `GET /api/dashboard`
Returns the complete, immutable `DashboardSnapshot` computed by `StatisticsService`.
- **Response**: `200 OK`
  ```json
  {
    "generatedAt": 1728800000,
    "overall": {
      "totalQuestions": 10,
      "solvedCount": 5,
      "masteredCount": 2,
      "inProgressCount": 1,
      "unsolvedCount": 2,
      "favoriteCount": 4,
      "dueForRevisionCount": 3,
      "upcomingRevisionsCount": 5,
      "totalScheduledCount": 8,
      "completionPercentage": 70.0
    },
    "difficulty": {
      "easyCount": 4,
      "mediumCount": 4,
      "hardCount": 2,
      "easyPercentage": 40.0,
      "mediumPercentage": 40.0,
      "hardPercentage": 20.0
    },
    "topic": {
      "distinctTopicsCount": 5,
      "topicCounts": [
        {"topic": "Arrays", "topicName": "Arrays", "count": 3, "percentage": 30.0},
        {"topic": "DynamicProgramming", "topicName": "Dynamic Programming", "count": 2, "percentage": 20.0},
        {"topic": "Graphs", "topicName": "Graphs", "count": 2, "percentage": 20.0},
        {"topic": "Trees", "topicName": "Trees", "count": 2, "percentage": 20.0},
        {"topic": "Strings", "topicName": "Strings", "count": 1, "percentage": 10.0}
      ]
    },
    "status": {
      "unsolvedCount": 2,
      "inProgressCount": 1,
      "solvedCount": 5,
      "masteredCount": 2
    },
    "revision": {
      "dueCount": 3,
      "upcomingCount": 5,
      "scheduledCount": 8,
      "unscheduledCount": 2,
      "priorityCounts": [0, 2, 3, 2, 1, 0],
      "levelCounts": [0, 2, 3, 2, 2, 1]
    },
    "practice": {
      "practicedCount": 8,
      "unpracticedCount": 2,
      "practicedPercentage": 80.0,
      "lastPracticedTimestamp": 1728795000
    }
  }
  ```

---

### 2.3 Spaced Revision API

#### `GET /api/revision/due`
Retrieve questions currently due (`next_revision_at <= now()`), prioritized by MinHeap.
- **Response**: `200 OK` (Array of RevisionItems and associated Question entities).

#### `GET /api/revision/upcoming`
Retrieve upcoming scheduled revisions (`next_revision_at > now()`) ordered chronologically.
- **Response**: `200 OK`.

#### `POST /api/revision/schedule`
Schedule or reschedule a question with custom timestamp and priority (1-5).
- **Request Body**: `{"questionId": "Q-1001", "nextRevisionAt": 1729000000, "priority": 1}`
- **Response**: `200 OK`.

---

### 2.4 Practice Session & Advanced Workflow API

All practice endpoints are strictly protected by session authentication middleware (`401 Unauthorized` when unauthenticated) and enforce multi-user tenant isolation.

#### `GET /api/practice/next`
Deterministically evaluates and recommends the single best next problem to practice according to the 5-tier recommendation cascade:
1. Head of active practice session queue (if matching criteria)
2. Overdue spaced revision items from `MinHeap` (earliest due + highest urgency priority)
3. Unsolved / never-practiced problems
4. In-progress / attempted problems
5. Stale solved/mastered retention reinforcement

- **Query Parameters**:
  - `topic`: string (optional, e.g. `Arrays`, `DynamicProgramming`)
  - `difficulty`: string (optional, `Easy`, `Medium`, `Hard`)
  - `includeDueRevisions`: boolean (optional, default `true`)
  - `preferUnsolved`: boolean (optional, default `true`)
- **Response**: `200 OK`
  ```json
  {
    "hasQuestion": true,
    "question": {
      "id": "Q-1005",
      "title": "Merge Intervals",
      "topic": "Arrays",
      "difficulty": "Medium",
      "status": "Solved",
      "last_practiced_at": 1728000000,
      "next_revision_at": 1728100000,
      "revision_priority": 1,
      "owner_id": "USR-1001"
    },
    "recommendationReason": "Due for spaced revision (Priority 1)"
  }
  ```
  When no question matches: `{"hasQuestion": false, "question": null, "recommendationReason": "No eligible problem found matching criteria"}`.

#### `POST /api/practice/session`
Initialize a targeted practice session filtered by specific criteria. Loads matching questions in FIFO order into the active practice queue.
- **Request Body**:
  ```json
  {
    "topic": "Arrays",
    "difficulty": "Medium",
    "status": "Unsolved",
    "company": "Google",
    "isFavorite": true,
    "dueOnly": false
  }
  ```
- **Response**: `200 OK` with updated `SessionProgress`:
  ```json
  {
    "total": 4,
    "completed": 0,
    "remaining": 4,
    "skipped": 0
  }
  ```

#### `GET /api/practice/queue`
Inspect all questions currently loaded in the active FIFO practice queue.
- **Response**: `200 OK`
  ```json
  {
    "queue": [
      { "id": "Q-1001", "title": "Two Sum", "topic": "Arrays", "difficulty": "Easy", ... },
      { "id": "Q-1005", "title": "Merge Intervals", "topic": "Arrays", "difficulty": "Medium", ... }
    ],
    "count": 2
  }
  ```

#### `DELETE /api/practice/queue/:questionId`
Remove a specific question from the active practice queue without affecting persistent storage.
- **Authentication**: Required (`401 Unauthorized` if unauthenticated).
- **HTTP Method**: `DELETE`
- **Successful Response**: `200 OK`
  Returned when the question exists, belongs to the authenticated user, and was successfully removed from the active queue:
  ```json
  {
    "success": true,
    "removed": true,
    "questionId": "Q-1001"
  }
  ```
- **Error Responses**:
  - `400 Bad Request`: `{"error": "Question ID is required"}` when `questionId` is missing or empty.
  - `404 Not Found`:
    - `{"error": "Question not found"}`: If the question does not exist in the database or belongs to another user (owner-scoped isolation).
    - `{"error": "Question is not in the practice queue"}`: If the question exists for the user but is not currently present in their active queue (or was already removed).

#### `POST /api/practice/:questionId/result`
Directly submit a practice verdict (`Solved`, `NeedsReview`, `Skipped`) for any specific question (e.g. from the Problem Detail workspace). Updates question status and timestamps in storage via `QuestionService`, advances Leitner 5-box SRS interval via `RevisionService`, and unlinks the question if present in the active practice queue.
- **Request Body**:
  ```json
  {
    "verdict": "Solved"
  }
  ```
  Allowed verdicts: `"Solved"`, `"NeedsReview"` (or `"Review"`), `"Skipped"` (or `"Skip"`).
- **Response**: `200 OK`
  ```json
  {
    "success": true,
    "questionId": "Q-1005",
    "verdict": "Solved",
    "question": {
      "id": "Q-1005",
      "title": "Merge Intervals",
      "status": "Solved",
      "last_practiced_at": 1728800000,
      "next_revision_at": 1729059200,
      "revision_priority": 2,
      ...
    },
    "schedule": {
      "nextLevel": 2,
      "nextRevisionAt": 1729059200,
      "revisionPriority": 2,
      "lastPracticedAt": 1728800000,
      "newStatus": "Solved",
      "intervalSeconds": 259200
    }
  }
  ```

#### `POST /api/practice/start`
Legacy/batch initialization of a FIFO practice session with selected question IDs or preset filter strings (`due`, `all_unsolved`, `favorites`, `topic:<topic>`, `difficulty:<diff>`).
- **Request Body**: `{"filter": "due"}`, or `{"questionIds": ["Q-1001", "Q-1004"]}`
- **Response**: `200 OK` with `SessionProgress`.

#### `GET /api/practice/current`
Inspect the question currently at the head of the FIFO queue without dequeuing.
- **Response**: `200 OK` (`{"hasQuestion": true, "question": {...}, "progress": {...}}` or `{"hasQuestion": false}`).

#### `POST /api/practice/verdict`
Submit a practice verdict for the question currently at the front of the queue.
- **Request Body**: `{"verdict": "Solved"}`
- **Response**: `200 OK` with `RevisionScheduleResult` and next question.

#### `POST /api/practice/skip`
Cycles the current front question to the back of the FIFO queue.
- **Response**: `200 OK` with `{"skipped": true, "progress": {...}}`.

#### `GET /api/practice/progress`
Returns active progress metrics: `{"total": 6, "completed": 2, "remaining": 3, "skipped": 1}`.

#### `POST /api/practice/exit`
Clears active practice session queue.
- **Response**: `200 OK` with `{"success": true}`.

---

### 2.5 Recent History & Diagnostics API

#### `GET /api/history`
Returns questions on the LIFO history stack ordered from most recently viewed to oldest.
- **Response**: `200 OK` (Array of Question IDs).

#### `GET /api/settings/diagnostics`
Returns system status, active file path (`data/questions.csv`), record count, and DSA engine diagnostics.
- **Response**: `200 OK`

---

### 2.6 Authentication & Account Management API

CodeVault uses session tokens stored in secure `HttpOnly; SameSite=Lax` cookies named `codevault_session` (or passed via standard `Authorization: Bearer <token>` headers).

#### `POST /api/auth/register`
Create a new user account. Returns user object and sets `codevault_session` cookie.
- **Public endpoint**: No authentication required.
- **Request Body**:
  ```json
  {
    "username": "coder_jane",
    "displayName": "Jane Doe",
    "email": "jane@example.com",
    "password": "SecretPassword123!"
  }
  ```
- **Response**: `201 Created` with set cookie and body:
  ```json
  {
    "user": {
      "id": "USR-1001",
      "username": "coder_jane",
      "displayName": "Jane Doe",
      "email": "jane@example.com",
      "createdAt": 1728800000,
      "updatedAt": 1728800000
    }
  }
  ```
- **Error Response**: `400 Bad Request` with `{"error": "Validation error or username taken"}`.

#### `POST /api/auth/login`
Authenticate existing user by username and password. Sets `codevault_session` cookie.
- **Public endpoint**: No authentication required.
- **Request Body**:
  ```json
  {
    "username": "coder_jane",
    "password": "SecretPassword123!"
  }
  ```
- **Response**: `200 OK` with set cookie and user object:
  ```json
  {
    "user": {
      "id": "USR-1001",
      "username": "coder_jane",
      "displayName": "Jane Doe",
      "email": "jane@example.com",
      "createdAt": 1728800000,
      "updatedAt": 1728800000
    }
  }
  ```
- **Error Response**: `401 Unauthorized` with `{"error": "Invalid username or password"}`.

#### `POST /api/auth/logout`
Revoke active session token and clear `codevault_session` cookie.
- **Response**: `200 OK`
  ```json
  {"message": "Successfully logged out"}
  ```

#### `GET /api/auth/me`
Retrieve currently authenticated user profile.
- **Authentication**: Required (`401 Unauthorized` if invalid/missing session).
- **Response**: `200 OK`
  ```json
  {
    "user": {
      "id": "USR-1001",
      "username": "coder_jane",
      "displayName": "Jane Doe",
      "email": "jane@example.com",
      "createdAt": 1728800000,
      "updatedAt": 1728800000
    }
  }
  ```

#### `PUT /api/auth/profile`
Update user display name and email.
- **Authentication**: Required.
- **Request Body**:
  ```json
  {
    "displayName": "Jane D.",
    "email": "janedoe@example.com"
  }
  ```
- **Response**: `200 OK` with updated user object.
- **Error Response**: `400 Bad Request` with `{"error": "..."}`.

#### `POST /api/auth/change-password`
Change authenticated user's password.
- **Authentication**: Required.
- **Request Body**:
  ```json
  {
    "currentPassword": "OldPassword123!",
    "newPassword": "NewStrongPassword456!"
  }
  ```
- **Response**: `200 OK`
  ```json
  {"message": "Password updated successfully"}
  ```
- **Error Response**: `400 Bad Request` with `{"error": "..."}`.

#### `GET /api/auth/sessions`
List all active sessions for the authenticated user.
- **Authentication**: Required.
- **Response**: `200 OK`
  ```json
  {
    "sessions": [
      {
        "id": "SES-1001",
        "createdAt": 1728800000,
        "expiresAt": 1729404800,
        "lastSeenAt": 1728805000,
        "isCurrentSession": true
      }
    ]
  }
  ```

#### `POST /api/auth/sessions/:sessionId/revoke`
Revoke a specific active session by ID.
- **Authentication**: Required.
- **Response**: `200 OK` with `{"message": "Session revoked successfully"}` or `404 Not Found`.

#### `POST /api/auth/sessions/revoke-others`
Revoke all active sessions belonging to the user except the caller's current session.
- **Authentication**: Required.
- **Response**: `200 OK` with `{"message": "All other sessions revoked successfully"}`.

#### `GET /api/user/export`
Export the authenticated user's question catalog as JSON or CSV.
- **Authentication**: Required.
- **Query Parameters**:
  - `format`: `json` (default) or `csv`
- **Response**: `200 OK`
  - For `format=json`:
    - `Content-Type: application/json; charset=utf-8`
    - `Content-Disposition: attachment; filename="codevault_questions_export.json"`
    - Body: `{"questions": [...], "exportedAt": 1728800000, "totalCount": 10}`
  - For `format=csv`:
    - `Content-Type: text/csv; charset=utf-8`
    - `Content-Disposition: attachment; filename="codevault_questions_export.csv"`
    - Body: RFC 4180 CSV with standard header row.

---

### 2.7 Catalog Import & Advanced Export API (Stage 9.5)

Stage 9.5 introduces robust, atomic catalog import and advanced export formats for personal data portability and spaced-repetition workflow integrations.

#### `POST /api/user/import`
Import question catalog records from raw JSON or RFC 4180 CSV payloads with atomic transaction rollback and DSA synchronization.
- **Authentication**: Required. Returns `401 Unauthorized` with `{"error": "Authentication required"}` if unauthenticated.
- **Supported Content-Types**:
  - `application/json`
  - `text/csv` or `application/csv`
  - (Alternatively specified via query parameter `?format=json` or `?format=csv`)
- **Query Parameters**:
  - `conflict_strategy`: Conflict resolution strategy for matching question IDs (`id`).
    - `skip` (default if omitted): Keep existing records unchanged; skip conflicting incoming records.
    - `overwrite`: Overwrite existing questions in storage and refresh in-memory DSA nodes.
    - `generate_new_id`: Assign a new unique sequential ID (e.g., `Q-1011`) to conflicting incoming questions.
    - Also supports camelCase `conflictStrategy` parameter.
- **Request Body Semantics**:
  - For `application/json`:
    - Can be a JSON object containing a `questions` array: `{"questions": [...]}`
    - Or a direct JSON array of questions: `[...]`
  - For `text/csv`:
    - Raw RFC 4180 CSV string with a header row matching CodeVault question fields (`id`, `title`, `description`, `topic`, `difficulty`, `company`, `platform`, `source_url`, `status`, `is_favorite`, `notes`, `created_at`, `updated_at`, `last_practiced_at`, `next_revision_at`, `revision_priority`, `tags`, `owner_id`).
- **Owner Isolation**:
  - Incoming records are strictly scoped to the authenticated caller's `owner_id`. Any mismatched incoming `owner_id` is overwritten with the authenticated user ID.
  - Conflicts are checked strictly against questions owned by the authenticated caller.
- **Atomic Semantics & DSA Synchronization**:
  - All database mutations run inside an atomic SQLite transaction (`BEGIN IMMEDIATE TRANSACTION ... COMMIT`).
  - If any record validation fails or a database write errors out, the transaction is completely rolled back (`ROLLBACK`).
  - In-memory `PrefixTrie` and `MinHeap` structures are synchronized only upon a successful commit, guaranteeing in-memory and on-disk parity.
- **Successful Response**: `200 OK`
  Wire format exposes both camelCase and snake_case fields for full compatibility:
  ```json
  {
    "success": true,
    "totalProcessed": 10,
    "importedCount": 8,
    "updatedCount": 2,
    "skippedCount": 0,
    "total_processed": 10,
    "imported_count": 8,
    "updated_count": 2,
    "skipped_count": 0,
    "errors": []
  }
  ```
- **Error Response**: `400 Bad Request`
  Returned when payload format is invalid, required fields are missing, or transaction fails validation:
  ```json
  {
    "success": false,
    "totalProcessed": 0,
    "importedCount": 0,
    "updatedCount": 0,
    "skippedCount": 0,
    "total_processed": 0,
    "imported_count": 0,
    "updated_count": 0,
    "skipped_count": 0,
    "errors": ["Missing question 'title' on row 3", "Invalid difficulty 'Extreme'"]
  }
  ```

#### `GET /api/user/export/markdown`
Export the authenticated user's question catalog as a human-readable Markdown study sheet.
- **Authentication**: Required (`401 Unauthorized` if unauthenticated).
- **HTTP Method**: `GET`
- **Response Headers**:
  - `Content-Type: text/markdown; charset=utf-8`
  - `Content-Disposition: attachment; filename="codevault-questions.md"`
- **Response Body**:
  - UTF-8 Markdown document containing:
    - `# CodeVault Questions Export`
    - Catalog metadata header (Export Date, Total Questions, Generated by CodeVault).
    - Structured H2 sections for each question containing Topic, Difficulty, Status, Favorite, Platform, URL, Tags, Notes, and Spaced Revision metadata.
- **Owner Scoping**:
  - Strictly limited to questions owned by the authenticated user.

#### `GET /api/user/export/anki`
Export the authenticated user's question catalog as a 3-column TSV ready for one-click import into Anki or compatible flashcard apps.
- **Authentication**: Required (`401 Unauthorized` if unauthenticated).
- **HTTP Method**: `GET`
- **Response Headers**:
  - `Content-Type: text/tab-separated-values; charset=utf-8`
  - `Content-Disposition: attachment; filename="codevault-questions-anki.tsv"`
- **Response Body**:
  - UTF-8 tab-separated values (3 columns: `Front`, `Back`, `Tags`):
    - `Front`: Question title, topic, difficulty badge, company, and problem description formatted with clean HTML breaks (`<br>`).
    - `Back`: Solution notes, approaches, platform link, and status.
    - `Tags`: Space-delimited tags normalized for Anki tags field.
  - TSV fields are escaped against embedded tabs and raw newlines to ensure standard TSV parser compatibility.
- **Owner Scoping**:
  - Strictly limited to questions owned by the authenticated user.
- **Format Note**: Generates standard, cross-platform Anki-compatible TSV. Does not construct proprietary `.apkg` SQLite bundles.
