# ADR 003: Multi-Core Concurrency Architecture via Work-Stealing ThreadPool

## Status
Accepted

## Context
Shell tab completion and pathname/glob discovery can block user interactive feedback if executed sequentially on the main thread, especially when examining large filesystem trees, complex network filesystems, or extensive PATH lists.

## Decision
We introduce a structured `ThreadPool` powered by C++20 `std::jthread` with cooperative `std::stop_token` cancellation. Subsystems like `CompletionEngine` fan out async queries across independent candidate providers (`FilesystemProvider`, `BuiltinProvider`, `VariableProvider`, `CommandProvider`) concurrently and aggregate results deterministically within strict latency deadlines.

## Consequences
- Main interactive REPL thread remains responsive (< 50ms user-perceived completion).
- Predictable deterministic sorting and deduplication across completion candidates.
- Complete thread safety verified through continuous ThreadSanitizer builds.
