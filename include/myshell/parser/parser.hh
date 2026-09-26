// include/myshell/parser/parser.hh
#pragma once

#include "myshell/lexer/lexer.hh"
#include "myshell/parser/ast.hh"
#include "myshell/support/error.hh"
#include "myshell/support/result.hh"

#include <memory>
#include <vector>

namespace myshell {

class Parser {
public:
    explicit Parser(Lexer& lexer)
        : lexer_(lexer) {
    }

    Result<ast::Script, ShellError> parse();

private:
    Lexer& lexer_;

    Token current();
    Token advance();
    bool match(TokenKind kind);
    bool expect(TokenKind kind, std::string_view errMsg);

    Result<ast::List, ShellError> parse_list();
    Result<ast::AndOrList, ShellError> parse_and_or();
    Result<ast::Pipeline, ShellError> parse_pipeline();
    Result<ast::SimpleCommand, ShellError> parse_simple_command();
    Result<ast::Redirect, ShellError> parse_redirect();
};

}  // namespace myshell
