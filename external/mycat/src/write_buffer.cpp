#include <iostream>
//#include <fcntl.h>
#include <unistd.h>



int write_buffer (const char* buffer , ssize_t &size){
    ssize_t written_bytes = 0;
    while( written_bytes < size ) {
        ssize_t written_now = write(1, buffer + written_bytes,
                                    size - written_bytes );
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

