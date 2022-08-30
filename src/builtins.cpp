#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <wait.h>
#include "options_parser.h"
#include "parse_args.h"
#include "builtins.h"
#include <regex>
#include <bits/stdc++.h>




int execute_command(std::vector<std::string> &parsed) {
    std::string program_name = parsed[0];
    parsed.erase(parsed.begin());

    auto it = builtin_programs.find(program_name);
    if (it != builtin_programs.end()) {
        builtin_programs[program_name](parsed);
    }
    else if (std::filesystem::exists(program_name) &&
                (parsed.empty()) &&
                ends_with(program_name, ".msh")) {

        pid_t pid = fork();

        if (pid == 0) {
            //child
            std::vector<std::string> data = {program_name};
            execute_script(data);
            exit(0);
        } else if (pid > 0){
            //parent
            int status;
            waitpid(pid, &status, 0);
        } else  {
            std::cerr << "Failed to fork()" << std::endl;
            error_value = EXIT_FAILURE;
            return EXIT_FAILURE;
        }

    } else if ((program_name == "myshell") &&
                std::filesystem::exists(parsed[0]) &&
                (parsed.size() == 1) &&
                ends_with(parsed[0], ".msh")) {
        pid_t pid = fork();

        if (pid == 0) {
            //child
            char *argv[2];
            argv[0] = parsed[0].data();
            argv[1] = NULL;
            execvp(argv[0], argv);
            return EXIT_FAILURE;

        } else if (pid > 0){
            //parent
            int status;
            waitpid(pid, &status, 0);
        } else  {
            std::cerr << "Failed to fork()" << std::endl;
            error_value = EXIT_FAILURE;
            return EXIT_FAILURE;
        }
    } else {
        bool failed = false;
        pid_t pid = fork();

        if (pid == 0) {
            //child
            char *argv[parsed.size() + 2];
            argv[0] = program_name.data();
            for (size_t i = 1; i < parsed.size() + 1; i++) {
                argv[i] = parsed[i - 1].data();
            }
            argv[parsed.size() + 1] = NULL;
            if (execvp(argv[0], argv)) {
                failed = true;
            }
        } else if (pid > 0){
            //parent
            int status;
            waitpid(pid, &status, 0);
        } else  {
            std::cerr << "Failed to fork()" << std::endl;
            error_value = EXIT_FAILURE;
            return EXIT_FAILURE;
        }
        if (failed) {
            std::cerr << "Unknown name: " << program_name << std::endl;
            error_value = EXIT_FAILURE;
            return EXIT_FAILURE;
        }

    }
    return 0;
}


bool compare_wildcard(std::string str, std::string pattern)
{
    int n = str.size(), m = pattern.size();
    if (m == 0)
        return (n == 0);
    bool lookup[n + 1][m + 1];
    memset(lookup, false, sizeof(lookup));
    lookup[0][0] = true;
    for (int j = 1; j <= m; j++)
        if (pattern[j - 1] == '*')
            lookup[0][j] = lookup[0][j - 1];
    int i, j;
    for (i = 1; i <= n; i++) {
        for (j = 1; j <= m; j++) {
            if (pattern[j - 1] == '*')
                lookup[i][j]
                        = lookup[i][j - 1] || lookup[i - 1][j];
            else if (pattern[j - 1] == '?'
                     || str[i - 1] == pattern[j - 1])
                lookup[i][j] = lookup[i - 1][j - 1];
            else if (pattern[j - 1] == '[') {
                if (pattern[j + 1] == '-') {
                    if (str[i - 1] > pattern[j] && str[i - 1] < pattern[j + 2]) {
                        for (int q = 0; q < 5; q++) {
                            lookup[i][j + q] = lookup[i - 1][j - 1];
                        }
                    } else {
                        for (int q = 0; q < 5; q++) {
                            lookup[i][j + q] = false;
                        }
                    }
                    j += 4;

                    if (pattern[j - 1] != ']') {
                        std::cerr << "wildcard pattern is incorrect" << std::endl;
                        error_value = EXIT_FAILURE;
                        return EXIT_FAILURE;
                    }
                }
                else {
                    std::vector<char> characters_to_find;
                    while (1) {
                        j++;
                        if (pattern[j-1] != ']')
                            characters_to_find.push_back(pattern[j-1]);
                        else
                            break;
                    }
                    bool match=false;
                    for (char el: characters_to_find) {
                        if (el == str[i-1]){
                            match = true;
                            break;
                        }
                    }
                    for (int q=characters_to_find.size() + 1; q>=0; q--){
                        lookup[i][j-q] = match;
                    }
                }
            }
            else
                lookup[i][j] = false;
        }
    }
    return lookup[n][m];
}

