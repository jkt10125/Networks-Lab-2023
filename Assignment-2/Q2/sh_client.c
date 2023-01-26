/*
** Date:   20/01/2023 | Time: 04:38:07
** Author: Jay Kumar Thakur
** Rollno: 20CS30024
*/


#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <poll.h>

int packetSize = 47;


// this function reads the input character by character
void getLine(char **s) {
    char c = '?';
    
    char *tmp;
    if (*s != NULL) {
        free(*s);
    }
    *s = NULL;

    size_t bufsize = 0;
    
    
    while (c != '\n') {

        scanf("%c", &c);

        *s = realloc(*s, ++bufsize);

        // checking for reallocation failure
        if (*s == NULL) {

            perror("reallocation failed!");
            exit(0);
        }
        
        tmp = *s + bufsize - 1;
        
        *tmp = c;
    }

    tmp = *s + bufsize - 1;

    *tmp = '\0';

}

// this function sends the data to the server  
// in form of packets of desired length
void Send(int sockfd, char *send_buf) {
    int len = strlen(send_buf);

    int idx = 0;

    while (idx < len - packetSize) {

        send(sockfd, &send_buf[idx], packetSize, 0);
        idx += packetSize;
    }


    send(sockfd, &send_buf[idx], len - idx + 1, 0);

}


// this fucntion reads the data sent by the server
// in the form of packets and stops receiving packets 
// when '\0' is encountered
void Recv(int sockfd, char **recv_buf) {
    
    int bufsize = 100000;
    if (*recv_buf != NULL) {

        free(*recv_buf);
    }
    
    *recv_buf = NULL;
    int recvBufSize = 0;
    
    char *temp_buf = (char *) malloc(sizeof(char) * bufsize);

    int readingIsAllowed = 1, idx = 0;
    
    int len = 0;

    while (readingIsAllowed) {

        len = recv(sockfd, temp_buf, bufsize, 0);

        if (temp_buf[len - 1] == '\0') {
            readingIsAllowed = 0;
        }

        *recv_buf = realloc(*recv_buf, idx + len);

        char *tmp = *recv_buf + idx;

        for (int i = 0; i < len; i++) {

            *(tmp++) = temp_buf[i];
        }

        idx += len;
    }

}




int main() {

    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
 
    if (sockfd < 0) {

        perror("Socket creation failed!");
        exit(0);
    }

    printf("Socket created successfully!\n");

    struct sockaddr_in ServerAddress;

    ServerAddress.sin_family = AF_INET;
    inet_aton("127.0.0.1", &ServerAddress.sin_addr);
    ServerAddress.sin_port = htons(20000);

    int connectStatus  = connect(sockfd, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress));

    if (connectStatus < 0) {

        perror("connection failed!");
        exit(0);
    }

    printf("Connection Established!\n");

    char *recv_buf = strdup("");

    // LOGIN: 
    Recv(sockfd, &recv_buf);
    
    printf("%s", recv_buf);

    char *username;
    // getting username from the user
    getLine(&username);

    // sending username back to the server to 
    // check its authenticity
    Send(sockfd, username);

    char *loginStatus;

    // reciving loginStatus from the server
    Recv(sockfd, &loginStatus);

    if (strcmp(loginStatus, "FOUND") == 0) {

        char *command = NULL;
        char *result = NULL;

        printf("$ ");
        getLine(&command);

        while (strcmp(command, "exit") != 0) {

            // sending the command to the server in form of packets
            Send(sockfd, command);

            // reciving the results from the server and displaying the 
            // appropriate message
            Recv(sockfd, &result);

            if (strcmp(result, "$$$$") == 0) {

                printf("Invalid command!\n");
            }

            else if (strcmp(result, "####") == 0) {

                printf("Error in running command\n");
            }
            else if (strlen(result) != 0) {

                printf("%s\n", result);
            }

            // getting the next command from the user
            printf("$ ");
            getLine(&command);

        }

        Send(sockfd, command);
    }

    else {

        printf("Invalid Username!");
    }

    close(sockfd);
    return 0;
}