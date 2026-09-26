// src/terminal/prompt.cc
#include "myshell/terminal/prompt.hh"

#include "myshell/platform/posix/environment.hh"

#include <pwd.h>
#include <unistd.h>

#include <climits>

namespace myshell::terminal {

PromptRenderer::PromptRenderer(const support::Environment& env, const support::ShellVariables& vars)
    : env_(env)
    , vars_(vars) {
}

std::string PromptRenderer::render(std::string_view ps1_template) const {
    std::string out;
    for (size_t i = 0; i < ps1_template.size(); ++i) {
        if (ps1_template[i] == '\\' && i + 1 < ps1_template.size()) {
            out += expand_escape(ps1_template[++i]);
        } else {
            out.push_back(ps1_template[i]);
        }
    }
    return out;
}

std::string PromptRenderer::ps2() const {
    if (auto val = vars_.get("PS2")) {
        return render(*val);
    }
    return "> ";
}

std::string PromptRenderer::expand_escape(char c) const {
    switch (c) {
    case 'u': {
        if (auto u = env_.get("USER"))
            return *u;
        if (auto pw = ::getpwuid(::geteuid()))
            return pw->pw_name;
        return "user";
    }
    case 'h': {
        char buf[256];
        if (::gethostname(buf, sizeof(buf)) == 0) {
            std::string h(buf);
            auto dot = h.find('.');
            if (dot != std::string::npos)
                return h.substr(0, dot);
            return h;
        }
        return "localhost";
    }
    case 'H': {
        char buf[256];
        if (::gethostname(buf, sizeof(buf)) == 0)
            return std::string(buf);
        return "localhost";
    }
    case 'w': {
        char cwd[PATH_MAX];
        if (::getcwd(cwd, sizeof(cwd))) {
            std::string p(cwd);
            if (auto home = env_.get("HOME")) {
                if (p == *home)
                    return "~";
                if (p.starts_with(*home + "/")) {
                    return "~" + p.substr(home->size());
                }
            }
            return p;
        }
        return ".";
    }
    case 'W': {
        char cwd[PATH_MAX];
        if (::getcwd(cwd, sizeof(cwd))) {
            std::string p(cwd);
            if (auto home = env_.get("HOME"); home && p == *home)
                return "~";
            auto slash = p.rfind('/');
            if (slash != std::string::npos && slash + 1 < p.size()) {
                return p.substr(slash + 1);
            }
            return p;
        }
        return ".";
    }
    case '$': {
        return (::geteuid() == 0) ? "#" : "$";
    }
    case 'n':
        return "\n";
    case 'e':
        return "\033";
    case '[':
        return "\001";  // libedit literal start
    case ']':
        return "\002";  // libedit literal end
    case '\\':
        return "\\";
    default:
        return std::string("\\") + c;
    }
}

}  // namespace myshell::terminal
