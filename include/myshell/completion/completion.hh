#pragma once

#include "myshell/support/result.hh"

#include <chrono>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

// Forward declarations to keep this header self-contained and fast to compile.
namespace myshell::concurrency {
class ThreadPool;
}
namespace myshell::platform {
class Environment;
}
namespace myshell::builtins {
class BuiltinRegistry;
}

namespace myshell::support {
using Environment = myshell::platform::Environment;
using BuiltinRegistry = myshell::builtins::BuiltinRegistry;
}  // namespace myshell::support

namespace myshell::completion {

// ---------------------------------------------------------------------------
// ICompletionProvider — interface every provider must implement
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for a single tab-completion data source.
 *
 * Implementations must be:
 *   - Const-correct (get_candidates is const).
 *   - Cancellable: poll stop.stop_requested() and return early when set.
 *   - Thread-safe with respect to concurrent calls (the engine may call the
 *     same provider from multiple threads if re-used across engine instances,
 *     though in practice each engine owns its own set of providers).
 *   - Non-blocking: never wait on locks held by the interactive thread.
 */
class ICompletionProvider {
public:
    virtual ~ICompletionProvider() = default;

    /**
     * @brief Stable, human-readable identifier (e.g. "filesystem", "builtin").
     * @return Name string_view whose lifetime is at least as long as the provider.
     */
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /**
     * @brief Generate candidate completions for the given prefix.
     *
     * @param prefix   The partial token to complete (may be empty for "all").
     * @param context  The full current input line (for context-aware providers).
     * @param stop     Cancellation token; poll regularly and return early if set.
     * @return         List of candidate strings. Order is unspecified; the engine
     *                 deduplicates and sorts across all providers.
     */
    [[nodiscard]] virtual std::vector<std::string> get_candidates(std::string_view prefix,
                                                                  std::string_view context,
                                                                  std::stop_token stop) const = 0;
};

// ---------------------------------------------------------------------------
// CompletionEngine — merges results from all registered providers
// ---------------------------------------------------------------------------

/**
 * @brief Orchestrates parallel tab-completion across multiple ICompletionProvider
 *        implementations using the shell's thread pool.
 *
 * Each call to complete() fans out to all registered providers in parallel,
 * collects results within the given timeout, then merges, deduplicates, and
 * sorts them deterministically before returning.
 *
 * If the timeout expires before all providers finish, partial results from
 * the providers that did complete are returned; running providers receive a
 * cancellation signal via std::stop_token.
 *
 * Thread-safety: complete() is safe to call from the interactive thread.
 * add_provider() must NOT be called concurrently with complete().
 */
class CompletionEngine {
public:
    /**
     * @brief Construct a CompletionEngine.
     * @param pool      The shell's worker thread pool for parallel fan-out.
     * @param env       Shell environment (used by providers that need PATH etc.).
     * @param builtins  Builtin registry for BuiltinProvider lookups.
     */
    CompletionEngine(concurrency::ThreadPool& pool, const support::Environment& env,
                     const support::BuiltinRegistry& builtins);

    ~CompletionEngine();

    // Non-copyable; non-movable due to reference members.
    CompletionEngine(const CompletionEngine&) = delete;
    CompletionEngine& operator=(const CompletionEngine&) = delete;
    CompletionEngine(CompletionEngine&&) = delete;
    CompletionEngine& operator=(CompletionEngine&&) = delete;

    /**
     * @brief Register a completion provider.
     *
     * Providers are queried in registration order for determinism in the
     * merged, deduplicated output. Must be called before the first complete().
     *
     * @param provider  Unique ownership transferred to the engine.
     */
    void add_provider(std::unique_ptr<ICompletionProvider> provider);

    /**
     * @brief Run all registered providers and return merged completions.
     *
     * Guarantees:
     *   - Returned list is sorted lexicographically.
     *   - Duplicates are removed.
     *   - Providers are run in parallel on the thread pool.
     *   - Returns within timeout + small scheduling overhead.
     *
     * @param prefix   Partial word to complete.
     * @param context  Full current input line.
     * @param timeout  Maximum time to wait for all providers (default 200 ms).
     * @return         Sorted, deduplicated list of candidates.
     */
    [[nodiscard]] std::vector<std::string>
    complete(std::string_view prefix, std::string_view context,
             std::chrono::milliseconds timeout = std::chrono::milliseconds{200});

private:
    concurrency::ThreadPool& pool_;
    const support::Environment& env_;
    const support::BuiltinRegistry& builtins_;

    std::vector<std::unique_ptr<ICompletionProvider>> providers_;
};

// ---------------------------------------------------------------------------
// Standard provider declarations (implemented in separate .cc files)
// ---------------------------------------------------------------------------

/**
 * @brief Completes filenames relative to the current working directory,
 *        and executable names found anywhere on $PATH.
 *
 * Respects std::stop_token: directory iteration exits early on cancellation.
 */
class FilesystemProvider final : public ICompletionProvider {
public:
    explicit FilesystemProvider(const support::Environment& env);
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::vector<std::string> get_candidates(std::string_view prefix,
                                                          std::string_view context,
                                                          std::stop_token stop) const override;

private:
    const support::Environment& env_;
};

/**
 * @brief Returns the names of all registered shell builtins matching prefix.
 */
class BuiltinProvider final : public ICompletionProvider {
public:
    explicit BuiltinProvider(const support::BuiltinRegistry& builtins);
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::vector<std::string> get_candidates(std::string_view prefix,
                                                          std::string_view context,
                                                          std::stop_token stop) const override;

private:
    const support::BuiltinRegistry& builtins_;
};

/**
 * @brief Returns $-prefixed variable names from the current environment that
 *        match prefix (prefix must start with '$' for any results to be emitted).
 */
class VariableProvider final : public ICompletionProvider {
public:
    explicit VariableProvider(const support::Environment& env);
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::vector<std::string> get_candidates(std::string_view prefix,
                                                          std::string_view context,
                                                          std::stop_token stop) const override;

private:
    const support::Environment& env_;
};

/**
 * @brief Scans all directories listed in $PATH for executables whose basename
 *        begins with prefix.  Results are cached per PATH value and per prefix
 *        for the lifetime of the provider instance.
 */
class CommandProvider final : public ICompletionProvider {
public:
    explicit CommandProvider(const support::Environment& env);
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::vector<std::string> get_candidates(std::string_view prefix,
                                                          std::string_view context,
                                                          std::stop_token stop) const override;

private:
    const support::Environment& env_;
    // Simple cache: last PATH → sorted executable list.
    mutable std::string cached_path_;
    mutable std::vector<std::string> cached_executables_;
    mutable std::mutex cache_mutex_;
};

}  // namespace myshell::completion
