// src/builtins/builtin.cc
#include "myshell/builtins/builtin.hh"

#include <unistd.h>

#include <climits>
#include <iostream>

namespace myshell::builtins {

class EchoBuiltin : public IBuiltin {
public:
    std::string_view name() const noexcept override {
        return "echo";
    }
    std::string_view synopsis() const noexcept override {
        return "echo [-n] [args...]";
    }
    std::string_view description() const noexcept override {
        return "Write arguments to standard output";
    }

    int execute(BuiltinContext& ctx) override {
        bool newline = true;
        size_t start = 1;
        if (ctx.args.size() > 1 && ctx.args[1] == "-n") {
            newline = false;
            start = 2;
        }

        for (size_t i = start; i < ctx.args.size(); ++i) {
            std::cout << ctx.args[i];
            if (i + 1 < ctx.args.size()) {
                std::cout << ' ';
            }
        }
        if (newline) {
            std::cout << '\n';
        }
        std::cout << std::flush;
        return 0;
    }
};

class PwdBuiltin : public IBuiltin {
public:
    std::string_view name() const noexcept override {
        return "pwd";
    }
    std::string_view synopsis() const noexcept override {
        return "pwd";
    }
    std::string_view description() const noexcept override {
        return "Print current working directory";
    }

    int execute(BuiltinContext&) override {
        char buf[PATH_MAX];
        if (::getcwd(buf, sizeof(buf))) {
            std::cout << buf << '\n';
            return 0;
        }
        std::cerr << "pwd: error retrieving current directory\n";
        return 1;
    }
};

class CdBuiltin : public IBuiltin {
public:
    std::string_view name() const noexcept override {
        return "cd";
    }
    std::string_view synopsis() const noexcept override {
        return "cd [dir]";
    }
    std::string_view description() const noexcept override {
        return "Change working directory";
    }

    int execute(BuiltinContext& ctx) override {
        std::string target;
        if (ctx.args.size() < 2) {
            if (auto home = ctx.env.get("HOME")) {
                target = *home;
            } else {
                std::cerr << "cd: HOME not set\n";
                return 1;
            }
        } else if (ctx.args[1] == "-") {
            if (auto oldpwd = ctx.env.get("OLDPWD")) {
                target = *oldpwd;
                std::cout << target << '\n';
            } else {
                std::cerr << "cd: OLDPWD not set\n";
                return 1;
            }
        } else {
            target = ctx.args[1];
        }

        char old_cwd[PATH_MAX];
        char* old_res = ::getcwd(old_cwd, sizeof(old_cwd));

        if (::chdir(target.c_str()) != 0) {
            ::perror("cd");
            return 1;
        }

        if (old_res) {
            ctx.env.set("OLDPWD", old_cwd);
        }

        char new_cwd[PATH_MAX];
        if (::getcwd(new_cwd, sizeof(new_cwd))) {
            ctx.env.set("PWD", new_cwd);
        }

        return 0;
    }
};

class ExitBuiltin : public IBuiltin {
public:
    std::string_view name() const noexcept override {
        return "exit";
    }
    std::string_view synopsis() const noexcept override {
        return "exit [n]";
    }
    std::string_view description() const noexcept override {
        return "Exit the shell";
    }

    int execute(BuiltinContext& ctx) override {
        int code = 0;
        if (ctx.args.size() > 1) {
            try {
                code = std::stoi(ctx.args[1]);
            } catch (...) {
                code = 1;
            }
        }
        ::exit(code);
    }
};

class ExportBuiltin : public IBuiltin {
public:
    std::string_view name() const noexcept override {
        return "export";
    }
    std::string_view synopsis() const noexcept override {
        return "export [name[=value]...]";
    }
    std::string_view description() const noexcept override {
        return "Set export attribute for shell variables";
    }

    int execute(BuiltinContext& ctx) override {
        if (ctx.args.size() == 1) {
            for (const auto& [k, v] : ctx.env.all_vars()) {
                if (ctx.env.is_exported(k)) {
                    std::cout << "export " << k << "=\"" << v << "\"\n";
                }
            }
            return 0;
        }

        for (size_t i = 1; i < ctx.args.size(); ++i) {
            auto eq = ctx.args[i].find('=');
            if (eq != std::string::npos) {
                std::string k = ctx.args[i].substr(0, eq);
                std::string v = ctx.args[i].substr(eq + 1);
                ctx.env.set(k, v, true);
            } else {
                if (auto cur = ctx.env.get(ctx.args[i])) {
                    ctx.env.set(ctx.args[i], *cur, true);
                } else {
                    ctx.env.set(ctx.args[i], "", true);
                }
            }
        }
        return 0;
    }
};

BuiltinRegistry::BuiltinRegistry() {
    register_builtin(std::make_unique<EchoBuiltin>());
    register_builtin(std::make_unique<PwdBuiltin>());
    register_builtin(std::make_unique<CdBuiltin>());
    register_builtin(std::make_unique<ExitBuiltin>());
    register_builtin(std::make_unique<ExportBuiltin>());
}

void BuiltinRegistry::register_builtin(std::unique_ptr<IBuiltin> builtin) {
    if (builtin) {
        builtins_[std::string(builtin->name())] = std::move(builtin);
    }
}

IBuiltin* BuiltinRegistry::find(std::string_view name) const noexcept {
    auto it = builtins_.find(std::string(name));
    if (it != builtins_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string_view> BuiltinRegistry::names() const {
    std::vector<std::string_view> res;
    res.reserve(builtins_.size());
    for (const auto& [k, _] : builtins_) {
        res.push_back(k);
    }
    return res;
}

}  // namespace myshell::builtins
