// include/myshell/platform/posix/file_descriptor.hh
#pragma once

#include "myshell/support/error.hh"
#include "myshell/support/result.hh"

#include <fcntl.h>
#include <unistd.h>

#include <utility>

namespace myshell::platform {

class FileDescriptor {
public:
    FileDescriptor() noexcept
        : fd_(-1) {
    }
    explicit FileDescriptor(int fd) noexcept
        : fd_(fd) {
    }

    ~FileDescriptor() {
        close();
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept
        : fd_(other.release()) {
    }
    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.release();
        }
        return *this;
    }

    [[nodiscard]] int get() const noexcept {
        return fd_;
    }
    [[nodiscard]] bool is_valid() const noexcept {
        return fd_ >= 0;
    }
    explicit operator bool() const noexcept {
        return is_valid();
    }

    int release() noexcept {
        int old = fd_;
        fd_ = -1;
        return old;
    }

    void reset(int fd = -1) noexcept {
        close();
        fd_ = fd;
    }

    void close() noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    [[nodiscard]] Result<void, ShellError> set_cloexec(bool enable = true) const noexcept {
        if (!is_valid())
            return Result<void, ShellError>::err(
                ShellError::make(ErrorKind::BadFileDescriptor, "Invalid FD"));
        int flags = ::fcntl(fd_, F_GETFD);
        if (flags < 0)
            return Result<void, ShellError>::err(
                ShellError::from_errno(ErrorKind::SystemError, "fcntl F_GETFD"));
        if (enable)
            flags |= FD_CLOEXEC;
        else
            flags &= ~FD_CLOEXEC;
        if (::fcntl(fd_, F_SETFD, flags) < 0) {
            return Result<void, ShellError>::err(
                ShellError::from_errno(ErrorKind::SystemError, "fcntl F_SETFD"));
        }
        return Result<void, ShellError>::ok();
    }

    [[nodiscard]] Result<FileDescriptor, ShellError> duplicate(int target_fd = -1) const noexcept {
        if (!is_valid())
            return Result<FileDescriptor, ShellError>::err(
                ShellError::make(ErrorKind::BadFileDescriptor, "Invalid FD"));
        int new_fd = -1;
        if (target_fd >= 0) {
            new_fd = ::dup2(fd_, target_fd);
        } else {
            new_fd = ::dup(fd_);
        }
        if (new_fd < 0) {
            return Result<FileDescriptor, ShellError>::err(
                ShellError::from_errno(ErrorKind::DupFailed, "dup/dup2 failed"));
        }
        return Result<FileDescriptor, ShellError>::ok(FileDescriptor(new_fd));
    }

private:
    int fd_{-1};
};

class Pipe {
public:
    Pipe() noexcept = default;

    static Result<Pipe, ShellError> create(bool cloexec = true) noexcept {
        int fds[2];
        int flags = cloexec ? O_CLOEXEC : 0;
        if (::pipe2(fds, flags) < 0) {
            return Result<Pipe, ShellError>::err(
                ShellError::from_errno(ErrorKind::PipeFailed, "pipe2 failed"));
        }
        return Result<Pipe, ShellError>::ok(Pipe(FileDescriptor(fds[0]), FileDescriptor(fds[1])));
    }

    Pipe(FileDescriptor read_end, FileDescriptor write_end) noexcept
        : read_(std::move(read_end))
        , write_(std::move(write_end)) {
    }

    FileDescriptor& read_end() noexcept {
        return read_;
    }
    FileDescriptor& write_end() noexcept {
        return write_;
    }

    void close_read() noexcept {
        read_.close();
    }
    void close_write() noexcept {
        write_.close();
    }

private:
    FileDescriptor read_;
    FileDescriptor write_;
};

}  // namespace myshell::platform
