# CodeVault — SQLite Persistence Architecture & Migration Guide

> **Document Version**: 1.0.0 (Stage 9 Foundation)  
> **Status**: Approved & Implemented  
> **Target Engine**: SQLite 3.49+ (Vendored Amalgamation, C99/C++17)

---

## 1. Executive Summary & Architecture Context

Prior to Stage 9, CodeVault persisted algorithmic problem entities into flat CSV text files (`data/questions.csv`) using an atomic-flush file strategy. While human-readable and zero-dependency, flat CSV storage exhibits fundamental concurrency and integrity limitations:
- Lack of row-level ACID transactions.
- Whole-file rewrite on any single field mutation ($O(N)$ disk write overhead).
- Susceptibility to file locking conflicts under simultaneous server and background worker access.
- Absence of indexing for direct key/tag lookups at scale.

**Stage 9 introduces an enterprise-grade, embedded SQLite persistence layer (`SqliteQuestionRepository`)** while maintaining 100% backward compatibility with the existing clean architecture domain models (`models::Question`), service interfaces (`IQuestionRepository`), REST API contracts, custom DSA implementations, and React UI.

```
       +--------------------------------------------------------+
       |               React 18 / TypeScript Web UI             |
       +---------------------------+----------------------------+
                                   | HTTP / REST (Port 8080)
       +---------------------------v----------------------------+
       |                 Native C++17 HTTP Server               |
       +---------------------------+----------------------------+
                                   |
       +---------------------------v----------------------------+
       |   Services: QuestionService, RevisionService, Stats    |
       +---------------------------+----------------------------+
                                   | IQuestionRepository
                 +-----------------+-----------------+
                 |                                   |
       +---------v----------+              +---------v----------+
       |  FileQuestionRepo  |              |  SqliteQuestionRepo|
       |  (Legacy CSV Mode) |              |  (Default ACID DB) |
       +---------+----------+              +---------+----------+
                 |                                   |
       +---------v----------+              +---------v----------+
       | data/questions.csv | <---Migration| data/codevault.db  |
       +--------------------+      Service +--------------------+
```

---

## 2. SQLite Schema Specification (v1)

The database schema is initialized with explicit schema versioning and secondary indexes tailored for CodeVault queries.

### 2.1 Metadata Table: `schema_version`
Tracks database schema migrations:
```sql
CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER PRIMARY KEY,
    applied_at INTEGER NOT NULL,
    description TEXT NOT NULL
);
```

### 2.2 Entity Table: `questions`
Stores all 18 domain attributes of `models::Question`:
```sql
CREATE TABLE IF NOT EXISTS questions (
    id TEXT PRIMARY KEY NOT NULL,
    title TEXT NOT NULL,
    description TEXT DEFAULT '',
    topic TEXT NOT NULL,
    difficulty TEXT NOT NULL,
    company TEXT DEFAULT '',
    platform TEXT NOT NULL,
    source_url TEXT DEFAULT '',
    status TEXT NOT NULL,
    is_favorite INTEGER NOT NULL DEFAULT 0,
    notes TEXT DEFAULT '',
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    last_practiced_at INTEGER NOT NULL DEFAULT 0,
    next_revision_at INTEGER NOT NULL DEFAULT 0,
    revision_priority INTEGER NOT NULL DEFAULT 2,
    tags TEXT DEFAULT '',
    owner_id TEXT DEFAULT 'local_user'
);
```

### 2.3 Secondary Indexes
Accelerates multi-criteria queries and range scans:
```sql
CREATE INDEX IF NOT EXISTS idx_questions_topic ON questions(topic);
CREATE INDEX IF NOT EXISTS idx_questions_difficulty ON questions(difficulty);
CREATE INDEX IF NOT EXISTS idx_questions_status ON questions(status);
CREATE INDEX IF NOT EXISTS idx_questions_company ON questions(company);
CREATE INDEX IF NOT EXISTS idx_questions_next_rev ON questions(next_revision_at);
CREATE INDEX IF NOT EXISTS idx_questions_owner ON questions(owner_id);
```

---

## 3. High-Performance Pragmas & ACID Transactions

`SqliteQuestionRepository` executes the following initialization pragmas:
- **`PRAGMA journal_mode = WAL;`**: Write-Ahead Logging enables concurrent readers without blocking writers and writers without blocking readers.
- **`PRAGMA synchronous = NORMAL;`**: Provides durability while reducing fsync disk latency in WAL mode.
- **`PRAGMA foreign_keys = ON;`**: Enforces relational constraints.
- **`PRAGMA busy_timeout = 5000;`**: Waits up to 5000ms on lock contention before returning busy errors.

