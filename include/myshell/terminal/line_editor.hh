#pragma once

#include "myshell/support/error.hh"
#include "myshell/support/result.hh"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace myshell::terminal {

/**
 * @brief Wraps BSD libedit to provide interactive line editing, persistent history,
 *        and pluggable tab-completion for the shell's REPL loop.
 *
 * Only one LineEditor should be constructed per process because libedit uses
 * global terminal state internally.
 *
 * Thread-safety: LineEditor itself is NOT thread-safe. Call read_line() only
 * from the main/interactive thread. add_to_history() and on_resize() may be
 * called from the signal-monitor thread provided external synchronisation is
 * used by the caller (the interactive loop already provides this).
 *
 * Non-interactive mode: When stdout is not a tty (detected via isatty(1)),
 * read_line() falls back to a plain getline() read and completion/history are
 * no-ops. This is important for script mode and piped input.
 */
class LineEditor {
public:
    /// Configuration bundle passed at construction time.
    struct Config {
        /// Program name forwarded to el_init(); appears in .editrc matches.
        std::string program_name{"myshell"};
        /// Absolute path to the history file (e.g. ~/.myshell_history).
        std::string history_file;
        /// Maximum number of history entries to keep in memory and on disk.
        size_t history_size{1000};
        /// Whether to enable tab-completion machinery.
        bool enable_completion{true};
    };

    /**
     * @brief Construct and initialise the line editor.
     * @param cfg  Configuration; history_file may be empty to disable persistence.
     */
    explicit LineEditor(Config cfg);

    /**
     * @brief Destructor — always restores terminal state, even on abnormal paths.
     *
     * Also invoked via an atexit handler registered at construction time to
     * provide best-effort terminal recovery on crash.
     */
    ~LineEditor();

    // Non-copyable, non-movable (libedit state is not relocatable).
    LineEditor(const LineEditor&) = delete;
    LineEditor& operator=(const LineEditor&) = delete;
    LineEditor(LineEditor&&) = delete;
    LineEditor& operator=(LineEditor&&) = delete;

    /**
     * @brief Read one logical line of input from the user.
     *
     * Blocks until the user presses Enter (or sends EOF/signal). The trailing
     * newline is stripped from the returned string.
     *
     * @param prompt  The rendered prompt string (may contain ANSI escapes wrapped
     *                in \[ \] libedit non-printing delimiters).
     * @return        The line text, or std::nullopt on EOF (Ctrl-D on empty line).
     *
     * @throws Nothing. All errors are absorbed; on unrecoverable libedit failure
     *         returns std::nullopt so the shell can exit cleanly.
     */
    [[nodiscard]] std::optional<std::string> read_line(std::string_view prompt);

    /**
     * @brief Append a successfully-executed command to the in-memory history ring.
     * @param line  The command text (must not be blank; caller is responsible).
     */
    void add_to_history(std::string_view line);

    /// Signature of a completion provider callback.
    /// @param prefix   The word to complete (everything left of the cursor on the
    ///                 current token boundary).
    /// @return         Sorted, deduplicated list of completion candidates.
    using CompletionCallback = std::function<std::vector<std::string>(std::string_view prefix)>;

    /**
     * @brief Register the completion callback invoked on Tab key press.
     *
     * Replaces any previously registered callback. The callback is called on
     * the interactive thread; it must not block for more than ~200 ms.
     *
     * @param cb  Callable matching CompletionCallback signature.
     */
    void set_completion_callback(CompletionCallback cb);

    /**
     * @brief Persist the current in-memory history to disk.
     * @return Ok on success; Err with ShellError::io_error on failure.
     */
    [[nodiscard]] support::Result<void, support::ShellError> save_history();

    /**
     * @brief Load history from disk into the in-memory ring.
     *
     * Entries read from disk are appended after any existing in-memory entries.
     * Excess entries beyond history_size are silently trimmed (oldest first).
     *
     * @return Ok on success; Err with ShellError::io_error if the file cannot
     *         be opened (missing file is treated as Ok — first run).
     */
    [[nodiscard]] support::Result<void, support::ShellError> load_history();

    /**
     * @brief Notify the editor of a terminal resize event (SIGWINCH).
     * @param new_cols  New terminal column count.
     * @param new_rows  New terminal row count.
     */
    void on_resize(int new_cols, int new_rows);

private:
    // libedit handle; allocated by el_init(), freed by el_end().
    void* el_{nullptr};
    // libedit history handle; allocated by history_init(), freed by history_end().
    void* hist_{nullptr};

    Config cfg_;
    bool is_tty_{false};
    CompletionCallback completion_cb_;

    // Initialise libedit bindings and history subsystem.
    void init_libedit();
    void init_history();
    void restore_terminal();
};

}  // namespace myshell::terminal
