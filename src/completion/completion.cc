// src/completion/completion.cc
#include "myshell/completion/completion.hh"

#include "myshell/builtins/builtin.hh"
#include "myshell/concurrency/thread_pool.hh"
#include "myshell/platform/posix/environment.hh"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>

namespace myshell::completion {

CompletionEngine::CompletionEngine(concurrency::ThreadPool& pool, const support::Environment& env,
                                   const support::BuiltinRegistry& builtins)
    : pool_(pool)
    , env_(env)
    , builtins_(builtins) {
}

CompletionEngine::~CompletionEngine() = default;

void CompletionEngine::add_provider(std::unique_ptr<ICompletionProvider> provider) {
    if (provider) {
        providers_.push_back(std::move(provider));
    }
}

std::vector<std::string> CompletionEngine::complete(std::string_view prefix,
                                                    std::string_view context,
                                                    std::chrono::milliseconds timeout) {
    (void)timeout;

    std::vector<std::future<std::vector<std::string>>> futures;
    futures.reserve(providers_.size());

    for (const auto& provider : providers_) {
        futures.push_back(
            pool_.submit([&provider, p = std::string(prefix), c = std::string(context)]() {
                std::stop_source ss;
                return provider->get_candidates(p, c, ss.get_token());
            }));
    }

    std::vector<std::string> all_candidates;
    for (auto& f : futures) {
        try {
            auto res = f.get();
            all_candidates.insert(all_candidates.end(), res.begin(), res.end());
        } catch (...) {
        }
    }

    std::sort(all_candidates.begin(), all_candidates.end());
    all_candidates.erase(std::unique(all_candidates.begin(), all_candidates.end()),
                         all_candidates.end());
    return all_candidates;
}

// BuiltinProvider
BuiltinProvider::BuiltinProvider(const support::BuiltinRegistry& builtins)
    : builtins_(builtins) {
}

std::string_view BuiltinProvider::name() const noexcept {
    return "builtin";
}

std::vector<std::string> BuiltinProvider::get_candidates(std::string_view prefix, std::string_view,
                                                         std::stop_token stop) const {
    std::vector<std::string> res;
    for (auto b : builtins_.names()) {
        if (stop.stop_requested())
            break;
        if (b.starts_with(prefix)) {
            res.emplace_back(b);
        }
    }
    return res;
}

// VariableProvider
VariableProvider::VariableProvider(const support::Environment& env)
    : env_(env) {
}

std::string_view VariableProvider::name() const noexcept {
    return "variable";
}

std::vector<std::string> VariableProvider::get_candidates(std::string_view prefix, std::string_view,
                                                          std::stop_token stop) const {
    std::vector<std::string> res;
    if (!prefix.starts_with('$'))
        return res;

    std::string_view var_prefix = prefix.substr(1);
    for (const auto& [k, _] : env_.all_vars()) {
        if (stop.stop_requested())
            break;
        if (k.starts_with(var_prefix)) {
            res.push_back("$" + k);
        }
    }
    return res;
}

// FilesystemProvider
FilesystemProvider::FilesystemProvider(const support::Environment& env)
    : env_(env) {
}

std::string_view FilesystemProvider::name() const noexcept {
    return "filesystem";
}

std::vector<std::string> FilesystemProvider::get_candidates(std::string_view prefix,
                                                            std::string_view,
                                                            std::stop_token stop) const {
    std::vector<std::string> res;
    std::string dir_path = ".";
    std::string file_prefix = std::string(prefix);

    auto last_slash = prefix.rfind('/');
    if (last_slash != std::string_view::npos) {
        dir_path = std::string(prefix.substr(0, last_slash));
        if (dir_path.empty())
            dir_path = "/";
        file_prefix = std::string(prefix.substr(last_slash + 1));
    }

    DIR* d = ::opendir(dir_path.c_str());
    if (!d)
        return res;

    struct dirent* entry;
    while ((entry = ::readdir(d)) != nullptr) {
        if (stop.stop_requested())
            break;
        std::string_view name(entry->d_name);
        if (name == "." || name == "..")
            continue;
        if (name.starts_with(file_prefix)) {
            if (last_slash != std::string_view::npos) {
                if (dir_path == "/") {
                    res.push_back("/" + std::string(name));
                } else {
                    res.push_back(dir_path + "/" + std::string(name));
                }
            } else {
                res.emplace_back(name);
            }
        }
    }
    ::closedir(d);
    return res;
}

// CommandProvider
CommandProvider::CommandProvider(const support::Environment& env)
    : env_(env) {
}

std::string_view CommandProvider::name() const noexcept {
    return "command";
}

std::vector<std::string> CommandProvider::get_candidates(std::string_view prefix, std::string_view,
                                                         std::stop_token stop) const {
    std::vector<std::string> res;
    if (prefix.empty())
        return res;

    auto path_val = env_.get("PATH").value_or("/usr/bin:/bin");
    std::stringstream ss(path_val);
    std::string dir;

    while (std::getline(ss, dir, ':')) {
        if (stop.stop_requested())
            break;
        DIR* d = ::opendir(dir.c_str());
        if (!d)
            continue;

        struct dirent* entry;
        while ((entry = ::readdir(d)) != nullptr) {
            if (stop.stop_requested())
                break;
            std::string_view name(entry->d_name);
            if (name.starts_with(prefix)) {
                res.emplace_back(name);
            }
        }
        ::closedir(d);
    }
    return res;
}

}  // namespace myshell::completion
