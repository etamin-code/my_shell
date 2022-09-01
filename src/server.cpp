#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <filesystem>
#include "builtins.h"

//int main(int argc, char *argv[]) {
int mysh_server(uint16_t SERVER_PORT = 1234) {
    // server address
    auto SERVER_ADDRESS = INADDR_LOOPBACK;
    std::cout << "Info: Start server on 127.0.0.1:" << SERVER_PORT << std::endl;

    // setup server address
    sockaddr_in server_address{};
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = htonl(SERVER_ADDRESS);

    // create a TCP socket to listen, creation returns -1 on failure
    int listen_sock;
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "could not create listen socket" << std::endl;
        return 1;
    }

    // bind server socket to server address
    if ((bind(listen_sock, (sockaddr *) &server_address, sizeof(server_address))) < 0) {
        std::cerr << "could not bind socket" << std::endl;
        return 1;
    }

    int wait_size = 32;  // maximum number of waiting clients, after which dropping begins
    if (listen(listen_sock, wait_size) < 0) {
        std::cerr << "could not open socket for listening" << std::endl;
        return 1;
    }

    // socket address used to store client address
    sockaddr_in client_address{};
    int client_address_len = 0;

    while (true) {
        // open a new socket to transmit data per connection
        int client_sock;
        if ((client_sock = accept(listen_sock,
                                  (sockaddr *) &client_address,
                                  (socklen_t *) &client_address_len)) < 0) {
            std::cerr << "could not open a socket to accept data" << std::endl;
            return 1;
        }

        std::cout << "client connected with ip address: "
                  << inet_ntoa(client_address.sin_addr) << ":"
                  << ntohs(client_address.sin_port) << std::endl;

        auto pid = fork();
        if (pid == -1) {
            std::cerr << "error while forking" << std::endl;
            return 1;
        } else if (pid != 0) {
            // pid 0 - we're at main server process, continue listening
            close(client_sock);
            continue;
        }

        // update io fds
        if (dup2(client_sock, STDIN_FILENO) == -1 || dup2(client_sock, STDOUT_FILENO) == -1 ||
            dup2(client_sock, STDERR_FILENO) == -1) {
            std::cerr << "error in dup2" << std::endl;
            return 1;
        }

        // execute commands indefinitely
        while (true) {
            std::string input;
            std::cout << std::filesystem::current_path() << " $ ";
            getline(std::cin, input);
            std::cout << std::endl;
            check_and_execute(input);
        }

        close(client_sock);
    }

    close(listen_sock);
    return 0;
}