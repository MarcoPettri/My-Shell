// include/myshell/execution/executor.hh
#pragma once

#include "myshell/builtins/builtin.hh"
#include "myshell/concurrency/thread_pool.hh"
#include "myshell/parser/ast.hh"
#include "myshell/platform/posix/environment.hh"
#include "myshell/support/error.hh"
#include "myshell/support/result.hh"

namespace myshell::execution {

class Executor {
public:
    Executor(platform::Environment& env, builtins::BuiltinRegistry& builtins)
        : env_(env)
        , builtins_(builtins) {
    }

    Result<int, ShellError> execute(const ast::Script& script);
    Result<int, ShellError> execute_list(const ast::List& list);
    Result<int, ShellError> execute_and_or(const ast::AndOrList& and_or);
    Result<int, ShellError> execute_pipeline(const ast::Pipeline& pipeline);
    Result<int, ShellError> execute_simple_command(const ast::SimpleCommand& cmd);

private:
    platform::Environment& env_;
    builtins::BuiltinRegistry& builtins_;
    int last_exit_status_{0};

    std::vector<std::string> expand_words(const std::vector<std::string>& words);
    std::string expand_word(std::string_view word);
    std::string find_executable(std::string_view cmd);
    Result<void, ShellError> apply_redirect(const ast::Redirect& redir);
};

}  // namespace myshell::execution
