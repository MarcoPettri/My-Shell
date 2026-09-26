// src/app/main.cc
#include "myshell/builtins/builtin.hh"
#include "myshell/completion/completion.hh"
#include "myshell/concurrency/thread_pool.hh"
#include "myshell/execution/executor.hh"
#include "myshell/history/history.hh"
#include "myshell/lexer/lexer.hh"
#include "myshell/parser/parser.hh"
#include "myshell/platform/posix/environment.hh"
#include "myshell/terminal/line_editor.hh"
#include "myshell/terminal/prompt.hh"

#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
    auto env = myshell::platform::Environment::from_system();
    myshell::builtins::BuiltinRegistry builtins;
    myshell::concurrency::ThreadPool pool(4);
    myshell::execution::Executor executor(env, builtins);

    // If a script file is passed
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "myshell: cannot open file: " << argv[1] << "\n";
            return 1;
        }
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        myshell::Lexer lexer(content, argv[1]);
        myshell::Parser parser(lexer);
        auto script_res = parser.parse();
        if (!script_res.ok()) {
            std::cerr << "Syntax error: " << script_res.error().message << "\n";
            return 2;
        }
        auto exec_res = executor.execute(script_res.value());
        return exec_res.ok() ? exec_res.value() : 1;
    }

    // Interactive mode
    myshell::terminal::LineEditor::Config editor_cfg{.program_name = "myshell",
                                                     .history_file = env.get("HOME").value_or(".") +
                                                                     "/.myshell_history",
                                                     .history_size = 1000,
                                                     .enable_completion = true};
    myshell::terminal::LineEditor editor(editor_cfg);
    myshell::terminal::PromptRenderer prompt(env, env);

    myshell::completion::CompletionEngine comp_engine(pool, env, builtins);
    comp_engine.add_provider(std::make_unique<myshell::completion::BuiltinProvider>(builtins));
    comp_engine.add_provider(std::make_unique<myshell::completion::VariableProvider>(env));
    comp_engine.add_provider(std::make_unique<myshell::completion::FilesystemProvider>(env));
    comp_engine.add_provider(std::make_unique<myshell::completion::CommandProvider>(env));

    editor.set_completion_callback(
        [&comp_engine](std::string_view prefix) { return comp_engine.complete(prefix, ""); });

    std::string ps1 = env.get("PS1").value_or("\\[\\e[1;32m\\]myshell\\[\\e[0m\\]:\\w\\$ ");

    while (true) {
        std::string prompt_str = prompt.render(ps1);
        auto line = editor.read_line(prompt_str);
        if (!line.has_value()) {
            break;  // EOF
        }

        std::string cmd = *line;
        if (cmd.empty())
            continue;

        editor.add_to_history(cmd);

        myshell::Lexer lexer(cmd, "<stdin>");
        myshell::Parser parser(lexer);
        auto script_res = parser.parse();
        if (!script_res.ok()) {
            std::cerr << "myshell: syntax error: " << script_res.error().message << "\n";
            continue;
        }

        auto exec_res = executor.execute(script_res.value());
        if (!exec_res.ok()) {
            std::cerr << "myshell: " << exec_res.error().message << "\n";
        }
    }

    return 0;
}
