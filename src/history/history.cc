// src/history/history.cc
#include "myshell/history/history.hh"

#include <fcntl.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <sstream>

namespace myshell::history {

History::History(std::string history_file, size_t max_size)
    : file_(std::move(history_file))
    , max_size_(max_size == 0 ? 1000 : max_size) {
}

History::History(History&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    entries_ = std::move(other.entries_);
    file_ = std::move(other.file_);
    max_size_ = other.max_size_;
}

History& History::operator=(History&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(mutex_, other.mutex_);
        entries_ = std::move(other.entries_);
        file_ = std::move(other.file_);
        max_size_ = other.max_size_;
    }
    return *this;
}

void History::add(std::string entry) {
    if (entry.empty())
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!entries_.empty() && entries_.back() == entry) {
        return;  // Deduplicate consecutive identical entries
    }

    entries_.push_back(std::move(entry));
    trim_to_limit_locked();
}

void History::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

void History::delete_entry(size_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index < entries_.size()) {
        entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

std::optional<std::string_view> History::at(size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index < entries_.size()) {
        return entries_[index];
    }
    return std::nullopt;
}

size_t History::size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

std::vector<size_t> History::search(std::string_view pattern) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<size_t> res;
    for (size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].find(pattern) != std::string::npos) {
            res.push_back(i);
        }
    }
    return res;
}

support::Result<void, support::ShellError> History::save() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.empty())
        return support::Result<void, support::ShellError>::ok();

    std::ofstream out(file_, std::ios::trunc);
    if (!out.is_open()) {
        return support::Result<void, support::ShellError>::err(support::ShellError::make(
            support::ErrorKind::IoError, "Failed to open history file for writing: " + file_));
    }

    for (const auto& line : entries_) {
        // Simple escape of newlines
        for (char c : line) {
            if (c == '\n')
                out << "\\n";
            else
                out << c;
        }
        out << '\n';
    }

    return support::Result<void, support::ShellError>::ok();
}

support::Result<void, support::ShellError> History::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.empty())
        return support::Result<void, support::ShellError>::ok();

    std::ifstream in(file_);
    if (!in.is_open()) {
        return support::Result<void,
                               support::ShellError>::ok();  // Missing file is fine on initial run
    }

    std::string raw;
    while (std::getline(in, raw)) {
        if (raw.empty())
            continue;
        std::string processed;
        for (size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] == '\\' && i + 1 < raw.size() && raw[i + 1] == 'n') {
                processed.push_back('\n');
                ++i;
            } else {
                processed.push_back(raw[i]);
            }
        }
        entries_.push_back(std::move(processed));
    }
    trim_to_limit_locked();

    return support::Result<void, support::ShellError>::ok();
}

void History::print(int fd, std::optional<size_t> n) const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = entries_.size();
    size_t start = 0;
    if (n.has_value() && *n < count) {
        start = count - *n;
    }

    for (size_t i = start; i < count; ++i) {
        std::ostringstream ss;
        ss << std::setw(5) << (i + 1) << "  " << entries_[i] << "\n";
        std::string s = ss.str();
        [[maybe_unused]] auto res = ::write(fd, s.data(), s.size());
    }
}

void History::trim_to_limit_locked() {
    while (entries_.size() > max_size_) {
        entries_.pop_front();
    }
}

}  // namespace myshell::history