std::vector<std::string> get_wildcard_files(const std::string& search_path) {
    std::string working_dir = "./", cur_file;
    size_t found = search_path.find_last_of('/', search_path.size());
    std::string pattern = search_path;
    if (found != std::string::npos) {
        working_dir = search_path.substr(0, found);
        pattern = search_path.substr(found + 1, search_path.size() - found);
    }
    std::vector<std::string> match_files;
    const std::filesystem::directory_iterator end;
    std::error_code ec;

    for (std::filesystem::directory_iterator iter{working_dir}; iter != end; iter++) {
        cur_file = iter->path().string();

        try {
            if (std::filesystem::is_regular_file(cur_file, ec)) {
                found = cur_file.find_last_of('/', cur_file.size());
                if (compare_wildcard(cur_file.substr(found + 1, cur_file.size() - found), pattern)) {
                    match_files.push_back(cur_file);
                }
            }
            if (ec) {
                std::cerr << "error in file checking" << std::endl;
                error_value = EXIT_FAILURE;
                return match_files;
            }
        }
        catch (std::exception&) {}
    }
    return match_files;
    }


int check_and_execute(const std::string &input) {
    int result = 0, i = 0;
    history.push_back(input);
    std::vector<std::string> parsed, match_files;
    int wildcard=0;
    std::string special_ch="[]*?";

    if (parse_args(input, parsed)) {
        std::cerr << "Invalid arguments" << std::endl;
        error_value = -1;
        return 0;
    }
    if ( parsed.empty() ) {
        return result;
    } else {
        for (i=0; i<parsed.size(); i++) {
            for (int j = 0; j < special_ch.size(); j++)
                if ((parsed[i].find(special_ch[j]) != std::string::npos)) {
                    if (parsed[i].find('=') == std::string::npos) {
                        wildcard = 1;
                        break;
                    }
                }
            if (wildcard)
                break;
        }
        if (wildcard){
            match_files = get_wildcard_files(parsed[i]);
            parsed.erase(parsed.begin() + i);
            parsed.insert(parsed.begin() + i,match_files.begin(),match_files.end());
            result = execute_command(parsed);

            std::cout << std::endl;
            return result;
        }
        result = execute_command(parsed);
        std::cout << std::endl;
    }
    return result;
}

//------------------------------------------------------------------------------------------------


int merrno(std::vector<std::string> &args) {
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::cout << "merrno [-h|--help]" << std::endl;
            error_value = 0;
            return 0;
        }
    }
    if (!args.empty()) {
        error_value = -1;
        return error_value;
    }
    std::cout << error_value << std::endl;
    error_value = 0;
    return 0;
}


int mpwd(std::vector<std::string> &args) {
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::cout << "mpwd [-h|--help]" << std::endl;
            error_value = 0;
            return 0;
        }
    }
    if (!args.empty()) {
        error_value = -1;
        return error_value;
    }
    std::cout << std::filesystem::current_path() << std::endl;
    error_value = 0;
    return 0;
}


int mcd(std::vector<std::string> &args) {
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::cout << "mcd <path> [-h|--help] " << std::endl;
            error_value = 0;
            return 0;
        }
    }
    if (args.size() != 1) {
        std::cerr << "Invalid number of arguments" << std::endl;
        error_value = -1;
        return error_value;
    }

    int success;
    std::string cur_path = std::filesystem::current_path().string();

    std::string new_path;
    new_path = cur_path + "/" + args[0];
    success = chdir(new_path.c_str());
    if (success != 0) {
        std::cerr << "Unknown path" << std::endl;
        error_value = -1;
        return error_value;
    }
    error_value = 0;
    return 0;
}


int mexit(std::vector<std::string> &args) {
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::cout << "mexit [completion code] [-h|--help]" << std::endl;
            error_value = 0;
            return 0;
        }
    }
    if (args.empty()) {
        exit(0);
    } else if (args.size() != 1) {
        error_value = -1;
        return -1;
    } else {
        try {
            int code = atoi( args[0].c_str());
            exit(code);
        } catch (std::exception &E){
            error_value = -1;
            return -1;
        }
    }
}


int mecho(std::vector<std::string> &args) {
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::cout << "mecho [-h|--help] [text|$<var_name>] [text|$<var_name>]  [text|$<var_name>] ..." << std::endl;
            error_value = 0;
            return 0;
        }
    }
    std::cout << boost::algorithm::join(args, " ") << std::endl;
    error_value = 0;
    return 0;
}


int mexport(std::vector<std::string> &args) {
    const auto pos = args[0].find_last_of('=');
    std::string name = args[0].substr(0, pos);
    std::string value = args[0].substr(pos + 1, args[0].length() - pos - 1);
    global_vars[name] = value;
    error_value = 0;
    return 0;
}


int execute_script(std::vector<std::string> &args) {
    if (!((args.size() == 1) && (ends_with(args[0], ".msh")))) {
        std::cerr << "Invalid script" << std::endl;
        error_value = -1;
        return error_value;
    }
    int result = 0;
    std::ifstream file(args[0]);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            result = check_and_execute(line);
        }
        file.close();
    }
    error_value = result;
    return result;
}





