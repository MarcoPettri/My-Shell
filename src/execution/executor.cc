// src/execution/executor.cc
#include "myshell/execution/executor.hh"

#include "myshell/platform/posix/file_descriptor.hh"
#include "myshell/platform/posix/process.hh"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

namespace myshell::execution {

Result<int, ShellError> Executor::execute(const ast::Script& script) {
    return execute_list(script.commands);
}

Result<int, ShellError> Executor::execute_list(const ast::List& list) {
    int status = 0;
    for (const auto& item : list.items) {
        auto res = execute_and_or(item.and_or);
        if (!res.ok())
            return res;
        status = res.value();
        last_exit_status_ = status;
    }
    return Result<int, ShellError>::ok(status);
}

Result<int, ShellError> Executor::execute_and_or(const ast::AndOrList& and_or) {
    auto res = execute_pipeline(and_or.first);
    if (!res.ok())
        return res;
    int status = res.value();

    for (const auto& item : and_or.rest) {
        if (item.op == ast::AndOrOp::And) {
            if (status == 0) {
                auto next_res = execute_pipeline(item.pipeline);
                if (!next_res.ok())
                    return next_res;
                status = next_res.value();
            }
        } else if (item.op == ast::AndOrOp::Or) {
            if (status != 0) {
                auto next_res = execute_pipeline(item.pipeline);
                if (!next_res.ok())
                    return next_res;
                status = next_res.value();
            }
        }
    }
    return Result<int, ShellError>::ok(status);
}

Result<int, ShellError> Executor::execute_pipeline(const ast::Pipeline& pipeline) {
    if (pipeline.commands.empty()) {
        return Result<int, ShellError>::ok(0);
    }

    if (pipeline.commands.size() == 1) {
        auto res = execute_simple_command(pipeline.commands[0]);
        if (!res.ok())
            return res;
        int status = res.value();
        if (pipeline.negated)
            status = (status == 0) ? 1 : 0;
        return Result<int, ShellError>::ok(status);
    }

    // Multiple commands in pipeline
    size_t n = pipeline.commands.size();
    std::vector<int> pipe_fds(2 * (n - 1));

    for (size_t i = 0; i < n - 1; ++i) {
        if (::pipe(&pipe_fds[2 * i]) < 0) {
            return Result<int, ShellError>::err(
                ShellError::from_errno(ErrorKind::PipeFailed, "pipe"));
        }
    }

    std::vector<platform::ChildProcess> children;
    children.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        pid_t pid = ::fork();
        if (pid < 0) {
            return Result<int, ShellError>::err(
                ShellError::from_errno(ErrorKind::ForkFailed, "fork"));
        }

        if (pid == 0) {
            // Child process
            if (i > 0) {
                ::dup2(pipe_fds[2 * (i - 1)], STDIN_FILENO);
            }
            if (i < n - 1) {
                ::dup2(pipe_fds[2 * i + 1], STDOUT_FILENO);
            }

            for (size_t j = 0; j < 2 * (n - 1); ++j) {
                ::close(pipe_fds[j]);
            }

            auto res = execute_simple_command(pipeline.commands[i]);
            ::exit(res.ok() ? res.value() : 1);
        } else {
            children.emplace_back(pid);
        }
    }

    // Close all pipes in parent
    for (size_t j = 0; j < 2 * (n - 1); ++j) {
        ::close(pipe_fds[j]);
    }

    int last_status = 0;
    for (size_t i = 0; i < children.size(); ++i) {
        auto wait_res = children[i].wait();
        if (wait_res.ok() && i == children.size() - 1) {
            last_status = wait_res.value().to_shell_status();
        }
    }

    if (pipeline.negated)
        last_status = (last_status == 0) ? 1 : 0;
    return Result<int, ShellError>::ok(last_status);
}

