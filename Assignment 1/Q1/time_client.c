/*
** Date: 07/01/2023 | Time: 23:25:46
** Name: Jay Kumar Thakur
** Rollno: 20CS30024
*/


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int bufsize = 100;

int main() {
    
    int sockfd; // variable for socket creation
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    
    // checking for socket creation failures
    if (sockfd < 0) {
        
        perror("Socket creation failed!");
        exit(0);
    }
    else {

        printf("Successfully created socket!\n");
    }

    struct sockaddr_in ServerAddress;

    // setting the serveraddress specifications
    ServerAddress.sin_family = AF_INET;
    inet_aton ("127.0.0.1", &ServerAddress.sin_addr);
    ServerAddress.sin_port = htons (20000);

    // making a connect call to the server
    int connectStatus = connect (sockfd, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress));
    
    // checking for connections failures
    if (connectStatus < 0) {
        
        perror ("Unable to connect to the server!");
        exit(0);
    }

    char *buf = (char *) malloc(sizeof(char) * bufsize);


    // preparing the buffer string for receiving data from the server
    int receiveLength = recv(sockfd, buf, bufsize, 0);
    
    buf[receiveLength] = '\0';

    // printing the message
    printf("Current Date and Time is :\n");
    printf ("%s\n", buf);

    // closing the socket
    close (sockfd);
    return 0;
}