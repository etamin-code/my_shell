#include <iostream>
#include <vector>
#include <string>
#include "parse_args.h"


 int parse_args (const std::string &s, std::vector<std::string> &args) {
    const std::string uncom_input = s.substr(0, s.find('#'));
    //std::vector<std::string> args;
    std::string arg;
    for (const auto &c : uncom_input) {
        if (isspace(c)) {
            if (!arg.empty()) {
                std::string var = substitute_var(arg);
                if (var.empty()) {
                    return EXIT_FAILURE;
                }
                args.emplace_back(var);
                arg = "";
            }
        } else {
            arg += c;
        }
    }

    std::string var = substitute_var(arg);
    if (!var.empty()) {
        args.emplace_back(var);
    }
    return 0;
}


bool ends_with(const std::string &value, const std::string &ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


std::string substitute_var(std::string &arg) {
    if (arg[0] == '$') {
        arg.erase(arg.begin());
        auto it = global_vars.find(arg);
        if(it != global_vars.end()) {
            return global_vars[arg];
        } else {
            return "";
        }
    }
    return arg;
}