### 3.1 Thread Safety & Re-entrancy
- The repository utilizes `std::recursive_mutex dbMutex_` to guard all public member functions.
- Recursive mutex guarantees thread safety while allowing nested internal calls (e.g., `executeTransaction` executing multiple calls to `save` or schema introspection).
- Safe RAII wrappers (`ScopedStmt`) prevent statement leaks on exceptions or early returns.

---

## 4. Automatic Migration Service (`MigrationService`)

CodeVault automatically transitions legacy installations to SQLite seamlessly on startup:

1. **Detection**: On application boot (`src/main.cpp`), the system checks if `data/codevault.db` is empty ($0$ rows) and `data/questions.csv` exists.
2. **Backup Generation**: A pristine timestamped backup copy is created at `data/questions.csv.bak` before any ingestion begins.
3. **RFC 4180 Parsing**: Handles quotes, escaped commas, newlines, and unicode text safely without data truncation.
4. **Validation & Normalization**: Validates mandatory fields (`id`, `title`, enum values). Malformed rows are logged and isolated without halting batch execution.
5. **Atomic Transaction Batch**: All parsed entities are ingested within a single `BEGIN TRANSACTION ... COMMIT` envelope using parameterized upsert statements (`INSERT ... ON CONFLICT(id) DO UPDATE SET ...`).
6. **Count Invariance Check**: The post-migration database row count is checked against the parsed count.
7. **Idempotence**: Subsequent runs skip existing IDs, preventing accidental overwrites.

---

## 5. Verification & Testing

The SQLite persistence layer is verified by dedicated automated tests:
- **Unit Tests**: 20 dedicated SQLite tests in `tests/unit/unit_tests.cpp` (Tests 201–220) verifying:
  - File initialization, schema creation, and versioning.
  - CRUD operations over all 18 domain attributes.
  - Transaction commit, rollback on failure, and exception safety.
  - CSV migration parsing, backup generation, malformed row tolerance, and idempotency.
  - Multi-service live wiring (`QuestionService`, `RevisionService`, `PracticeService`, `StatisticsService`).
  - HTTP Server diagnostics reporting SQLite storage metadata.
- **Live Process Restart Verification**: Proves that modifications via REST API (`PUT /api/questions/Q-1001`) survive complete backend server process shutdown and restart.

---

## 6. Multi-User Foundation & Schema v2 (Stage 9.2)

> **Important Boundary**: Authentication and user login are **NOT** implemented in this stage. Stage 9.2 establishes the database, domain, and owner-scoped data isolation foundation required for multi-tenant CodeVault.

### 6.1 User Domain Model (`models::User`)
The `User` domain model is independent of SQLite and contains no passwords:
- `id`: Unique user identifier (e.g., `local_user`, `usr_123`).
- `username`: Unique username for display and identification.
- `displayName`: Friendly name for UI presentation.
- `email`: Optional email address.
- `createdAt` / `updatedAt`: Unix epoch timestamps.
- `active`: Boolean activation flag.

### 6.2 Schema v2 Specification
Upgrades from Schema v1 to Schema v2:
1. **`users` Table**:
```sql
CREATE TABLE IF NOT EXISTS users (
    id TEXT PRIMARY KEY NOT NULL,
    username TEXT UNIQUE NOT NULL,
    display_name TEXT NOT NULL,
    email TEXT NOT NULL DEFAULT '',
    created_at INTEGER NOT NULL DEFAULT 0,
    updated_at INTEGER NOT NULL DEFAULT 0,
    active INTEGER NOT NULL DEFAULT 1
);
CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
```

2. **Foreign Key Constraint**:
`questions.owner_id` references `users(id)` with `PRAGMA foreign_keys = ON;`.

3. **Deterministic Local User**:
The default user record `('local_user', 'local_user', 'Local User', '', ...)` is seeded to preserve full backward compatibility with existing single-user local workflows and migrated questions.

### 6.3 Current User Abstraction (`ICurrentUserProvider`)
Services decouple from hardcoded owner IDs via `ICurrentUserProvider`:
- In Stage 9.2, `StaticCurrentUserProvider("local_user")` supplies the active identity.
- Future authentication stages will swap this with a session or token-aware provider without rewriting any service or repository logic.

### 6.4 Owner-Scoped Data Isolation Architecture
To prevent cross-tenant data leakage:
- `IQuestionRepository` specifies owner-aware operations: `findAllByOwner`, `findByIdForOwner`, `existsForOwner`, `saveForOwner`, `removeForOwner`, `countForOwner`.
- `QuestionService` filters all queries by `currentUserProvider_->getCurrentUserId()`.
- Mutating operations (`updateQuestion`, `deleteQuestion`) block modifications to questions owned by another user.
- In-memory data structures (`PrefixTrie` autocomplete and `MinHeap` spaced repetition queue) are dynamically scoped and rebuilt for the active user upon user change (`refreshUserScope()`).
- High-level services (`PracticeService`, `StatisticsService`, `Dashboard`) automatically inherit strict multi-user isolation.

