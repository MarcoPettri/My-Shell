// include/myshell/platform/posix/process.hh
#pragma once

#include "myshell/platform/posix/environment.hh"
#include "myshell/platform/posix/file_descriptor.hh"
#include "myshell/support/error.hh"
#include "myshell/support/result.hh"

#include <sys/types.h>
#include <sys/wait.h>

#include <optional>
#include <string>
#include <vector>

namespace myshell::platform {

using ProcessId = pid_t;
using ProcessGroupId = pid_t;

struct ExitStatus {
    int raw_status{0};

    [[nodiscard]] bool exited() const noexcept {
        return WIFEXITED(raw_status);
    }
    [[nodiscard]] int exit_code() const noexcept {
        return WEXITSTATUS(raw_status);
    }
    [[nodiscard]] bool signaled() const noexcept {
        return WIFSIGNALED(raw_status);
    }
    [[nodiscard]] int term_signal() const noexcept {
        return WTERMSIG(raw_status);
    }
    [[nodiscard]] bool stopped() const noexcept {
        return WIFSTOPPED(raw_status);
    }
    [[nodiscard]] int stop_signal() const noexcept {
        return WSTOPSIG(raw_status);
    }

    [[nodiscard]] int to_shell_status() const noexcept {
        if (exited())
            return exit_code();
        if (signaled())
            return 128 + term_signal();
        return 1;
    }
};

class ChildProcess {
public:
    ChildProcess() noexcept
        : pid_(-1) {
    }
    explicit ChildProcess(ProcessId pid) noexcept
        : pid_(pid) {
    }

    ~ChildProcess() = default;

    ChildProcess(const ChildProcess&) = delete;
    ChildProcess& operator=(const ChildProcess&) = delete;

    ChildProcess(ChildProcess&& other) noexcept
        : pid_(other.pid_) {
        other.pid_ = -1;
    }
    ChildProcess& operator=(ChildProcess&& other) noexcept {
        if (this != &other) {
            pid_ = other.pid_;
            other.pid_ = -1;
        }
        return *this;
    }

    [[nodiscard]] ProcessId pid() const noexcept {
        return pid_;
    }
    [[nodiscard]] bool is_valid() const noexcept {
        return pid_ > 0;
    }

    [[nodiscard]] Result<ExitStatus, ShellError> wait(int options = 0) {
        if (!is_valid()) {
            return Result<ExitStatus, ShellError>::err(
                ShellError::make(ErrorKind::SystemError, "No child process to wait for"));
        }
        int status = 0;
        pid_t res = ::waitpid(pid_, &status, options);
        if (res < 0) {
            return Result<ExitStatus, ShellError>::err(
                ShellError::from_errno(ErrorKind::WaitFailed, "waitpid"));
        }
        return Result<ExitStatus, ShellError>::ok(ExitStatus{status});
    }

private:
    ProcessId pid_{-1};
};

}  // namespace myshell::platform
