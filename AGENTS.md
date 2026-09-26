# AGENTS.md — MyShell Project Engineering Rules

This file is read by every agent (human or AI) contributing to the MyShell project.
It must be consulted before making any non-trivial change.

---

## Project Purpose

MyShell is a production-quality POSIX-style interactive shell and scripting engine implemented
in C++20. Its primary goals are:

1. Correctness — correct POSIX/Bash-compatible shell semantics for the implemented language subset
2. Safety — RAII everywhere, no UB, clean sanitizers
3. Concurrency — measurable, semantically correct multi-core parallelism
4. Maintainability — modular architecture, clean interfaces, tested
5. Observability — structured logging, diagnostics, profiling hooks

This is NOT a toy shell. Every design decision must be justifiable to a demanding production
code review.

---

## Supported Platforms

- **Primary**: Linux/POSIX (x86-64)
- **Secondary**: macOS (where abstractions are sufficient — no POSIX-breaking assumptions)
- **Not supported**: Windows (do not contaminate POSIX core with Windows code)

---

## C++ Standard

- **Mandatory baseline**: C++20
- C++23 features may be used ONLY with a documented compatibility comment explaining what
  older-compiler fallback exists.
- Never silently upgrade to C++23-only without an ADR.

---

## Build Commands

```bash
# Configure (debug)
cmake --preset debug

# Build
cmake --build --preset debug

# Test
ctest --preset debug --output-on-failure

# Format check
cmake --build --preset debug --target format-check

# Format apply
cmake --build --preset debug --target format

# Clang-tidy
cmake --build --preset debug --target tidy

# ASan build
cmake --preset asan && cmake --build --preset asan

# UBSan build
cmake --preset ubsan && cmake --build --preset ubsan

# TSan build
cmake --preset tsan && cmake --build --preset tsan

# Benchmark build
cmake --preset benchmark && cmake --build --preset benchmark
```

All build artifacts go under `build/<preset>/`. Never commit build artifacts.

---

## Directory Ownership

| Directory | Owner subsystem | May be edited by |
|-----------|----------------|-----------------|
| `src/support/` | Core | Any engineer with ADR for new primitives |
| `src/platform/` | Systems Eng | Systems Eng, reviewed by Security Eng |
| `src/concurrency/` | Perf Eng | Perf Eng, reviewed by Systems Eng |
| `src/lexer/` | Language Eng | Language Eng |
| `src/parser/` | Language Eng | Language Eng |
| `src/expansion/` | Language Eng | Language Eng |
| `src/execution/` | Systems Eng | Systems Eng |
| `src/jobs/` | Systems Eng | Systems Eng |
| `src/builtins/` | Systems Eng | Systems Eng |
| `src/terminal/` | UX Eng | UX Eng |
| `src/history/` | UX Eng | UX Eng |
| `src/completion/` | UX Eng, Perf Eng | Both |
| `src/app/` | Integrator | Integrator |
| `tests/` | Test Eng | Any engineer for their subsystem |
| `benchmarks/` | Benchmark Eng | Benchmark Eng |
| `docs/` | All | All, reviewed by Docs Eng |
| `cmake/` | Build Eng | Build Eng |

---

## Architecture Invariants

1. **No circular dependencies** between modules. Module dependency order:
   `support → platform → concurrency → lexer → parser → expansion → execution → jobs → builtins → terminal/history/completion → app`

2. **AST is immutable** after construction. Never mutate AST nodes post-parse.

3. **No raw platform calls outside `src/platform/`**. All fork/exec/waitpid/signal/ioctl calls
   go through platform abstractions.

4. **No `system()` or `popen()`** anywhere. Ever.

5. **Every Result<> error must be handled at the call site** or explicitly propagated with
   a comment explaining why.

6. **Signal handlers** must contain only async-signal-safe code. Use the signal monitor thread
   pattern (signalfd/sigwaitinfo) for all complex signal handling.

---

## Concurrency Invariants

1. **Named threads only**. Every `std::jthread` must have a descriptive name set via
   `pthread_setname_np` within 50ms of creation.

2. **No detached threads** unless documented with a compelling reason in a comment.

3. **Shutdown order**: Thread pool first, then job monitor, then signal monitor, then main.
   Never reverse this order.

4. **Lock ordering** (must be acquired in this order when multiple locks needed):
   `support_mtx < concurrency_mtx < jobs_mtx < terminal_mtx`
   Violating this order is a critical bug.

