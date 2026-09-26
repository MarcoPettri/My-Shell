// include/myshell/parser/ast.hh
#pragma once

#include "myshell/lexer/token.hh"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace myshell::ast {

// Forward declarations
struct SimpleCommand;
struct Pipeline;
struct AndOrList;
struct List;
struct IfCommand;
struct WhileCommand;
struct ForCommand;
struct Script;

enum class RedirectOp {
    Input,           // <
    Output,          // >
    Append,          // >>
    Heredoc,         // <<
    Herestring,      // <<<
    DupInput,        // <&
    DupOutput,       // >&
    OutputAndError,  // &>
    AppendAndError,  // &>>
    Clobber,         // >|
};

struct Redirect {
    int fd{-1};  // -1 means default (0 for input, 1 for output)
    RedirectOp op{RedirectOp::Output};
    std::string target;  // filename, fd number as string, or heredoc body
    SourceLocation location;
};

struct Assignment {
    std::string name;
    std::string value;
    SourceLocation location;
};

struct SimpleCommand {
    std::vector<Assignment> assignments;
    std::vector<std::string> words;
    std::vector<Redirect> redirects;
    SourceLocation location;
};

using Command = std::variant<SimpleCommand, std::unique_ptr<Pipeline>, std::unique_ptr<IfCommand>,
                             std::unique_ptr<WhileCommand>, std::unique_ptr<ForCommand>>;

struct Pipeline {
    std::vector<SimpleCommand> commands;
    bool negated{false};  // ! pipeline
    SourceLocation location;
};

enum class AndOrOp {
    And,  // &&
    Or    // ||
};

struct AndOrItem {
    AndOrOp op;
    Pipeline pipeline;
};

struct AndOrList {
    Pipeline first;
    std::vector<AndOrItem> rest;
};

struct ListItem {
    AndOrList and_or;
    bool background{false};  // & vs ;
};

struct List {
    std::vector<ListItem> items;
};

struct IfCommand {
    List condition;
    List then_branch;
    std::vector<std::pair<List, List>> elif_branches;
    std::vector<ListItem> else_branch;
    SourceLocation location;
};

struct WhileCommand {
    List condition;
    List body;
    bool is_until{false};
    SourceLocation location;
};

struct ForCommand {
    std::string var_name;
    std::vector<std::string> words;
    List body;
    SourceLocation location;
};

struct Script {
    List commands;
};

}  // namespace myshell::ast
