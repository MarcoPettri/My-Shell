// src/parser/parser.cc
#include "myshell/parser/parser.hh"

namespace myshell {

Token Parser::current() {
    return lexer_.peek_token();
}

Token Parser::advance() {
    return lexer_.next_token();
}

bool Parser::match(TokenKind kind) {
    if (current().kind == kind) {
        advance();
        return true;
    }
    return false;
}

Result<ast::Script, ShellError> Parser::parse() {
    auto list_res = parse_list();
    if (!list_res.ok())
        return Result<ast::Script, ShellError>::err(list_res.error());

    return Result<ast::Script, ShellError>::ok(
        ast::Script{.commands = std::move(list_res.value())});
}

Result<ast::List, ShellError> Parser::parse_list() {
    ast::List list;

    while (current().kind != TokenKind::END_OF_INPUT) {
        // Skip leading newlines / semicolons
        while (current().kind == TokenKind::NEWLINE || current().kind == TokenKind::SEMICOLON) {
            advance();
        }
        if (current().kind == TokenKind::END_OF_INPUT)
            break;

        auto and_or_res = parse_and_or();
        if (!and_or_res.ok())
            return Result<ast::List, ShellError>::err(and_or_res.error());

        bool bg = false;
        if (current().kind == TokenKind::AMPERSAND) {
            bg = true;
            advance();
        } else if (current().kind == TokenKind::SEMICOLON || current().kind == TokenKind::NEWLINE) {
            advance();
        }

        list.items.push_back(
            ast::ListItem{.and_or = std::move(and_or_res.value()), .background = bg});
    }

    return Result<ast::List, ShellError>::ok(std::move(list));
}

Result<ast::AndOrList, ShellError> Parser::parse_and_or() {
    auto pipe_res = parse_pipeline();
    if (!pipe_res.ok())
        return Result<ast::AndOrList, ShellError>::err(pipe_res.error());

    ast::AndOrList and_or;
    and_or.first = std::move(pipe_res.value());

    while (current().kind == TokenKind::DOUBLE_AMPERSAND ||
           current().kind == TokenKind::DOUBLE_PIPE) {
        ast::AndOrOp op =
            (current().kind == TokenKind::DOUBLE_AMPERSAND) ? ast::AndOrOp::And : ast::AndOrOp::Or;
        advance();
        auto next_pipe = parse_pipeline();
        if (!next_pipe.ok())
            return Result<ast::AndOrList, ShellError>::err(next_pipe.error());
        and_or.rest.push_back(ast::AndOrItem{.op = op, .pipeline = std::move(next_pipe.value())});
    }

    return Result<ast::AndOrList, ShellError>::ok(std::move(and_or));
}

Result<ast::Pipeline, ShellError> Parser::parse_pipeline() {
    ast::Pipeline pipeline;
    if (current().kind == TokenKind::KW_BANG) {
        pipeline.negated = true;
        advance();
    }

    auto cmd_res = parse_simple_command();
    if (!cmd_res.ok())
        return Result<ast::Pipeline, ShellError>::err(cmd_res.error());
    pipeline.commands.push_back(std::move(cmd_res.value()));

    while (current().kind == TokenKind::PIPE) {
        advance();
        auto next_cmd = parse_simple_command();
        if (!next_cmd.ok())
            return Result<ast::Pipeline, ShellError>::err(next_cmd.error());
        pipeline.commands.push_back(std::move(next_cmd.value()));
    }

    return Result<ast::Pipeline, ShellError>::ok(std::move(pipeline));
}

Result<ast::SimpleCommand, ShellError> Parser::parse_simple_command() {
    ast::SimpleCommand cmd;
    cmd.location = current().location;

    bool had_element = false;

    while (true) {
        Token tok = current();
        if (tok.kind == TokenKind::ASSIGNMENT_WORD && cmd.words.empty()) {
            auto eq = tok.value.find('=');
            cmd.assignments.push_back(ast::Assignment{.name = tok.value.substr(0, eq),
                                                      .value = tok.value.substr(eq + 1),
                                                      .location = tok.location});
            advance();
            had_element = true;
        } else if (tok.is_redirect() || tok.kind == TokenKind::IO_NUMBER) {
            auto redir_res = parse_redirect();
            if (!redir_res.ok())
                return Result<ast::SimpleCommand, ShellError>::err(redir_res.error());
            cmd.redirects.push_back(std::move(redir_res.value()));
            had_element = true;
        } else if (tok.is_word_like()) {
            cmd.words.push_back(tok.value);
            advance();
            had_element = true;
        } else {
            break;
        }
    }

    if (!had_element) {
        return Result<ast::SimpleCommand, ShellError>::err(ShellError::make(
            ErrorKind::ParseError, "Expected command word, assignment, or redirection"));
    }

    return Result<ast::SimpleCommand, ShellError>::ok(std::move(cmd));
}

Result<ast::Redirect, ShellError> Parser::parse_redirect() {
    ast::Redirect redir;
    redir.location = current().location;

    if (current().kind == TokenKind::IO_NUMBER) {
        redir.fd = std::stoi(current().value);
        advance();
    }

    Token op_tok = current();
    switch (op_tok.kind) {
    case TokenKind::REDIRECT_IN:
        redir.op = ast::RedirectOp::Input;
        break;
    case TokenKind::REDIRECT_OUT:
        redir.op = ast::RedirectOp::Output;
        break;
    case TokenKind::REDIRECT_APPEND:
        redir.op = ast::RedirectOp::Append;
        break;
    case TokenKind::REDIRECT_HEREDOC:
        redir.op = ast::RedirectOp::Heredoc;
        break;
    case TokenKind::REDIRECT_HERESTRING:
        redir.op = ast::RedirectOp::Herestring;
        break;
    case TokenKind::REDIRECT_DUP_IN:
        redir.op = ast::RedirectOp::DupInput;
        break;
    case TokenKind::REDIRECT_DUP_OUT:
        redir.op = ast::RedirectOp::DupOutput;
        break;
    case TokenKind::REDIRECT_OUT_ERR:
        redir.op = ast::RedirectOp::OutputAndError;
        break;
    case TokenKind::REDIRECT_APPEND_ERR:
        redir.op = ast::RedirectOp::AppendAndError;
        break;
    case TokenKind::REDIRECT_CLOBBER:
        redir.op = ast::RedirectOp::Clobber;
        break;
    default:
        return Result<ast::Redirect, ShellError>::err(
            ShellError::make(ErrorKind::ParseError, "Expected redirect operator"));
    }
    advance();

    // Target
    if (!current().is_word_like()) {
        return Result<ast::Redirect, ShellError>::err(
            ShellError::make(ErrorKind::ParseError, "Expected redirect target"));
    }
    redir.target = current().value;
    advance();

    return Result<ast::Redirect, ShellError>::ok(std::move(redir));
}

}  // namespace myshell
