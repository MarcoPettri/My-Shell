// include/myshell/builtins/builtin.hh
#pragma once

#include "myshell/platform/posix/environment.hh"
#include "myshell/support/error.hh"
#include "myshell/support/result.hh"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace myshell::builtins {

struct BuiltinContext {
    std::span<const std::string> args;
    platform::Environment& env;
    int stdin_fd{0};
    int stdout_fd{1};
    int stderr_fd{2};
    bool interactive{false};
};

class IBuiltin {
public:
    virtual ~IBuiltin() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view synopsis() const noexcept = 0;
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
    [[nodiscard]] virtual int execute(BuiltinContext& ctx) = 0;
};

class BuiltinRegistry {
public:
    BuiltinRegistry();

    void register_builtin(std::unique_ptr<IBuiltin> builtin);
    [[nodiscard]] IBuiltin* find(std::string_view name) const noexcept;
    [[nodiscard]] std::vector<std::string_view> names() const;

private:
    std::unordered_map<std::string, std::unique_ptr<IBuiltin>> builtins_;
};

}  // namespace myshell::builtins

namespace myshell::support {
using BuiltinRegistry = myshell::builtins::BuiltinRegistry;
}
