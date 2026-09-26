// src/lexer/lexer.cc
// MyShell — Language Engineering subsystem
// SPDX-License-Identifier: MIT

#include "myshell/lexer/lexer.hh"

#include <cctype>

namespace myshell {

Lexer::Lexer(std::string_view source, std::string_view filename) noexcept
    : source_(source)
    , filename_(filename) {
}

Token Lexer::next_token() {
    if (peeked_.has_value()) {
        Token tok = std::move(*peeked_);
        peeked_.reset();
        ++tokens_produced_;
        return tok;
    }
    Token tok = produce_token();
    ++tokens_produced_;
    return tok;
}

Token Lexer::peek_token() {
    if (!peeked_.has_value()) {
        peeked_ = produce_token();
    }
    return *peeked_;
}

bool Lexer::at_end() const noexcept {
    return pos_ >= source_.size() && !peeked_.has_value();
}

SourceLocation Lexer::current_location() const noexcept {
    return make_location();
}

SourceLocation Lexer::make_location() const noexcept {
    return SourceLocation{.filename = filename_,
                          .line = line_,
                          .column = col_,
                          .offset = static_cast<uint32_t>(pos_)};
}

char Lexer::current() const noexcept {
    if (pos_ >= source_.size())
        return '\0';
    return source_[pos_];
}

char Lexer::peek(std::size_t offset) const noexcept {
    if (pos_ + offset >= source_.size())
        return '\0';
    return source_[pos_ + offset];
}

char Lexer::advance() noexcept {
    if (pos_ >= source_.size())
        return '\0';
    char c = source_[pos_++];
    if (c == '\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    return c;
}

bool Lexer::match_ahead(std::string_view s) const noexcept {
    if (pos_ + s.size() > source_.size())
        return false;
    return source_.substr(pos_, s.size()) == s;
}

void Lexer::consume(std::string_view s) noexcept {
    for (std::size_t i = 0; i < s.size(); ++i) {
        advance();
    }
}

void Lexer::skip_whitespace() noexcept {
    while (pos_ < source_.size()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\\' && peek(1) == '\n') {
            // Line continuation
            advance();
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skip_to_newline() noexcept {
    while (pos_ < source_.size()) {
        char c = advance();
        if (c == '\n')
            break;
    }
    peeked_.reset();
}

std::string Lexer::collect_heredoc_body() {
    if (pending_heredocs_.empty())
        return {};
    HeredocEntry entry = std::move(pending_heredocs_.front());
    pending_heredocs_.erase(pending_heredocs_.begin());

    std::string body;
    std::string line;

    while (pos_ < source_.size()) {
        line.clear();
        while (pos_ < source_.size()) {
            char c = advance();
            if (c == '\n')
                break;
            line.push_back(c);
        }

        std::string_view check_line = line;
        if (entry.strip_tabs) {
            while (!check_line.empty() && check_line.front() == '\t') {
                check_line.remove_prefix(1);
            }
        }

        if (check_line == entry.delimiter) {
            break;
        }

        body.append(line);
        body.push_back('\n');
    }

    return body;
}

std::optional<TokenKind> Lexer::lookup_keyword(std::string_view word) noexcept {
    if (word == "if")
        return TokenKind::KW_IF;
    if (word == "then")
        return TokenKind::KW_THEN;
    if (word == "else")
        return TokenKind::KW_ELSE;
    if (word == "elif")
        return TokenKind::KW_ELIF;
    if (word == "fi")
        return TokenKind::KW_FI;
    if (word == "for")
        return TokenKind::KW_FOR;
    if (word == "do")
        return TokenKind::KW_DO;
    if (word == "done")
        return TokenKind::KW_DONE;
    if (word == "while")
        return TokenKind::KW_WHILE;
    if (word == "until")
        return TokenKind::KW_UNTIL;
    if (word == "case")
        return TokenKind::KW_CASE;
    if (word == "esac")
        return TokenKind::KW_ESAC;
    if (word == "in")
        return TokenKind::KW_IN;
    if (word == "function")
        return TokenKind::KW_FUNCTION;
    if (word == "select")
        return TokenKind::KW_SELECT;
    if (word == "time")
        return TokenKind::KW_TIME;
    if (word == "coproc")
        return TokenKind::KW_COPROC;
    if (word == "!")
        return TokenKind::KW_BANG;
    return std::nullopt;
}

Token Lexer::produce_token() {
    skip_whitespace();

    if (pos_ >= source_.size()) {
        return Token{
            .kind = TokenKind::END_OF_INPUT, .value = "", .raw = "", .location = make_location()};
    }

    SourceLocation loc = make_location();
    char c = current();

    if (c == '\n') {
        advance();
        return Token{.kind = TokenKind::NEWLINE, .value = "\n", .raw = "\n", .location = loc};
    }

    if (c == '#') {
        return scan_comment(loc);
    }

    // Check operator beginnings
    if (c == '|' || c == '&' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}' ||
        c == '<' || c == '>') {
        return scan_operator(loc);
    }

    return scan_word(loc);
}

Token Lexer::scan_comment(SourceLocation loc) {
    std::string comment;
    while (pos_ < source_.size() && current() != '\n') {
        comment.push_back(advance());
    }
    return Token{.kind = TokenKind::COMMENT, .value = comment, .raw = comment, .location = loc};
}

Token Lexer::scan_operator(SourceLocation loc) {
    if (match_ahead("&&")) {
        consume("&&");
        return Token{
            .kind = TokenKind::DOUBLE_AMPERSAND, .value = "&&", .raw = "&&", .location = loc};
    }
    if (match_ahead("||")) {
        consume("||");
        return Token{.kind = TokenKind::DOUBLE_PIPE, .value = "||", .raw = "||", .location = loc};
    }
    if (match_ahead(";;")) {
        consume(";;");
        return Token{
            .kind = TokenKind::DOUBLE_SEMICOLON, .value = ";;", .raw = ";;", .location = loc};
    }
    if (match_ahead("<<<")) {
        consume("<<<");
        return Token{
            .kind = TokenKind::REDIRECT_HERESTRING, .value = "<<<", .raw = "<<<", .location = loc};
    }
    if (match_ahead("<<-")) {
        consume("<<-");
        // Handled as heredoc with strip_tabs
        return Token{
            .kind = TokenKind::REDIRECT_HEREDOC, .value = "<<-", .raw = "<<-", .location = loc};
    }
    if (match_ahead("<<")) {
        consume("<<");
        return Token{
            .kind = TokenKind::REDIRECT_HEREDOC, .value = "<<", .raw = "<<", .location = loc};
    }
    if (match_ahead("<&")) {
        consume("<&");
        return Token{
            .kind = TokenKind::REDIRECT_DUP_IN, .value = "<&", .raw = "<&", .location = loc};
    }
    if (match_ahead(">&")) {
        consume(">&");
        return Token{
            .kind = TokenKind::REDIRECT_DUP_OUT, .value = ">&", .raw = ">&", .location = loc};
    }
    if (match_ahead("&>>")) {
        consume("&>>");
        return Token{
            .kind = TokenKind::REDIRECT_APPEND_ERR, .value = "&>>", .raw = "&>>", .location = loc};
    }
    if (match_ahead("&>")) {
        consume("&>");
        return Token{
            .kind = TokenKind::REDIRECT_OUT_ERR, .value = "&>", .raw = "&>", .location = loc};
    }
    if (match_ahead(">>")) {
        consume(">>");
        return Token{
            .kind = TokenKind::REDIRECT_APPEND, .value = ">>", .raw = ">>", .location = loc};
    }
    if (match_ahead(">|")) {
        consume(">|");
        return Token{
            .kind = TokenKind::REDIRECT_CLOBBER, .value = ">|", .raw = ">|", .location = loc};
    }

    char c = advance();
    std::string s(1, c);
    switch (c) {
    case '|':
        return Token{.kind = TokenKind::PIPE, .value = s, .raw = s, .location = loc};
    case '&':
        return Token{.kind = TokenKind::AMPERSAND, .value = s, .raw = s, .location = loc};
    case ';':
        return Token{.kind = TokenKind::SEMICOLON, .value = s, .raw = s, .location = loc};
    case '(':
        return Token{.kind = TokenKind::LPAREN, .value = s, .raw = s, .location = loc};
    case ')':
        return Token{.kind = TokenKind::RPAREN, .value = s, .raw = s, .location = loc};
    case '{':
        return Token{.kind = TokenKind::LBRACE, .value = s, .raw = s, .location = loc};
    case '}':
        return Token{.kind = TokenKind::RBRACE, .value = s, .raw = s, .location = loc};
    case '<':
        return Token{.kind = TokenKind::REDIRECT_IN, .value = s, .raw = s, .location = loc};
    case '>':
        return Token{.kind = TokenKind::REDIRECT_OUT, .value = s, .raw = s, .location = loc};
    default:
        return Token{
            .kind = TokenKind::ERROR, .value = "Unexpected operator", .raw = s, .location = loc};
    }
}

Token Lexer::scan_word(SourceLocation loc) {
    std::string val;
    std::string raw;

    while (pos_ < source_.size()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            break;
        if (c == '|' || c == '&' || c == ';' || c == '(' || c == ')' || c == '<' || c == '>')
            break;

        if (c == '\\') {
            advance();
            raw.push_back('\\');
            if (pos_ < source_.size()) {
                char next = advance();
                raw.push_back(next);
                val.push_back(next);
            }
        } else if (c == '\'') {
            advance();
            raw.push_back('\'');
            while (pos_ < source_.size() && current() != '\'') {
                char ch = advance();
                raw.push_back(ch);
                val.push_back(ch);
            }
            if (pos_ < source_.size() && current() == '\'') {
                raw.push_back(advance());
            }
        } else if (c == '"') {
            advance();
            raw.push_back('"');
            while (pos_ < source_.size() && current() != '"') {
                char ch = current();
                if (ch == '\\' &&
                    (peek(1) == '"' || peek(1) == '\\' || peek(1) == '$' || peek(1) == '`')) {
                    advance();
                    raw.push_back('\\');
                    char esc = advance();
                    raw.push_back(esc);
                    val.push_back(esc);
                } else {
                    advance();
                    raw.push_back(ch);
                    val.push_back(ch);
                }
            }
            if (pos_ < source_.size() && current() == '"') {
                raw.push_back(advance());
            }
        } else {
            advance();
            raw.push_back(c);
            val.push_back(c);
        }
    }

    Token tok{
        .kind = TokenKind::WORD, .value = std::move(val), .raw = std::move(raw), .location = loc};

    classify_word(tok);
    return tok;
}

void Lexer::classify_word(Token& tok) const {
    if (ctx_.keyword_reserved) {
        if (auto kw = lookup_keyword(tok.value)) {
            tok.kind = *kw;
            return;
        }
    }

    // Check IO_NUMBER: all digits, immediately preceding < or >
    bool all_digits = !tok.value.empty();
    for (char ch : tok.value) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            all_digits = false;
            break;
        }
    }
    if (all_digits && (current() == '<' || current() == '>')) {
        tok.kind = TokenKind::IO_NUMBER;
        return;
    }

    // Check ASSIGNMENT_WORD: NAME=...
    auto eq_pos = tok.value.find('=');
    if (eq_pos != std::string::npos && eq_pos > 0) {
        bool valid_name = true;
        char first = tok.value[0];
        if (!std::isalpha(static_cast<unsigned char>(first)) && first != '_') {
            valid_name = false;
        }
        for (std::size_t i = 1; i < eq_pos; ++i) {
            char ch = tok.value[i];
            if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_') {
                valid_name = false;
                break;
            }
        }
        if (valid_name) {
            tok.kind = TokenKind::ASSIGNMENT_WORD;
            return;
        }
    }

    // Check NAME
    bool is_name = !tok.value.empty();
    char first = tok.value[0];
    if (!std::isalpha(static_cast<unsigned char>(first)) && first != '_') {
        is_name = false;
    } else {
        for (std::size_t i = 1; i < tok.value.size(); ++i) {
            char ch = tok.value[i];
            if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_') {
                is_name = false;
                break;
            }
        }
    }
    if (is_name) {
        tok.kind = TokenKind::NAME;
    }
}

}  // namespace myshell
