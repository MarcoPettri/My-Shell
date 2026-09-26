// include/myshell/lexer/lexer.hh
// MyShell — Language Engineering subsystem
// SPDX-License-Identifier: MIT
#pragma once

#include "myshell/lexer/token.hh"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace myshell {

/// @brief Context flags that influence tokenisation rules.
///
/// Several shell constructs (e.g. `case`, function arguments, arithmetic)
/// require the lexer to behave differently.  The Parser drives these flags.
struct LexerContext {
    bool in_case_pattern{false};  ///< Expect `;;` / `;&` as terminators
    bool in_arithmetic{false};    ///< Inside $(( … )) — different quoting rules
    bool keyword_reserved{true};  ///< Whether reserved words should be recognised
};

// ── Heredoc descriptor ──────────────────────────────────────────────────────

/// @brief Describes a pending heredoc whose body has not yet been consumed.
struct HeredocEntry {
    std::string delimiter;    ///< The raw delimiter token (before strip processing)
    bool strip_tabs;          ///< True when `<<-` was used
    bool quoted;              ///< True when delimiter was quoted (no expansion in body)
    SourceLocation location;  ///< Location of the `<<` operator for diagnostics
};

// ── Lexer ────────────────────────────────────────────────────────────────────

/// @brief Lazy, resumable tokeniser for MyShell source input.
///
/// The lexer operates on a `std::string_view` of the complete source text
/// (or the current interactive line).  It produces tokens on demand via
/// `next_token()` without pre-building the full token stream.
///
/// @par Error handling
///   The lexer **never throws** on malformed input.  All error conditions are
///   returned as `Token{TokenKind::ERROR, <message>}`.
///
/// @par Heredoc collection
///   After a `<<DELIM` operator is scanned, the body is not collected inline.
///   Instead an entry is pushed into the pending-heredoc queue.  The Parser is
///   responsible for calling `collect_heredoc_body()` at the appropriate point
///   (after the newline that ends the command line containing the `<<`).
///
/// @par Thread safety
///   A Lexer instance is **not** thread-safe; external synchronisation is
///   required if shared across threads (in practice, only one thread drives a Lexer).
class Lexer {
public:
    /// @brief Construct a lexer over a complete source buffer.
    /// @param source   Full text to lex.  The caller owns the string; the
    ///                 lexer holds a view — the source must outlive the Lexer.
    /// @param filename Used in SourceLocation records; typically a path or
    ///                 "<input>" for interactive lines.
    explicit Lexer(std::string_view source, std::string_view filename = "<input>") noexcept;

    // Non-copyable, movable
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer(Lexer&&) = default;
    Lexer& operator=(Lexer&&) = default;

    ~Lexer() = default;

    // ── Primary interface ─────────────────────────────────────────────────

    /// @brief Consume and return the next token.
    /// @return Next token; returns `END_OF_INPUT` repeatedly after source end.
    [[nodiscard]] Token next_token();

    /// @brief Return the next token without consuming it.
    /// @return Copy of the upcoming token.
    [[nodiscard]] Token peek_token();

    /// @brief True when the source has been fully consumed.
    [[nodiscard]] bool at_end() const noexcept;

    // ── Context control ───────────────────────────────────────────────────

    /// @brief Replace the active lexer context flags.
    void set_context(LexerContext ctx) noexcept {
        ctx_ = ctx;
    }

    /// @brief Read-only view of the current lexer context.
    [[nodiscard]] const LexerContext& context() const noexcept {
        return ctx_;
    }

    // ── Error recovery ────────────────────────────────────────────────────

    /// @brief Discard characters up to (and including) the next `\n` or EOF.
    ///
    /// Used by the parser for statement-level error recovery.
    void skip_to_newline() noexcept;

    // ── Heredoc support ───────────────────────────────────────────────────

    /// @brief True when there are pending heredoc bodies to collect.
    [[nodiscard]] bool has_pending_heredocs() const noexcept {
        return !pending_heredocs_.empty();
    }

    /// @brief Collect the body of the next pending heredoc from the current position.
    ///
    /// Should be called by the parser immediately after the NEWLINE that
    /// terminates the command line containing the `<<` operator.
    ///
    /// @return The collected body text, or an empty string if no pending heredoc.
    [[nodiscard]] std::string collect_heredoc_body();

    /// @brief Access the queue of pending heredoc entries (for the parser to inspect).
    [[nodiscard]] const std::vector<HeredocEntry>& pending_heredocs() const noexcept {
        return pending_heredocs_;
    }

    // ── Diagnostics / introspection ───────────────────────────────────────

    /// @brief Current source location (position of the *next* character to be read).
    [[nodiscard]] SourceLocation current_location() const noexcept;

    /// @brief Number of tokens produced so far (does not count peeked tokens).
    [[nodiscard]] std::size_t tokens_produced() const noexcept {
        return tokens_produced_;
    }

private:
    // Source and position
    std::string_view source_;
    std::string_view filename_;
    std::size_t pos_{0};
    uint32_t line_{1};
    uint32_t col_{1};

    // Context
    LexerContext ctx_{};

    // Lookahead cache
    std::optional<Token> peeked_;

    // Heredoc queue
    std::vector<HeredocEntry> pending_heredocs_;

    // Statistics
    std::size_t tokens_produced_{0};

    // ── Internal helpers ──────────────────────────────────────────────────

    /// Produce the next token from raw source.
    Token produce_token();

    /// Skip whitespace (spaces and tabs, not newlines).
    void skip_whitespace() noexcept;

    /// Advance position by one character, updating line/col counters.
    char advance() noexcept;

    /// Peek at current character without consuming.
    [[nodiscard]] char current() const noexcept;

    /// Peek N characters ahead without consuming.
    [[nodiscard]] char peek(std::size_t offset = 1) const noexcept;

    /// True if the upcoming sequence matches `s` exactly.
    [[nodiscard]] bool match_ahead(std::string_view s) const noexcept;

    /// Consume all characters in `s` (caller guarantees match).
    void consume(std::string_view s) noexcept;

    /// Build a SourceLocation for the current position.
    [[nodiscard]] SourceLocation make_location() const noexcept;

    // ── Token production helpers ──────────────────────────────────────────

    Token scan_word(SourceLocation loc);
    Token scan_operator(SourceLocation loc);
    Token scan_comment(SourceLocation loc);

    /// Attempt to classify a WORD token as ASSIGNMENT_WORD, NAME, IO_NUMBER, or keyword.
    void classify_word(Token& tok) const;

    // ── String / quoting helpers ──────────────────────────────────────────

    /// Scan a single-quoted string (verbatim).  Returns content (without quotes).
    std::string scan_single_quoted(SourceLocation loc, std::string& raw_out);

    /// Scan a double-quoted string (with expansion awareness for raw tracking).
    std::string scan_double_quoted(SourceLocation loc, std::string& raw_out);

    /// Scan a $(...) command substitution and return its raw text.
    std::string scan_command_substitution();

    /// Scan a $((...)) arithmetic expansion and return its raw text.
    std::string scan_arithmetic_expansion();

    /// Scan a ${...} parameter expansion and return its raw text.
    std::string scan_parameter_expansion();

    /// Apply backslash escape outside of quotes.
    char resolve_backslash_escape();

    // ── Keyword table ─────────────────────────────────────────────────────

    /// Returns the keyword token kind for `word`, or std::nullopt if not a keyword.
    [[nodiscard]] static std::optional<TokenKind> lookup_keyword(std::string_view word) noexcept;
};

}  // namespace myshell
