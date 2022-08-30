#include <iostream>
#include <filesystem>
#include "recursive_ls.h"
#include "options_parser.h"



int main(int argc, char** argv) {
    command_line_options parser;
    parser.parse(argc, argv);
    std::vector<std::string> arg = parser.get_filenames();

    std::string path;
    if (arg.empty()) {
        path = ".";
    }
    else if (arg.size() == 1) {
        if (std::filesystem::exists(parser.get_filenames()[0])) {
            path = parser.get_filenames()[0];
        } else {
            std::cerr << "No such file or directory" << std::endl;
            return -1;
        }
    }
    else {
        std::cerr << "Invalid arguments number" << std::endl;
        return -1;
    }

    int res = recursive_ls(path.c_str());
    if (res) {
        return -1;
    }
	return 0;
}
