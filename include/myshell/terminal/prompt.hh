#pragma once

#include "myshell/platform/posix/environment.hh"

#include <string>
#include <string_view>

namespace myshell::terminal {

/**
 * @brief Renders PS1/PS2 prompt template strings into display-ready strings
 *        suitable for consumption by LineEditor::read_line().
 *
 * Supported escape sequences (Bash-compatible subset):
 *   \u   Current username (from passwd or $USER).
 *   \h   Hostname up to first dot.
 *   \H   Full hostname.
 *   \w   Current working directory, with $HOME abbreviated to ~.
 *   \W   Basename of the current working directory.
 *   \$   '#' if effective UID == 0, otherwise '$'.
 *   \n   Newline character.
 *   \!   Current history number (1-based).
 *   \[   Begin non-printing sequence (maps to \001 for libedit width accounting).
 *   \]   End non-printing sequence (maps to \002 for libedit width accounting).
 *   \e   ESC character (0x1B) — used for ANSI color sequences.
 *   \\   Literal backslash.
 *
 * ANSI colors: embed naturally inside \[ ... \] delimiters, e.g.:
 *   PS1='\[\e[1;32m\]\u@\h\[\e[0m\]:\w\$ '
 *
 * Thread-safety: render() is const and stateless beyond the constructor
 * arguments; it is safe to call from multiple threads provided the Environment
 * and ShellVariables objects are not concurrently mutated.
 */
class PromptRenderer {
public:
    /**
     * @brief Construct the renderer.
     * @param env   Shell environment (provides $HOME, $USER, $HOSTNAME etc.).
     * @param vars  Shell variable store (provides $PS1, $PS2, history counter).
     */
    explicit PromptRenderer(const support::Environment& env, const support::ShellVariables& vars);

    /**
     * @brief Expand a PS1 template string into a display-ready prompt.
     *
     * Unknown escape sequences are passed through verbatim so that future
     * expansions do not silently corrupt prompt strings.
     *
     * @param ps1_template  The raw PS1 value (e.g. from $PS1).
     * @return              The expanded, display-ready string.
     */
    [[nodiscard]] std::string render(std::string_view ps1_template) const;

    /**
     * @brief Return the rendered PS2 (continuation) prompt.
     *
     * Reads $PS2 from the variable store and expands it via render(). Falls
     * back to "> " if $PS2 is unset.
     *
     * @return Expanded PS2 string.
     */
    [[nodiscard]] std::string ps2() const;

private:
    const support::Environment& env_;
    const support::ShellVariables& vars_;

    // Helper: expand a single recognised escape character following backslash.
    // Returns the expansion string; called per character in render().
    [[nodiscard]] std::string expand_escape(char c) const;
};

}  // namespace myshell::terminal
