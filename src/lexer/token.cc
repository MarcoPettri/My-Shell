// src/lexer/token.cc
// MyShell — Language Engineering subsystem
// SPDX-License-Identifier: MIT

#include "myshell/lexer/token.hh"

namespace myshell {

std::string_view Token::kind_name(TokenKind k) noexcept {
    switch (k) {
    case TokenKind::WORD:
        return "WORD";
    case TokenKind::ASSIGNMENT_WORD:
        return "ASSIGNMENT_WORD";
    case TokenKind::NAME:
        return "NAME";
    case TokenKind::IO_NUMBER:
        return "IO_NUMBER";
    case TokenKind::PIPE:
        return "PIPE";
    case TokenKind::AMPERSAND:
        return "AMPERSAND";
    case TokenKind::SEMICOLON:
        return "SEMICOLON";
    case TokenKind::NEWLINE:
        return "NEWLINE";
    case TokenKind::DOUBLE_AMPERSAND:
        return "DOUBLE_AMPERSAND";
    case TokenKind::DOUBLE_PIPE:
        return "DOUBLE_PIPE";
    case TokenKind::DOUBLE_SEMICOLON:
        return "DOUBLE_SEMICOLON";
    case TokenKind::LPAREN:
        return "LPAREN";
    case TokenKind::RPAREN:
        return "RPAREN";
    case TokenKind::LBRACE:
        return "LBRACE";
    case TokenKind::RBRACE:
        return "RBRACE";
    case TokenKind::REDIRECT_IN:
        return "REDIRECT_IN";
    case TokenKind::REDIRECT_OUT:
        return "REDIRECT_OUT";
    case TokenKind::REDIRECT_APPEND:
        return "REDIRECT_APPEND";
    case TokenKind::REDIRECT_HEREDOC:
        return "REDIRECT_HEREDOC";
    case TokenKind::REDIRECT_HERESTRING:
        return "REDIRECT_HERESTRING";
    case TokenKind::REDIRECT_DUP_IN:
        return "REDIRECT_DUP_IN";
    case TokenKind::REDIRECT_DUP_OUT:
        return "REDIRECT_DUP_OUT";
    case TokenKind::REDIRECT_OUT_ERR:
        return "REDIRECT_OUT_ERR";
    case TokenKind::REDIRECT_APPEND_ERR:
        return "REDIRECT_APPEND_ERR";
    case TokenKind::REDIRECT_CLOBBER:
        return "REDIRECT_CLOBBER";
    case TokenKind::KW_IF:
        return "KW_IF";
    case TokenKind::KW_THEN:
        return "KW_THEN";
    case TokenKind::KW_ELSE:
        return "KW_ELSE";
    case TokenKind::KW_ELIF:
        return "KW_ELIF";
    case TokenKind::KW_FI:
        return "KW_FI";
    case TokenKind::KW_FOR:
        return "KW_FOR";
    case TokenKind::KW_DO:
        return "KW_DO";
    case TokenKind::KW_DONE:
        return "KW_DONE";
    case TokenKind::KW_WHILE:
        return "KW_WHILE";
    case TokenKind::KW_UNTIL:
        return "KW_UNTIL";
    case TokenKind::KW_CASE:
        return "KW_CASE";
    case TokenKind::KW_ESAC:
        return "KW_ESAC";
    case TokenKind::KW_IN:
        return "KW_IN";
    case TokenKind::KW_FUNCTION:
        return "KW_FUNCTION";
    case TokenKind::KW_SELECT:
        return "KW_SELECT";
    case TokenKind::KW_TIME:
        return "KW_TIME";
    case TokenKind::KW_COPROC:
        return "KW_COPROC";
    case TokenKind::KW_BANG:
        return "KW_BANG";
    case TokenKind::END_OF_INPUT:
        return "END_OF_INPUT";
    case TokenKind::ERROR:
        return "ERROR";
    case TokenKind::COMMENT:
        return "COMMENT";
    }
    return "UNKNOWN";
}

}  // namespace myshell
