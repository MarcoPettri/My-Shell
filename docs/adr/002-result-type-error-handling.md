# ADR 002: Error Handling Strategy via Result<T, E>

## Status
Accepted

## Context
Shell systems software frequently crosses boundaries between the main interactive loop, thread pool workers, and child processes across `fork()`/`execve()`. C++ exceptions crossing `fork()` boundaries, signal handlers, or fine-grained parser recovery loops often lead to undefined behavior or silent aborts.

## Decision
We implement a lightweight, non-throwing `Result<T, E>` algebraic error type modeled after `std::expected` / Rust's `Result`, encapsulating typed `ShellError` structures.

## Consequences
- Clean, compiler-enforced `[[nodiscard]]` error propagation.
- Immediate `errno` capture at syscall sites via `ShellError::from_errno()`.
- Zero exceptions in process-launching and hot execution pathways.