Result<int, ShellError> Executor::execute_simple_command(const ast::SimpleCommand& cmd) {
    // Process assignments
    for (const auto& assign : cmd.assignments) {
        env_.set(assign.name, expand_word(assign.value));
    }

    if (cmd.words.empty()) {
        return Result<int, ShellError>::ok(0);
    }

    auto words = expand_words(cmd.words);
    if (words.empty())
        return Result<int, ShellError>::ok(0);

    const std::string& cmd_name = words[0];

    // Check builtins
    if (auto builtin = builtins_.find(cmd_name)) {
        // Save std fds if redirects exist
        int saved_in = -1, saved_out = -1, saved_err = -1;
        if (!cmd.redirects.empty()) {
            saved_in = ::dup(STDIN_FILENO);
            saved_out = ::dup(STDOUT_FILENO);
            saved_err = ::dup(STDERR_FILENO);
            for (const auto& redir : cmd.redirects) {
                auto r_res = apply_redirect(redir);
                if (!r_res.ok()) {
                    if (saved_in >= 0) {
                        ::dup2(saved_in, STDIN_FILENO);
                        ::close(saved_in);
                    }
                    if (saved_out >= 0) {
                        ::dup2(saved_out, STDOUT_FILENO);
                        ::close(saved_out);
                    }
                    if (saved_err >= 0) {
                        ::dup2(saved_err, STDERR_FILENO);
                        ::close(saved_err);
                    }
                    return Result<int, ShellError>::err(r_res.error());
                }
            }
        }

        builtins::BuiltinContext b_ctx{.args = words,
                                       .env = env_,
                                       .stdin_fd = STDIN_FILENO,
                                       .stdout_fd = STDOUT_FILENO,
                                       .stderr_fd = STDERR_FILENO,
                                       .interactive = false};
        int res = builtin->execute(b_ctx);

        if (!cmd.redirects.empty()) {
            if (saved_in >= 0) {
                ::dup2(saved_in, STDIN_FILENO);
                ::close(saved_in);
            }
            if (saved_out >= 0) {
                ::dup2(saved_out, STDOUT_FILENO);
                ::close(saved_out);
            }
            if (saved_err >= 0) {
                ::dup2(saved_err, STDERR_FILENO);
                ::close(saved_err);
            }
        }
        return Result<int, ShellError>::ok(res);
    }

    // External binary
    std::string path = find_executable(cmd_name);
    if (path.empty()) {
        std::cerr << cmd_name << ": command not found\n";
        return Result<int, ShellError>::ok(127);
    }

    pid_t pid = ::fork();
    if (pid < 0) {
        return Result<int, ShellError>::err(ShellError::from_errno(ErrorKind::ForkFailed, "fork"));
    }

    if (pid == 0) {
        for (const auto& redir : cmd.redirects) {
            auto r_res = apply_redirect(redir);
            if (!r_res.ok()) {
                std::cerr << "Redirect failed: " << r_res.error().message << "\n";
                ::exit(1);
            }
        }

        std::vector<char*> argv;
        argv.reserve(words.size() + 1);
        for (auto& w : words) {
            argv.push_back(w.data());
        }
        argv.push_back(nullptr);

        auto env_strs = env_.to_envp_strings();
        std::vector<char*> envp;
        envp.reserve(env_strs.size() + 1);
        for (auto& e : env_strs) {
            envp.push_back(e.data());
        }
        envp.push_back(nullptr);

        ::execve(path.c_str(), argv.data(), envp.data());
        ::perror("execve");
        ::exit(126);
    }

    platform::ChildProcess child(pid);
    auto wait_res = child.wait();
    if (!wait_res.ok())
        return Result<int, ShellError>::err(wait_res.error());

    return Result<int, ShellError>::ok(wait_res.value().to_shell_status());
}

std::vector<std::string> Executor::expand_words(const std::vector<std::string>& words) {
    std::vector<std::string> res;
    res.reserve(words.size());
    for (const auto& w : words) {
        res.push_back(expand_word(w));
    }
    return res;
}

std::string Executor::expand_word(std::string_view word) {
    std::string out;
    for (size_t i = 0; i < word.size(); ++i) {
        if (word[i] == '$') {
            if (i + 1 < word.size() && word[i + 1] == '?') {
                out += std::to_string(last_exit_status_);
                i++;
                continue;
            }
            if (i + 1 < word.size() && word[i + 1] == '$') {
                out += std::to_string(::getpid());
                i++;
                continue;
            }

            size_t start = i + 1;
            bool braced = false;
            if (start < word.size() && word[start] == '{') {
                braced = true;
                start++;
            }

            size_t end = start;
            while (end < word.size() &&
                   (std::isalnum(static_cast<unsigned char>(word[end])) || word[end] == '_')) {
                end++;
            }

            std::string var_name(word.substr(start, end - start));
            if (braced && end < word.size() && word[end] == '}') {
                end++;
            }

            if (auto val = env_.get(var_name)) {
                out += *val;
            }

            i = end - 1;
        } else if (word[i] == '~' && i == 0) {
            if (auto h = env_.get("HOME")) {
                out += *h;
            } else {
                out.push_back('~');
            }
        } else {
            out.push_back(word[i]);
        }
    }
    return out;
}

std::string Executor::find_executable(std::string_view cmd) {
    if (cmd.find('/') != std::string_view::npos) {
        if (::access(std::string(cmd).c_str(), X_OK) == 0) {
            return std::string(cmd);
        }
        return {};
    }

    auto path_var = env_.get("PATH").value_or("/usr/bin:/bin");
    std::stringstream ss(path_var);
    std::string item;
    while (std::getline(ss, item, ':')) {
        std::string full_path = item + "/" + std::string(cmd);
        if (::access(full_path.c_str(), X_OK) == 0) {
            return full_path;
        }
    }
    return {};
}

Result<void, ShellError> Executor::apply_redirect(const ast::Redirect& redir) {
    int target_fd = redir.fd;
    if (target_fd < 0) {
        target_fd = (redir.op == ast::RedirectOp::Input || redir.op == ast::RedirectOp::Heredoc)
                        ? STDIN_FILENO
                        : STDOUT_FILENO;
    }

    if (redir.op == ast::RedirectOp::Output) {
        int fd = ::open(redir.target.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
            return Result<void, ShellError>::err(
                ShellError::from_errno(ErrorKind::SystemError, "open output"));
        ::dup2(fd, target_fd);
        ::close(fd);
    } else if (redir.op == ast::RedirectOp::Append) {
        int fd = ::open(redir.target.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0)
            return Result<void, ShellError>::err(
                ShellError::from_errno(ErrorKind::SystemError, "open append"));
        ::dup2(fd, target_fd);
        ::close(fd);
    } else if (redir.op == ast::RedirectOp::Input) {
        int fd = ::open(redir.target.c_str(), O_RDONLY);
        if (fd < 0)
            return Result<void, ShellError>::err(
                ShellError::from_errno(ErrorKind::SystemError, "open input"));
        ::dup2(fd, target_fd);
        ::close(fd);
    } else if (redir.op == ast::RedirectOp::OutputAndError) {
        int fd = ::open(redir.target.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
            return Result<void, ShellError>::err(
                ShellError::from_errno(ErrorKind::SystemError, "open &>"));
        ::dup2(fd, STDOUT_FILENO);
        ::dup2(fd, STDERR_FILENO);
        ::close(fd);
    }

    return Result<void, ShellError>::ok();
}

}  // namespace myshell::execution
