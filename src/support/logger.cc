// src/support/logger.cc

#include "myshell/support/logger.hh"

#include <cstdio>
#include <ctime>
#include <iostream>

namespace myshell::support {

Logger& Logger::instance() noexcept {
    static Logger logger;
    return logger;
}

void Logger::log(LogLevel level, std::string_view subsystem, std::string_view message,
                 const std::source_location& loc) {
    if (level > level_)
        return;

    const char* lvl_str = "INFO";
    switch (level) {
    case LogLevel::Quiet:
        return;
    case LogLevel::Error:
        lvl_str = "ERROR";
        break;
    case LogLevel::Warning:
        lvl_str = "WARN";
        break;
    case LogLevel::Info:
        lvl_str = "INFO";
        break;
    case LogLevel::Debug:
        lvl_str = "DEBUG";
        break;
    case LogLevel::Trace:
        lvl_str = "TRACE";
        break;
    }

    std::lock_guard<std::mutex> lock(mtx_);
    // Print to stderr safely
    std::fprintf(stderr, "[%s][%s] %.*s (%s:%u)\n", lvl_str, std::string(subsystem).c_str(),
                 static_cast<int>(message.size()), message.data(), loc.file_name(), loc.line());
}

}  // namespace myshell::support
