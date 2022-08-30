#include <iostream>
#include <filesystem>
#include <map>
#include <cstdlib>
#include <fstream>
#include "builtins.h"

int error_value = 0;
std::map<std::string, std::string> global_vars{};
std::vector<std::string> history;

std::map<std::string, FnPtr> builtin_programs = {
         {"merrno", merrno},
         {"mpwd", mpwd},
         {"mcd", mcd},
         {"mexit", mexit},
         {"mecho", mecho},
         {"mexport", mexport},
         {".", execute_script},
 };

int init_shell() {
    std::stringstream ss;
    ss << getenv("PATH");
    std::string env = ss.str() + ":" + std::filesystem::current_path().string();
    setenv("PATH", env.c_str(), 1);
    //std::cout << getenv("PATH") << std::endl;

    std::cout << "\nMy shell started\n" << std::endl;
    while (true) {
        std::string input;
        std::cout << std::filesystem::current_path() << " $ ";
        getline (std::cin, input);
        std::cout << std::endl;
        int result = 0;
        result = check_and_execute(input);
        if (result != 0)
            return result;
    }
}



int main() {
	init_shell();
	return 0;
}
