// include/myshell/lexer/token.hh
// MyShell — Language Engineering subsystem
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace myshell {

/// @brief Enumeration of all token kinds produced by the MyShell lexer.
///
/// Ordering within each group is significant for range-checks; do not reorder
/// without updating the predicate helpers at the bottom of this header.
enum class TokenKind : uint8_t {
    // ── Word tokens ─────────────────────────────────────────────────────────
    WORD,             ///< Unquoted or quoted word
    ASSIGNMENT_WORD,  ///< name=value before the first bare word in a command
    NAME,             ///< Plain identifier (letters, digits, underscore; starts non-digit)
    IO_NUMBER,        ///< Integer immediately followed by a redirect operator

    // ── Operators ────────────────────────────────────────────────────────────
    PIPE,              ///< |
    AMPERSAND,         ///< &  (background / async)
    SEMICOLON,         ///< ;
    NEWLINE,           ///< \n (statement terminator)
    DOUBLE_AMPERSAND,  ///< &&
    DOUBLE_PIPE,       ///< ||
    DOUBLE_SEMICOLON,  ///< ;; (case arm terminator)
    LPAREN,            ///< (
    RPAREN,            ///< )
    LBRACE,            ///< {
    RBRACE,            ///< }

    // ── Redirect operators ───────────────────────────────────────────────────
    REDIRECT_IN,          ///< <
    REDIRECT_OUT,         ///< >
    REDIRECT_APPEND,      ///< >>
    REDIRECT_HEREDOC,     ///< <<
    REDIRECT_HERESTRING,  ///< <<<
    REDIRECT_DUP_IN,      ///< <&
    REDIRECT_DUP_OUT,     ///< >&
    REDIRECT_OUT_ERR,     ///< &>
    REDIRECT_APPEND_ERR,  ///< &>>
    REDIRECT_CLOBBER,     ///< >| (force-overwrite, Bash extension)

    // ── Reserved words ──────────────────────────────────────────────────────
    KW_IF,
    KW_THEN,
    KW_ELSE,
    KW_ELIF,
    KW_FI,
    KW_FOR,
    KW_DO,
    KW_DONE,
    KW_WHILE,
    KW_UNTIL,
    KW_CASE,
    KW_ESAC,
    KW_IN,
    KW_FUNCTION,
    KW_SELECT,
    KW_TIME,
    KW_COPROC,
    KW_BANG,  ///< !  (pipeline negation)

    // ── Special / meta ───────────────────────────────────────────────────────
    END_OF_INPUT,  ///< Logical end of input (EOF or explicit termination)
    ERROR,         ///< Malformed token — .value contains a diagnostic message
    COMMENT,       ///< # … (normally consumed; included when skipping is disabled)
};

// ── SourceLocation ──────────────────────────────────────────────────────────

/// @brief Lightweight position record within a source file or interactive line.
///
/// `filename` is a non-owning view into the string provided to the Lexer
/// constructor; its lifetime must exceed that of all tokens produced by
/// that Lexer instance.
struct SourceLocation {
    std::string_view filename{};  ///< Source file name (or "<input>" for interactive)
    uint32_t line{1};             ///< 1-based line number
    uint32_t column{1};           ///< 1-based column number
    uint32_t offset{0};           ///< Byte offset from start of source
};

// ── Token ────────────────────────────────────────────────────────────────────

/// @brief A single lexical token produced by the MyShell lexer.
///
/// @par Ownership
///   `value` and `raw` are owned by the token (heap-allocated `std::string`).
///   `location.filename` is a non-owning view.
///
/// @par Thread safety
///   Tokens are immutable after construction; reading from multiple threads is safe.
struct Token {
    TokenKind kind{TokenKind::ERROR};  ///< Classified kind
    std::string value{};               ///< Processed value (quote-stripped, escape-resolved)
    std::string raw{};                 ///< Exact source text that produced this token
    SourceLocation location{};         ///< Position of the first character of this token

    /// @brief Convenience: true when this token is a shell reserved word keyword.
    [[nodiscard]] bool is_keyword() const noexcept;

    /// @brief Convenience: true for any redirect operator.
    [[nodiscard]] bool is_redirect() const noexcept;

    /// @brief Convenience: true for WORD, ASSIGNMENT_WORD, NAME, IO_NUMBER.
    [[nodiscard]] bool is_word_like() const noexcept;

    /// @brief Human-readable token kind name (for diagnostics).
    [[nodiscard]] static std::string_view kind_name(TokenKind k) noexcept;
    [[nodiscard]] std::string_view kind_name() const noexcept {
        return kind_name(kind);
    }
};

// ── Inline predicate implementations ────────────────────────────────────────

inline bool Token::is_keyword() const noexcept {
    return kind >= TokenKind::KW_IF && kind <= TokenKind::KW_BANG;
}

inline bool Token::is_redirect() const noexcept {
    return kind >= TokenKind::REDIRECT_IN && kind <= TokenKind::REDIRECT_CLOBBER;
}

inline bool Token::is_word_like() const noexcept {
    return kind == TokenKind::WORD || kind == TokenKind::ASSIGNMENT_WORD ||
           kind == TokenKind::NAME || kind == TokenKind::IO_NUMBER;
}

}  // namespace myshell
