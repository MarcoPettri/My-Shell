# ADR 001: Adoption of C++20 as Mandatory Baseline

## Status
Accepted

## Context
MyShell is a modern, enterprise-grade POSIX interactive shell designed for high reliability, maintainability, and measurable multi-core concurrency. C++20 introduces major language features (`std::jthread`, `std::stop_token`, concepts, three-way comparisons, spans, string_views, and designated initializers) that drastically improve safe concurrency, ownership semantics, and performance without macro-heavy metaprogramming.

## Decision
We adopt C++20 as the mandatory baseline standard across all targets. Features from C++23 or later may only be used with explicit fallback strategies and documented ADRs.

## Consequences
- Guaranteed availability of `std::jthread` with cooperative cancellation via `std::stop_token`.
- Native compile-time concept checking and strict typing.
- Clear RAII primitives preventing resource leaks across complex signal and execution paths.
