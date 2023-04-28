/*
** Date: 08/01/2023 | Time: 13:29:58
** Name: Jay Kumar Thakur
** Rollno: 20CS30024
*/


/* Assumptions made:
    1. this program reads the input character by character
    2. client sends the data to the server in the from of packets
    3. the output sent by the server will fit in the string of 
        size bufsize
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

size_t bufsize = 10000; // MAX BUFFER LIMIT

// we send our expression in packets of size specified below to the server
const int packetSize = 10;

// this function reads the input character by character
void getLine(char **s) {
    char c = '?';
    
    char *tmp;
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

int main() {

    int sockfd; // variable for socket creation
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // checking for socket creation failures
    if (sockfd < 0) {
        perror("Socket creation failed!");
        exit(0);
    }
    else {
        printf("Socket Created Successfully!\n");
    }
    
    struct sockaddr_in ServerAddress;

    // setting the serveraddress specifications
    ServerAddress.sin_family = AF_INET;
    inet_aton("127.0.0.1", &ServerAddress.sin_addr);
    ServerAddress.sin_port = htons(20000);

    int connectStatus; // making a connect call to the server
    connectStatus = connect(sockfd, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress));

    // checking for connection failures
    if (connectStatus < 0) {
        
        perror("Connection failed!");
        exit(0);
    }
    else {
        
        printf("Connection Established!\n");
    }

    // creating send and receive buffer with sufficient size
    char *send_buf = "?";

    // size of the recv_buf updates on runtime basis
    char *recv_buf = (char *) malloc(sizeof(char) * bufsize);

    recv_buf[0] = '\0';
    
    // taking input from the user and fetching data from the server
    // until user enters "-1"
    while (strcmp(send_buf, "-1")) {
        
        printf("\n# ");

        // getting the input from the user
        getLine(&send_buf);



        // sending the data to the server
        Send(sockfd, send_buf);

        // sending the data to the server
        // send(sockfd, send_buf, strlen(send_buf), 0);

        // receiving the answer from the server
        int readLength = recv(sockfd, recv_buf, bufsize, 0);

        // printing the answer
        printf("## %s\n", recv_buf);

    }


    // closing the socket
    close(sockfd);

    return 0;
}