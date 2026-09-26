#pragma once

#include "myshell/support/error.hh"
#include "myshell/support/result.hh"

#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace myshell::history {

/**
 * @brief Thread-safe, bounded command history ring with filesystem persistence.
 *
 * History entries are stored in insertion order: index 0 is the oldest entry,
 * index size()-1 is the most recent. When the ring is full, the oldest entry
 * is evicted before adding the new one.
 *
 * Persistence format: one entry per line, UTF-8 text. Lines containing
 * embedded newlines are stored with the newline replaced by the literal
 * two-character sequence "\\n"; the reverse transformation is applied on load.
 *
 * Thread-safety: All public methods acquire the internal mutex and are safe
 * to call concurrently from any thread. The only exception is print(), which
 * holds the lock for the duration of the write — callers should avoid calling
 * it from time-critical paths.
 */
class History {
public:
    /**
     * @brief Construct a history store.
     * @param history_file  Path to the persistence file; may be empty to disable
     *                      persistence (useful in non-interactive / test mode).
     * @param max_size      Maximum number of entries; excess entries (oldest)
     *                      are silently removed. Must be >= 1.
     */
    explicit History(std::string history_file, size_t max_size = 1000);

    /// Non-copyable (mutex cannot be copied). Movable if caller ensures no
    /// concurrent access during the move.
    History(const History&) = delete;
    History& operator=(const History&) = delete;
    History(History&&) noexcept;
    History& operator=(History&&) noexcept;

    ~History() = default;

    /**
     * @brief Append an entry to the history ring.
     *
     * If the entry is identical to the most recent entry it is deduplicated
     * (not added again). Blank entries are silently ignored.
     *
     * @param entry  The command string to record.
     */
    void add(std::string entry);

    /**
     * @brief Remove all entries from the in-memory ring.
     *
     * Does NOT delete the persistence file; call save() afterward to persist
     * the cleared state.
     */
    void clear();

    /**
     * @brief Remove a single entry by index.
     * @param index  0-based index (0 = oldest). Out-of-range is a no-op.
     */
    void delete_entry(size_t index);

    /**
     * @brief Look up an entry by index.
     * @param index  0-based index (0 = oldest, size()-1 = newest).
     * @return       A view into the stored string, or std::nullopt if out of range.
     * @note         The returned view is invalidated by any subsequent mutating call.
     */
    [[nodiscard]] std::optional<std::string_view> at(size_t index) const;

    /**
     * @brief Number of entries currently in the ring.
     * @return Entry count.
     */
    [[nodiscard]] size_t size() const noexcept;

    /**
     * @brief Find entries containing pattern as a substring.
     * @param pattern  The substring to search for (case-sensitive).
     * @return         Indices of matching entries in ascending order.
     */
    [[nodiscard]] std::vector<size_t> search(std::string_view pattern) const;

    /**
     * @brief Write the in-memory ring to the configured history file.
     * @return Ok on success; Err(ShellError::io_error) on file write failure.
     *         No-op (returns Ok) if history_file is empty.
     */
    [[nodiscard]] support::Result<void, support::ShellError> save() const;

    /**
     * @brief Load entries from the history file into the in-memory ring.
     *
     * Entries from the file are appended after any existing in-memory entries.
     * If the combined count exceeds max_size, the oldest entries are trimmed.
     * A missing file is treated as an empty history (returns Ok).
     *
     * @return Ok on success or missing file; Err(ShellError::io_error) on read failure.
     */
    [[nodiscard]] support::Result<void, support::ShellError> load();

    /**
     * @brief Write history to a file descriptor in the format used by the
     *        'history' builtin: "  N  command\n" (right-aligned index).
     *
     * @param fd  File descriptor to write to (typically STDOUT_FILENO).
     * @param n   If given, print only the n most-recent entries.
     */
    void print(int fd, std::optional<size_t> n = std::nullopt) const;

private:
    mutable std::mutex mutex_;
    std::deque<std::string> entries_;
    std::string file_;
    size_t max_size_;

    // Trim entries_ to at most max_size_, removing from the front (oldest).
    // REQUIRES: mutex_ is held by caller.
    void trim_to_limit_locked();
};

}  // namespace myshell::history
