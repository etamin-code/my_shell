#include <iostream>
//#include <fcntl.h>
#include <unistd.h>


int hex_write_buffer(const char* buffer, ssize_t &size) {
    char new_buffer[size*4]; //max 1MB
    ssize_t ind = 0;
    for (ssize_t i = 0; i < size; i++) {
        if (!isspace(buffer[i]) && !isprint(buffer[i])) {
        //if (isspace(buffer[i])) {
            sprintf(&new_buffer[ind], "\\x%02x", buffer[i]);
            ind = ind + 4;
        } else {
            new_buffer[ind] = buffer[i];
            ind++;
        }
    }

    ssize_t written_bytes = 0;
    while( written_bytes < ind ) {
        ssize_t written_now = write(1, new_buffer + written_bytes,
                                    ind - written_bytes );
        if( written_now == -1){
            if (errno == EINTR){
                continue;
            } else{
                throw std::invalid_argument("Writing error. Error code: "
                                            + std::to_string(errno));
            }
        } else {
            written_bytes += written_now;
        }
    }
    return 0;
}