5. **Bounded queues** everywhere. No unbounded growth.

6. **Cancellation via `std::stop_token`**. Never use global flags or `pthread_cancel`.

7. **TSan clean** at all times. Any TSan finding blocks merge.

---

## Security Invariants

1. `FD_CLOEXEC` must be set on all file descriptors opened by the shell that are not
   intentionally inherited by child processes.

2. `execve` directly. Never shell out to `/bin/sh` for internal operations.

3. No TOCTOU: stat-then-open sequences must use `openat` / `O_PATH` guard patterns.

4. Temporary files: use `O_TMPFILE` or `mkstemp` only. Never predictable path + create.

5. Inherited FD audit: on startup, close or verify all FDs above stderr.

6. Never log user passwords, tokens, or secrets — even at TRACE level.

7. Test fixtures run in isolated tmpdir. Never touch `~` in tests.

---

## Error Handling Rules

1. Use `Result<T, ShellError>` (or equivalent) for all expected failure paths.
2. Use exceptions only for programmer-error / unrecoverable paths.
3. Never cross process/exec boundaries with exceptions.
4. User-visible errors go to stderr via the Logger, never via `std::cerr` directly.
5. Every error must include context (which operation, on what resource).
6. `errno` must be captured immediately after a failing syscall.

---

## Code Style

- Formatting: enforced by `.clang-format` (LLVM base, 100 column limit)
- Every PR/commit must pass `format-check` CMake target
- Naming: `snake_case` for everything; `UPPER_SNAKE` for macros/constants
- Headers: `#pragma once` (project convention); include guards not used
- Include order: own header first, then project headers, then system headers, then stdlib
- `[[nodiscard]]` on all `Result<>` returns and any function whose return value must be checked
- `noexcept` on functions that are genuinely noexcept (verified, not aspirational)

---

## Documentation Requirements

1. Every public API must have a Doxygen comment with:
   - `@brief`
   - `@param` for non-obvious parameters
   - `@return` / `@throws` (or note that it doesn't throw)
   - Thread-safety note if not obvious
   - Ownership semantics if non-trivial

2. Every ADR must be in `docs/adr/` with format `NNN-title.md`.

3. Every significant performance claim requires a benchmark result file reference.

4. Architecture documents must be kept current; stale diagrams are bugs.

---

## "Never Do" Rules

- Never call `system()`, `popen()`, or `fork()` outside `src/platform/`
- Never use `printf`/`fprintf` for internal logging; use the Logger subsystem
- Never use raw `new`/`delete` for owning pointers; use `std::make_unique`
- Never use `std::shared_ptr` without documenting why shared ownership is required
- Never silently swallow `Result<>` errors
- Never block the main/interactive thread for > 5ms without justification
- Never add a dependency without an ADR and license review
- Never invent benchmark numbers; never hard-code performance claims
- Never commit with sanitizer findings active
- Never use `volatile` for threading; use `std::atomic`
- Never use `pthread_cancel`; use `std::stop_token` cooperative cancellation
- Never skip tests for "obvious" code; obvious code has bugs too

---

## Definition of Done (per feature)

- [ ] Design exists (ADR if significant)
- [ ] Implementation complete with no TODO comments for essential functionality
- [ ] Unit tests exist and pass
- [ ] Integration tests exist and pass
- [ ] Static analysis (clang-tidy) clean
- [ ] ASan + UBSan clean
- [ ] Documentation (Doxygen + markdown) updated
- [ ] Performance implications understood and measured if in hot path
- [ ] Security implications reviewed
- [ ] Code review performed

---

## Agent Roster

See `.agents/agents/` for specialized agent definitions.

| Agent | Role |
|-------|------|
| `architect` | System architecture, ADRs, subsystem boundaries |
| `cpp-systems-engineer` | Platform layer, execution, job control |
| `parser-language-engineer` | Lexer, parser, AST, grammar, expansion |
| `concurrency-performance-engineer` | Thread pool, parallel workloads, benchmarks |
| `security-engineer` | Threat model, FD safety, signal safety |
| `test-engineer` | Test infrastructure, fuzzing, regression |
| `benchmark-engineer` | Benchmark harness, profiling, results |
| `documentation-engineer` | Docs, ADRs, user guide, API docs |
| `code-reviewer` | Adversarial review of integrated code |
