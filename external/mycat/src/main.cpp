#include <iostream>
#include <fcntl.h>
#include "options_parser.h"
#include "read_buffer.h"


int main(int argc, char** argv) {
    command_line_options parser;
    parser.parse(argc, argv);
    for (const auto &file: parser.get_filenames()) {      //check for not existing file
        command_line_options::assert_file_exist(file);
    }


    for (const auto &file: parser.get_filenames()) {
        int fd = open(file.c_str(), O_RDONLY);
        while (errno == EINTR) {
            fd = open(file.c_str(), O_RDONLY);
        }
        if (fd == -1) {
            throw std::invalid_argument(file + " opening error. Error code: "
                                                 + std::to_string(errno));
        }
        read_buffer(fd, parser.get_A());
    }

	return 0;
}
