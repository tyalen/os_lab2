#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 5555

int main() {
    struct sockaddr_in serverAddress;
    int sockt;
    char* message = "Message from client";
    char buffer[1024] = { 0 };

    if ((sockt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error creating socket");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    // Converting address to the binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        printf("Error: Address not supported \n");
        exit(EXIT_FAILURE);
    }

    if (connect(sockt, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("Connection failed \n");
        exit(EXIT_FAILURE);
    }
    
    if(send(sockt, message, strlen(message), 0)<0){
    printf("Message sending failed \n");
    exit(EXIT_FAILURE);
    }
    
    printf("Message sent\n");

    close(sockt);

    return 0;
}
