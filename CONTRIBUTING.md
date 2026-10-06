# Contributing to CodeVault

Thank you for contributing to **CodeVault**! Whether you are an undergraduate student, open-source enthusiast, or engineering contributor, following these guidelines ensures our codebase remains clean, maintainable, and production-ready.

---

## 1. Branch Strategy & Naming Conventions

All active feature development is branch-driven. Do not commit directly to the `main` branch.

Branch naming formats:
- `feature/<stage-number>-<short-description>` (e.g., `feature/stage1-foundation`, `feature/stage3-trie-engine`)
- `fix/<short-description>` (e.g., `fix/csv-delimiter-parsing`)
- `docs/<short-description>` (e.g., `docs/update-dsa-design`)
- `refactor/<short-description>` (e.g., `refactor/clean-min-heap`)

---

## 2. Commit Message Guidelines

We follow a lightweight variation of the Conventional Commits specification:

```
<type>(<scope>): <short imperative summary>

[optional body explaining motivation and context]
```

**Allowed Types:**
- `feat`: A new user-facing or architectural capability
- `fix`: A bug fix in existing logic
- `docs`: Documentation modifications or additions
- `refactor`: Code reorganization that does not alter external behavior
- `test`: Adding or correcting tests
- `chore`: Build system, CMake, or tooling updates

**Examples:**
- `feat(dsa): implement PrefixTrie with prefix search`
- `fix(persistence): handle quoted strings in CSV parser`
- `docs(arch): update high-level sequence diagrams`

---

## 3. Code Style & C++ Conventions

1. **Standard**: Modern C++17.
2. **Formatting**:
   - 4-space indentation (no hard tabs).
   - Maximum line width: 100 characters.
   - Braces `{}` on the same line for control statements (`if`, `for`, `while`) or on a new line for functions.
3. **Identifiers**:
   - Classes, Structs, Enums: `PascalCase` (e.g. `QuestionService`, `PrefixTrie`)
   - Functions & Methods: `camelCase` (e.g. `getQuestionById`, `insertPrefix`)
   - Local Variables: `snake_case` or `camelCase` (consistent within file)
   - Member Variables: `trailing_underscore_` (e.g. `repository_`, `capacity_`)
   - Constants & Macros: `UPPER_SNAKE_CASE` (e.g. `DEFAULT_BUFFER_SIZE`)
4. **Memory Management**:
   - Prefer RAII and stack allocation.
   - Use `std::unique_ptr` for exclusive ownership and `std::shared_ptr` when shared ownership is genuinely needed.
   - Raw pointers (`T*`) are strictly non-owning and primarily reserved for pointer traversal in custom DSA nodes.
   - Zero tolerance for memory leaks.

---

## 4. Testing Expectations

- Any new custom data structure or service method **must** include corresponding unit tests in `tests/unit/`.
- Ensure tests execute deterministically and run in under a few seconds.
- Run tests locally using the smoke test runner or CTest before submitting any changes.

---

## 5. Documentation Expectations

- If a change modifies the domain entity or storage format, update `DATA_MODEL.md`.
- If an architectural boundary or layer interaction shifts, update `ARCHITECTURE.md`.
- Significant engineering decisions must be documented in `DECISIONS.md`.
- Keep `TODO.md` updated as tasks transition from planned to completed.

---

## 6. Pull Request Process

1. Ensure the project builds cleanly with zero compiler warnings (`-Wall -Wextra`).
2. Run the test suite and verify all tests pass.
3. Open a Pull Request referencing the stage and task in `TODO.md`.
4. Provide a clear summary of changes, test commands executed, and verification output.
