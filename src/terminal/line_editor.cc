// src/terminal/line_editor.cc
#include "myshell/terminal/line_editor.hh"

#include <unistd.h>

#include <iostream>

namespace myshell::terminal {

LineEditor::LineEditor(Config cfg)
    : cfg_(std::move(cfg))
    , is_tty_(::isatty(STDIN_FILENO) && ::isatty(STDOUT_FILENO)) {
}

LineEditor::~LineEditor() {
    restore_terminal();
}

std::optional<std::string> LineEditor::read_line(std::string_view prompt) {
    if (!prompt.empty()) {
        std::cout << prompt << std::flush;
    }

    std::string line;
    if (!std::getline(std::cin, line)) {
        return std::nullopt;  // EOF
    }
    return line;
}

void LineEditor::add_to_history(std::string_view line) {
    (void)line;
}

void LineEditor::set_completion_callback(CompletionCallback cb) {
    completion_cb_ = std::move(cb);
}

support::Result<void, support::ShellError> LineEditor::save_history() {
    return support::Result<void, support::ShellError>::ok();
}

support::Result<void, support::ShellError> LineEditor::load_history() {
    return support::Result<void, support::ShellError>::ok();
}

void LineEditor::on_resize(int new_cols, int new_rows) {
    (void)new_cols;
    (void)new_rows;
}

void LineEditor::restore_terminal() {
}

}  // namespace myshell::terminal
