/*
** Date: 08/01/2023 | Time: 01:15:46
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

int main() {

    int sockfd; // variable for socket creation
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) {
        
        perror("Socket creation failed!");
        exit(0);
    }
    else {
        
        printf("Socket successfully created!...\n");
    }

    struct sockaddr_in ClintAddress, ServerAddress;
        // setting serveraddress specifications
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons (20000);
    ServerAddress.sin_addr.s_addr = INADDR_ANY;

    // binding with local address
    int bindStatus = bind(sockfd, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress));
    
    // checking for binding failures
    if (bindStatus < 0) {
        
        perror("Local Address bind failed!");
        exit(0);
    }
    else {
        printf("bind() successfully executed!\n");
    }

    // at max 5 clints can wait in the queue
    listen (sockfd, 5);

    int clientNumber = 0; // variable to count for clients that have connected so far
    while (1) {
        int ClintAddressLength = sizeof(ClintAddress);
        
        // making the accept call to the client
        int newsockfd = accept(sockfd, (struct sockaddr *) &ClintAddress, &ClintAddressLength);

        // checking for the accept failure
        if (newsockfd < 0) {
            
            perror("Accept failed!");
            exit(0);
        }
        else {
            
            printf("New Client Joined! #%d\n", ++clientNumber);
        }


        // getting the time information from the system

        time_t rawtime;
        time (&rawtime);

        struct tm *timeinfo = localtime(&rawtime);
        
        // storing the time in the buf string
        char *buf = strdup(asctime(timeinfo));
        
        
        int sendLength = send (newsockfd, buf, strlen(buf) + 1, 0);
        
        // checking for the send failure
        if (sendLength == 0) {
            printf ("Nothing sent...\n");
        }

        // closing the new socket
        close(newsockfd);
    }

    return 0;
}