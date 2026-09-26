// include/myshell/support/logger.hh
#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>

namespace myshell::support {

enum class LogLevel : uint8_t { Quiet = 0, Error, Warning, Info, Debug, Trace };

class Logger {
public:
    static Logger& instance() noexcept;

    void set_level(LogLevel level) noexcept {
        level_ = level;
    }
    [[nodiscard]] LogLevel level() const noexcept {
        return level_;
    }

    void log(LogLevel level, std::string_view subsystem, std::string_view message,
             const std::source_location& loc = std::source_location::current());

    template<typename... Args>
    void error(std::string_view sub, std::string_view msg,
               const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Error, sub, msg, loc);
    }

    template<typename... Args>
    void warn(std::string_view sub, std::string_view msg,
              const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Warning, sub, msg, loc);
    }

    template<typename... Args>
    void info(std::string_view sub, std::string_view msg,
              const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Info, sub, msg, loc);
    }

    template<typename... Args>
    void debug(std::string_view sub, std::string_view msg,
               const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Debug, sub, msg, loc);
    }

    template<typename... Args>
    void trace(std::string_view sub, std::string_view msg,
               const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Trace, sub, msg, loc);
    }

private:
    Logger() = default;
    LogLevel level_{LogLevel::Warning};
    std::mutex mtx_;
};

}  // namespace myshell::support
