#include <iostream>
#include <filesystem>
#include <map>
#include <cstdlib>
#include <fstream>
#include "builtins.h"
#include <boost/program_options.hpp>
#include <server.h>

int error_value = 0;
std::map<std::string, std::string> global_vars{};
std::vector<std::string> history;
namespace po = boost::program_options;
const int DEFAULT_PORT = 1234;

std::map<std::string, FnPtr> builtin_programs = {
         {"merrno", merrno},
         {"mpwd", mpwd},
         {"mcd", mcd},
         {"mexit", mexit},
         {"mecho", mecho},
         {"mexport", mexport},
         {".", execute_script},
 };

//int init_shell(int argc, char* argv[]) {
int init_shell() {
    std::stringstream ss;
    ss << getenv("PATH");
    std::string env = ss.str() + ":" + std::filesystem::current_path().string();
    setenv("PATH", env.c_str(), 1);

//    if (argc >= 2) {
//        std::string cmd= ". " + std::string(argv[1]);
//        int result = check_and_execute(cmd);
//        return result;
//    }
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



int main(int argc, char** argv) {
    bool server;
    po::options_description hidden("Hidden options");
    hidden.add_options()
            ("server", po::bool_switch(&server), "Start server")
            ("port", po::value<uint16_t>(), "server port");

    po::options_description cmdline_options;
    cmdline_options.add(hidden);

    po::variables_map vm;
    store(po::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
    notify(vm);

    if (server) {
        std::cout << "Start server" << std::endl;
        uint16_t port = !vm.count("port") ? DEFAULT_PORT : vm["port"].as<uint16_t>();
        mysh_server(port);
    } else {
        init_shell();
    }

	return 0;
}
