// include/myshell/platform/posix/environment.hh
#pragma once

#include <unistd.h>

#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace myshell::platform {

class Environment {
public:
    Environment() = default;

    // Explicit copy/move constructors because std::shared_mutex is not copyable/movable
    Environment(const Environment& other) {
        std::shared_lock lock(other.mtx_);
        vars_ = other.vars_;
    }

    Environment& operator=(const Environment& other) {
        if (this != &other) {
            std::scoped_lock lock(mtx_, other.mtx_);
            vars_ = other.vars_;
        }
        return *this;
    }

    Environment(Environment&& other) noexcept {
        std::unique_lock lock(other.mtx_);
        vars_ = std::move(other.vars_);
    }

    Environment& operator=(Environment&& other) noexcept {
        if (this != &other) {
            std::scoped_lock lock(mtx_, other.mtx_);
            vars_ = std::move(other.vars_);
        }
        return *this;
    }

    static Environment from_system() {
        Environment env;
        if (environ != nullptr) {
            for (char** p = environ; *p != nullptr; ++p) {
                std::string entry(*p);
                auto eq = entry.find('=');
                if (eq != std::string::npos) {
                    env.set(entry.substr(0, eq), entry.substr(eq + 1), true);
                }
            }
        }
        return env;
    }

    void set(std::string_view name, std::string_view value, bool exported = true) {
        std::unique_lock lock(mtx_);
        vars_[std::string(name)] = VarEntry{std::string(value), exported};
    }

    void unset(std::string_view name) {
        std::unique_lock lock(mtx_);
        vars_.erase(std::string(name));
    }

    [[nodiscard]] std::optional<std::string> get(std::string_view name) const {
        std::shared_lock lock(mtx_);
        auto it = vars_.find(std::string(name));
        if (it != vars_.end()) {
            return it->second.value;
        }
        return std::nullopt;
    }

    [[nodiscard]] bool is_exported(std::string_view name) const {
        std::shared_lock lock(mtx_);
        auto it = vars_.find(std::string(name));
        if (it != vars_.end()) {
            return it->second.exported;
        }
        return false;
    }

    [[nodiscard]] std::vector<std::string> to_envp_strings() const {
        std::shared_lock lock(mtx_);
        std::vector<std::string> res;
        res.reserve(vars_.size());
        for (const auto& [k, v] : vars_) {
            if (v.exported) {
                res.push_back(k + "=" + v.value);
            }
        }
        return res;
    }

    [[nodiscard]] std::unordered_map<std::string, std::string> all_vars() const {
        std::shared_lock lock(mtx_);
        std::unordered_map<std::string, std::string> res;
        for (const auto& [k, v] : vars_) {
            res[k] = v.value;
        }
        return res;
    }

private:
    struct VarEntry {
        std::string value;
        bool exported{true};
    };

    mutable std::shared_mutex mtx_;
    std::unordered_map<std::string, VarEntry> vars_;
};

}  // namespace myshell::platform

namespace myshell::support {
using Environment = myshell::platform::Environment;
using ShellVariables = myshell::platform::Environment;
}  // namespace myshell::support
