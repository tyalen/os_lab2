#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_CLIENTS 1
#define PORT 5555

int server_socket = -1;
int client_socket = -1;

volatile sig_atomic_t wasSighup = 0;

void sighupHandler(int signal) {
    if (signal == SIGHUP) 
    {
        wasSighup = 1;
    }
}

void close_sockets() {
    close(server_socket);
    if (client_socket != -1) 
    {
        close(client_socket);
        client_socket = -1;
    }
}

int main() {
    struct sockaddr_in serverAddr;
    int new_socket;

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) 
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Signal handling
    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sighupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    // Set up server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) 
    {
        perror("Error binding");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) 
    {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

	// Blocking the signal
    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        int max_fd = server_socket;

        if (client_socket != -1) 
        {
            FD_SET(client_socket, &read_fds);
            max_fd = (client_socket > max_fd) ? client_socket : max_fd;
        }
        
        int pselectRes = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &origMask);

        if (pselectRes == -1) 
        {
            if (errno == EINTR) {
                if (wasSighup) {
                    printf("Received SIGHUP");
                    wasSighup = 0;
                } else {
                    perror("EINTR, but not sighup");
                    close_sockets();
                    exit(EXIT_FAILURE);
                }
            } else {
                perror("PSelect unknown error.");
                close_sockets();
                exit(EXIT_FAILURE);
            }
        }

        if (pselectRes == 0) {
            continue;
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            if ((new_socket = accept(server_socket, NULL, NULL)) == -1) {
                perror("Accept error");
                close_sockets();
                exit(EXIT_FAILURE);
            }

            if (client_socket != -1) {
                close(new_socket);
            } else {
                printf("New connection accepted\n");
                client_socket = new_socket;
            }
        }

        if (client_socket != -1 && FD_ISSET(client_socket, &read_fds)) {
            char buffer[1024];
            ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

            if (bytes_received > 0) {
                printf("Received data from the client: %zd bytes\n", bytes_received);
            } 
            else if (bytes_received == 0) {
                printf("Client disconnected\n");
                close(client_socket);
                client_socket = -1;
            } else {
                perror("Error receiving data");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Close sockets before exiting
    close_sockets();

    return 0;
}
