#include <iostream>
#include "unistd.h"
#include "write_buffer.h"
#include "hex_write_buffer.h"


int read_buffer(int &fd, bool is_A){
    int buf_size = 256000;
    char buffer[buf_size]; //Buffer of size 250KB

    ssize_t read_bytes = 0;
    while (read_bytes < buf_size) {
        ssize_t read_now = read(fd, buffer + read_bytes, buf_size - read_bytes);
        if (read_now == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                throw std::invalid_argument("Reading error. Error code: "
                                            + std::to_string(errno));
            }
        } else {
            if (!read_now) {
                break;
            }
            read_bytes += read_now;
            if (is_A) {
                hex_write_buffer(buffer, read_now);
            } else {
                write_buffer(buffer, read_now);
            }
        }
    }
    int res = close(fd);
    while (errno == EINTR) {
        close(fd);
    }
    if (res == -1) {
        throw std::invalid_argument("Closing error. Error code: "
                                    + std::to_string(errno));
    }
    return 0;
}



