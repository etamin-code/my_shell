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
#include <regex>
#include <bits/stdc++.h>
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include <wait.h>
#include "options_parser.h"
#include "parse_args.h"
#include "builtins.h"

std::map<std::string, int> FD{{"in",0}, {"out",1}, {"err",2}};


int execute_command(std::vector<std::string> &parsed) {
    std::string program_name = parsed[0];
    parsed.erase(parsed.begin());
    int background_mode = 0;
    if (parsed[parsed.size() - 1] == "&") {
        background_mode = 1;
        parsed.pop_back();
    }
    auto it = builtin_programs.find(program_name);

    if (it != builtin_programs.end()) {
        check_out_redirect(parsed);
        builtin_programs[program_name](parsed);
        if (FD["out"]!=1) {
            close(FD["out"]);
            FD["out"] = 1;
        }
        if (FD["err"]!=2) {
            close(FD["err"]);
            FD["err"] = 2;
        }
    }
    else if (std::filesystem::exists(program_name) &&
                (parsed.empty()) &&
                ends_with(program_name, ".msh")) {
        std::string cmd = "myshell " + program_name;
        if (check_and_execute(cmd))
            return EXIT_FAILURE;
    }
    else {
        bool failed = false;
        pid_t pid = fork();

        if (pid == 0) {
            //child
            check_out_redirect(parsed);
            if (FD["out"]!=1) {
                dup2(FD["out"], 1);
            }
            if (FD["err"]!=2) {
                dup2(FD["err"], 2);
            }

            char *argv[parsed.size() + 2];
            argv[0] = program_name.data();
            for (size_t i = 1; i < parsed.size() + 1; i++) {
                argv[i] = parsed[i - 1].data();
            }
            argv[parsed.size() + 1] = NULL;
            if (execvp(argv[0], argv)) {
                failed = true;
            }
            if (FD["out"]!=1) {
                close(FD["out"]);
                FD["out"] = 1;
            }
            if (FD["err"]!=2) {
                close(FD["err"]);
                FD["err"] = 2;
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

int check_pipe(std::string &input, std::vector<std::string> &commands) {
    std::string delimiter = "|";
    size_t pos = 0;
    std::string token;
    while ((pos = input.find(delimiter)) != std::string::npos) {
        token = input.substr(0, pos);
        commands.push_back(token);
        input.erase(0, pos + delimiter.length());
    }
    commands.push_back(input);
    if (commands.size() >= 2) {
        for (int i=0; i<commands.size(); i++) {
            if (i != 0)
                commands[i] = commands[i] + " < in.txt";
            if (i != commands.size() - 1)
                commands[i] = commands[i] + " > out.txt";
        }
    }
    return 0;
}

int check_inside_commands(std::string &input) {
    size_t pos1, pos2;
    while ((pos1 = input.find("$(")) != std::string::npos) {
        std::ifstream read_file;
        pos2 = input.find(")");
        std::string cmd = input.substr(pos1+2, pos2 - pos1 - 2  );
        cmd = cmd + " > out2.txt";
        check_and_execute(cmd);
        read_file.open("out2.txt");
        std::string line, file_data="";
        if (read_file.is_open()) {
            std::getline(read_file, line);
            file_data = file_data + line;
        }
        input = input.substr(0, pos1) + file_data + input.substr(pos2 + 1, input.size() - pos2 - 1);
    }
    remove("out2.txt");
    return 0;
}

int check_in_redirect(std::string &input) {
//   if it finds sign '<' read file content and add it to input
    std::vector <std::string> parsed;
    if (parse_args(input, parsed)) {
        std::cout << "errorr" << std::endl;
        std::string cur_string = "Invalid arguments";
        error_value = write_to_fd(FD["err"], cur_string);
        if (error_value != 0) {
            return error_value;
        }
        error_value = EXIT_FAILURE;
        return error_value;
    }
    std::ifstream read_file;
    for (int i = 0; i < parsed.size(); i++) {
        if (parsed[i].find_last_of('<', parsed[i].size()) != std::string::npos) {
            if (i == parsed.size() - 1) {
                std::string cur_string = "incorrect syntax, the only appearence of < cant be in the last argument";
                error_value = write_to_fd(FD["err"], cur_string);
                if (error_value != 0) {
                    return error_value;
                }
            }
            if (std::filesystem::exists(parsed[i + 1])) {
                read_file.open(parsed[i + 1].c_str());
            } else {
                std::string cur_string = "file for redirection doesn`t exist\n";
                error_value = write_to_fd(FD["err"], cur_string);
                if (error_value != 0) {
                    return error_value;
                }
                error_value = EXIT_FAILURE;
                return error_value;
            }

            std::string line, file_data="";
            if (read_file.is_open()) {
                std::getline(read_file, line);
                file_data = file_data + line;
            }
            else {
                std::string cur_string = "error in opening file";
                error_value = write_to_fd(FD["err"], cur_string);
                if (error_value != 0) {
                    return error_value;
                }
                error_value = EXIT_FAILURE;
                return error_value;
            }

        //  delete elements which relate to redirection
            parsed.erase(parsed.begin() + i);
            parsed.erase(parsed.begin() + i);
            std::vector <std::string> parsed_from_file;
            if (parse_args(file_data, parsed_from_file)) {
                std::string cur_string = "Invalid arguments in file";
                error_value = write_to_fd(FD["err"], cur_string);
                if (error_value != 0) {
                    return error_value;
                }
                error_value = EXIT_FAILURE;
                return error_value;
            }
            parsed.insert(parsed.begin() + i,parsed_from_file.begin(),parsed_from_file.end());
            input = boost::algorithm::join(parsed, " ");

        }
    }
    error_value = EXIT_SUCCESS;
    return error_value;

}


int check_out_redirect(std::vector<std::string> &parsed) {
    int fd;
    for (int i=0; i<parsed.size(); i++){
        if (parsed[i].find_last_of('>', parsed[i].size()) != std::string::npos) {
            if (i == parsed.size() - 1) {
                std::string cur_string = "incorrect syntax, the only appearence of > cant be in the last argument";
                error_value = write_to_fd(FD["err"], cur_string);
                if (error_value != 0) {
                    return error_value;
                }
                error_value = EXIT_FAILURE;
                return error_value;
            }
            fd = open(parsed[i+1].c_str(), O_WRONLY | O_APPEND | O_CREAT, 0600);
            if (fd == -1) {
                std::string cur_string = "error in opening redirection file\n";
                error_value = write_to_fd(FD["err"], cur_string);
                if (error_value != 0) {
                    return error_value;
                }
                error_value = EXIT_FAILURE;
                return error_value;
            }
            if (parsed[i].find_last_of('2', parsed[i].size()) != std::string::npos) {
                FD["err"] = fd;
            }else if (parsed[i].find_last_of('&', parsed[i].size()) != std::string::npos) {
                FD["err"] = fd;
                FD["out"] = fd;
            } else if ((parsed[i].find_last_of('1', parsed[i].size()) != std::string::npos)) {
                FD["out"] = fd;
            }
            else if (parsed[i].size() == 1) {
                if (i + 2 < parsed.size()) {
                    if (parsed[i + 2] == "2>&1" || parsed[i + 2] == "1>&2") {
                        FD["err"] = fd;
                        FD["out"] = fd;
                    }
                }
                else {
                    FD["out"] = fd;
                }

            }
            else {
                std::string cur_string = "incorrect syntax " + parsed[i] + " is not acceptable argument";
                error_value = write_to_fd(FD["err"], cur_string);
                if (error_value != 0) {
                    return error_value;
                }
                error_value = EXIT_FAILURE;
                return error_value;
            }
//            delete elements which relate to redirection
            parsed.erase(parsed.begin() + i);
            parsed.erase(parsed.begin() + i);
            if (parsed[i].find_last_of('>', parsed[i].size()) != std::string::npos) {
                parsed.erase(parsed.begin() + i);
            }
            break;
        }
    }
    error_value = EXIT_SUCCESS;
    return error_value;
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
                        std::string cur_string = "wildcard pattern is incorrect";
                        error_value = write_to_fd(FD["err"], cur_string);
                        if (error_value != 0) {
                            return error_value;
                        }
                        error_value = EXIT_FAILURE;
                        return error_value;
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

int get_wildcard_files(const std::string& search_path, std::vector<std::string> &match_files) {
    std::string working_dir = "./", cur_file;
    size_t found = search_path.find_last_of('/', search_path.size());
    std::string pattern = search_path;
    if (found != std::string::npos) {
        working_dir = search_path.substr(0, found);
        pattern = search_path.substr(found + 1, search_path.size() - found);
    }
    const std::filesystem::directory_iterator end;
    std::error_code ec;

    for (std::filesystem::directory_iterator iter{working_dir}; iter != end; iter++) {
        cur_file = iter->path().string();

        try {
//            if (std::filesystem::is_regular_file(cur_file, ec)) {
                found = cur_file.find_last_of('/', cur_file.size());
                if (compare_wildcard(cur_file.substr(found + 1, cur_file.size() - found), pattern)) {
                    match_files.push_back(cur_file);
                }
//            }
            if (ec) {
                std::string cur_string = "error in file checking";
                error_value = write_to_fd(FD["err"], cur_string);
                if (error_value != 0) {
                    return error_value;
                }
                error_value = EXIT_FAILURE;
                return error_value;
            }
        }
        catch (std::exception&) {}
    }
    error_value = EXIT_SUCCESS;
    return error_value;
    }


int check_and_execute(std::string &input) {
    int result = 0, i = 0;
    history.push_back(input);
    std::vector<std::string> commands;
    check_pipe(input, commands);

    for (std::string cmd: commands) {
        check_in_redirect(cmd);
        check_inside_commands(cmd);
        std::vector <std::string> parsed;
        int wildcard = 0;
        std::string special_ch = "[]*?";

        if (parse_args(cmd, parsed)) {
            std::string cur_string = "Invalid arguments";
            error_value = write_to_fd(FD["err"], cur_string);
            if (error_value != 0) {
                return error_value;
            }
            error_value = EXIT_FAILURE;
            return error_value;
        }
        if (parsed.empty()) {
            return result;
        } else {
            for (i = 0; i < parsed.size(); i++) {
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
            if (wildcard) {
                std::vector <std::string> match_files;
                get_wildcard_files(parsed[i], match_files);
                parsed.erase(parsed.begin() + i);
                parsed.insert(parsed.begin() + i, match_files.begin(), match_files.end());
                result = execute_command(parsed);
                return result;
            }
            result = execute_command(parsed);
            std::cout << std::endl;
        }
        rename("out.txt", "in.txt");
        if (result)
            return result;
    }
    remove("in.txt");
    return result;
}


//------------------------------------------------------------------------------------------------

int write_to_fd(int fd, std::string &cur_string) {
    char cur_char_array[cur_string.size() + 1];
    strcpy(cur_char_array, cur_string.c_str());
    if (write(fd, &cur_char_array, cur_string.size()) == -1)
        error_value = EXIT_FAILURE;
    else
        error_value = EXIT_SUCCESS;
    return error_value;
}

int merrno(std::vector<std::string> &args) {
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::string cur_string = "merrno [-h|--help]";
            error_value = write_to_fd(FD["out"], cur_string);
            return error_value;
        }
    }
    if (!args.empty()) {
        error_value = -1;
        return error_value;
    }
    std::string cur_string = std::to_string(error_value);
    error_value = write_to_fd(FD["out"], cur_string);
    return error_value;
}


int mpwd(std::vector<std::string> &args) {
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::string cur_string = "mpwd [-h|--help]";
            error_value = write_to_fd(FD["out"], cur_string);
            return error_value;
        }
    }
    if (!args.empty()) {
        error_value = -1;
        return error_value;
    }
    std::string cur_string = std::filesystem::current_path();
    error_value = write_to_fd(FD["out"], cur_string);
    return error_value;
}


int mcd(std::vector<std::string> &args) {
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::string cur_string = "mcd <path> [-h|--help] ";
            error_value = write_to_fd(FD["out"], cur_string);
            return error_value;
        }
    }
    if (args.size() != 1) {
        std::string cur_string = "Invalid number of arguments";
        error_value = write_to_fd(FD["err"], cur_string);
        return error_value;
    }

    int success;
    std::string cur_path = std::filesystem::current_path().string();

    std::string new_path;
    new_path = cur_path + "/" + args[0];
    success = chdir(new_path.c_str());
    if (success != 0) {
        std::string cur_string = "Unknown path";
        error_value = write_to_fd(FD["err"], cur_string);
        if (error_value != 0) {
            return error_value;
        }
        error_value = -1;
        return error_value;
    }
    error_value = 0;
    return 0;
}


int mexit(std::vector<std::string> &args) {
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::string cur_string = "mexit [completion code] [-h|--help]";
            error_value = write_to_fd(FD["out"], cur_string);
            return error_value;
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
//    int fd_out=1, fd_err=1;
    for (const auto & arg : args) {
        if ((arg == "-h") || (arg == "--help")) {
            std::string cur_string = "mecho [-h|--help] [text|$<var_name>] [text|$<var_name>]  [text|$<var_name>] ...";
            error_value = write_to_fd(FD["out"], cur_string);
            return error_value;
        }
    }
    std::string cur_string = boost::algorithm::join(args, " ");
    error_value = write_to_fd(FD["out"], cur_string);
    return error_value;
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
        std::string cur_string = "Invalid script";
        error_value = write_to_fd(FD["err"], cur_string);
        if (error_value != 0) {
            return error_value;
        }
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





