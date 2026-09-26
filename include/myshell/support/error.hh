#pragma once

#include <cerrno>
#include <cstring>
#include <string>
#include <string_view>

namespace myshell {

/// @brief Error category for shell errors.
enum class ErrorKind {
    // System / IO errors
    SystemError,        ///< errno-carrying POSIX error
    NotFound,           ///< file or command not found
    PermissionDenied,   ///< EACCES / not executable
    IoError,            ///< general I/O failure
    BadFileDescriptor,  ///< invalid fd

    // Shell semantic errors
    CommandNotFound,  ///< command not on PATH
    NotExecutable,    ///< file exists but not executable
    InvalidRedirect,  ///< malformed redirect spec
    TooManyArgs,      ///< argument count exceeded
    InvalidArgument,  ///< bad option/argument to builtin
    NotADirectory,    ///< path component not a dir
    IsADirectory,     ///< expected file, got directory
    NoSuchJob,        ///< job id/spec not in job table
    ForkFailed,       ///< fork() returned -1
    ExecFailed,       ///< execve() returned -1
    PipeFailed,       ///< pipe() failed
    WaitFailed,       ///< waitpid() failed
    DupFailed,        ///< dup2() failed
    FdLimit,          ///< EMFILE / ENFILE
    Timeout,          ///< operation timed out

    // Scripting errors
    ParseError,  ///< expression parse failure
    DivisionByZero,
    UndefinedVariable,

    // Control-flow signals (not true errors)
    ExitRequested,      ///< exit builtin invoked
    ReturnRequested,    ///< return builtin invoked
    BreakRequested,     ///< break builtin invoked
    ContinueRequested,  ///< continue builtin invoked

    // Catch-all
    Unknown,
};

/// @brief Describes an error in the shell.
///
/// Carries a human-readable message, an optional errno value, and a category.
/// Use ShellError::from_errno() to capture the current errno immediately after
/// a failing syscall.
struct ShellError {
    ErrorKind kind{ErrorKind::Unknown};
    std::string message;
    int sys_errno{0};  ///< captured errno (0 if not applicable)

    // Payload for control-flow "errors"
    int exit_code{0};   ///< used by ExitRequested / ReturnRequested
    int loop_depth{1};  ///< used by BreakRequested / ContinueRequested

    // ------------------------------------------------------------------ //
    //  Factory helpers
    // ------------------------------------------------------------------ //

    /// @brief Create a ShellError from the current errno.
    /// @param kind     Error category (typically SystemError)
    /// @param context  Human-readable context ("open '/etc/passwd'")
    /// @return ShellError with message = context + ": " + strerror(errno)
    static ShellError from_errno(ErrorKind kind, std::string_view context) {
        int saved = errno;
        ShellError e;
        e.kind = kind;
        e.sys_errno = saved;
        e.message = std::string(context) + ": " + ::strerror(saved);
        return e;
    }

    /// @brief Create a simple error with a message.
    static ShellError make(ErrorKind kind, std::string msg) {
        ShellError e;
        e.kind = kind;
        e.message = std::move(msg);
        return e;
    }

    /// @brief Create an exit-requested control-flow error.
    static ShellError exit_requested(int code) {
        ShellError e;
        e.kind = ErrorKind::ExitRequested;
        e.exit_code = code;
        return e;
    }

    /// @brief Create a return-requested control-flow error.
    static ShellError return_requested(int code) {
        ShellError e;
        e.kind = ErrorKind::ReturnRequested;
        e.exit_code = code;
        return e;
    }

    /// @brief Create a break control-flow error.
    static ShellError break_requested(int depth = 1) {
        ShellError e;
        e.kind = ErrorKind::BreakRequested;
        e.loop_depth = depth;
        return e;
    }

    /// @brief Create a continue control-flow error.
    static ShellError continue_requested(int depth = 1) {
        ShellError e;
        e.kind = ErrorKind::ContinueRequested;
        e.loop_depth = depth;
        return e;
    }

    [[nodiscard]] std::string_view kind_name() const noexcept {
        switch (kind) {
        case ErrorKind::SystemError:
            return "SystemError";
        case ErrorKind::NotFound:
            return "NotFound";
        case ErrorKind::PermissionDenied:
            return "PermissionDenied";
        case ErrorKind::IoError:
            return "IoError";
        case ErrorKind::BadFileDescriptor:
            return "BadFileDescriptor";
        case ErrorKind::CommandNotFound:
            return "CommandNotFound";
        case ErrorKind::NotExecutable:
            return "NotExecutable";
        case ErrorKind::InvalidRedirect:
            return "InvalidRedirect";
        case ErrorKind::TooManyArgs:
            return "TooManyArgs";
        case ErrorKind::InvalidArgument:
            return "InvalidArgument";
        case ErrorKind::NotADirectory:
            return "NotADirectory";
        case ErrorKind::IsADirectory:
            return "IsADirectory";
        case ErrorKind::NoSuchJob:
            return "NoSuchJob";
        case ErrorKind::ForkFailed:
            return "ForkFailed";
        case ErrorKind::ExecFailed:
            return "ExecFailed";
        case ErrorKind::PipeFailed:
            return "PipeFailed";
        case ErrorKind::WaitFailed:
            return "WaitFailed";
        case ErrorKind::DupFailed:
            return "DupFailed";
        case ErrorKind::FdLimit:
            return "FdLimit";
        case ErrorKind::Timeout:
            return "Timeout";
        case ErrorKind::ParseError:
            return "ParseError";
        case ErrorKind::DivisionByZero:
            return "DivisionByZero";
        case ErrorKind::UndefinedVariable:
            return "UndefinedVariable";
        case ErrorKind::ExitRequested:
            return "ExitRequested";
        case ErrorKind::ReturnRequested:
            return "ReturnRequested";
        case ErrorKind::BreakRequested:
            return "BreakRequested";
        case ErrorKind::ContinueRequested:
            return "ContinueRequested";
        default:
            return "Unknown";
        }
    }
};

namespace support {
using ErrorKind = myshell::ErrorKind;
using ShellError = myshell::ShellError;
}  // namespace support

}  // namespace myshell
