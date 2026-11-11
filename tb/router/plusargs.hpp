// plusargs.hpp -- +NAME=value command-line knobs ($value$plusargs equivalent).
#pragma once
#include <cstdlib>
#include <map>
#include <string>

class PlusArgs {
public:
    PlusArgs(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            std::string a = argv[i];
            if (a.size() < 2 || a[0] != '+' || a.rfind("+verilator+", 0) == 0) continue;
            auto eq = a.find('=');
            if (eq == std::string::npos) args_[a.substr(1)] = "1";
            else args_[a.substr(1, eq - 1)] = a.substr(eq + 1);
        }
    }
    bool has(const std::string& k) const { return args_.count(k) != 0; }
    std::string str(const std::string& k, const std::string& def = "") const {
        auto it = args_.find(k);
        return it == args_.end() ? def : it->second;
    }
    long long num(const std::string& k, long long def) const {
        auto it = args_.find(k);
        return it == args_.end() ? def : std::strtoll(it->second.c_str(), nullptr, 0);
    }
    double real(const std::string& k, double def) const {
        auto it = args_.find(k);
        return it == args_.end() ? def : std::strtod(it->second.c_str(), nullptr);
    }
    const std::map<std::string, std::string>& all() const { return args_; }

private:
    std::map<std::string, std::string> args_;
};