---

## 7. Authentication & Session Foundation: Schema v3 (Stage 9.3)

Stage 9.3 upgrades CodeVault from static identity to real authenticated user identity with server-side sessions, Argon2id password hashing, and cookie management.

### 7.1 Schema v3 Specification
```sql
CREATE TABLE IF NOT EXISTS user_credentials (
    user_id TEXT PRIMARY KEY NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    password_hash TEXT NOT NULL,
    created_at INTEGER NOT NULL DEFAULT 0,
    updated_at INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS sessions (
    id TEXT PRIMARY KEY NOT NULL,
    user_id TEXT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token_hash TEXT NOT NULL UNIQUE,
    created_at INTEGER NOT NULL,
    expires_at INTEGER NOT NULL,
    revoked_at INTEGER DEFAULT NULL,
    last_seen_at INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_sessions_token_hash ON sessions(token_hash);
CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id);
```

### 7.2 Cryptographic Security Invariants
1. **Password Hashing (Argon2id)**:
   - Algorithm: Argon2id (OWASP recommended standard).
   - Parameters: $t=2$ iterations, $m=19456$ KiB (19 MiB), $p=1$ thread, salt length 16 bytes (CSPRNG generated), hash length 32 bytes.
   - Plaintext passwords and reversible encryptions are never stored.
   - Passwords and password hashes are never returned through any API endpoint.
2. **Session Token Architecture (Blake2b Hashing)**:
   - Client receives a cryptographically secure random token generated via Windows CryptoAPI (`CryptGenRandom`) with fallback to `std::random_device` (32 bytes entropy = 64 hex characters).
   - Server-side `sessions` table **never** stores raw session tokens. Only the 256-bit cryptographic digest `token_hash = blake2b_256(raw_token)` is persisted.
   - If the SQLite database file is leaked, active session tokens cannot be derived from stored hashes.

### 7.3 HTTP Cookie Security & Middleware
- Cookies are issued as: `codevault_session=<token>; Path=/; Max-Age=604800; HttpOnly; SameSite=Lax`.
- `HttpOnly` protects against XSS token exfiltration.
- `SameSite=Lax` prevents CSRF attacks for state-changing cross-origin requests.
- For local development on plain HTTP, the `Secure` attribute is omitted; when deployed behind HTTPS/TLS, the server can enable `Secure`.
- Logout is idempotent and invalidates the session in the database while clearing the browser cookie (`Max-Age=0`).

### 7.4 CurrentUserProvider Integration
For authenticated HTTP requests, `AuthenticatedCurrentUserProvider` wraps the resolved `models::User` and dynamically binds to `QuestionService`. Owner-scoped repositories and queries enforce strict multi-tenant data isolation. Unauthenticated requests to protected endpoints return HTTP 401 Unauthorized.

### 7.5 Scope Notice (Out of Scope for Stage 9.3)
- Cloud deployment, PostgreSQL, Redis
- OAuth / Google / GitHub / Apple SSO
- Email verification and password reset via email
- Role-based access control (RBAC) and admin dashboards

---

## 8. Account Management & Data Export Architecture (Stage 9.4)

Stage 9.4 implements self-service account management, multi-device session revocation, and personal data export.

### 8.1 Schema Compatibility & Zero Migration Invariant
Schema v3 designed in Stage 9.3 natively supports all Stage 9.4 capabilities without requiring schema version elevation to v4:
- `users`: `display_name`, `email`, and `updated_at` columns support in-place user profile updates (`PUT /api/auth/profile`).
- `user_credentials`: `password_hash` and `updated_at` columns store the updated Argon2id password hash upon `POST /api/auth/change-password`.
- `sessions`: `user_id`, `expires_at`, `revoked_at`, and `last_seen_at` columns together with indexes `idx_sessions_user_id` and `idx_sessions_token_hash` enable sub-millisecond retrieval of active sessions and atomic bulk revocation of other devices (`UPDATE sessions SET revoked_at = ? WHERE user_id = ? AND id != ? AND revoked_at IS NULL;`).

### 8.2 Data Export Scoping
Data export operations (`GET /api/user/export?format=json` and `GET /api/user/export?format=csv`) execute queries strictly through `QuestionService::getAllQuestions()`, inheriting `AuthenticatedCurrentUserProvider` scoping. Cross-tenant leakage is physically impossible, as foreign key constraints and repository filters only select records where `questions.owner_id = ?`.

### 8.3 Inactive & Revoked Session Lifecycle
- A session is active if and only if `revoked_at IS NULL` AND `expires_at > currentTimestamp`.
- Session revocation is deterministic: `revoked_at` is set to the current epoch timestamp.
- Revoked sessions are immediately rejected by authentication middleware.


