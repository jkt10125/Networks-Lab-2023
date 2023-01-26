/*
** Date: 19/01/2023 | Time: 21:20:18
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

#include <time.h>

#define MAXLINE 1024

int main() {

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {

        perror("Socket Cration Failed!");
        exit(EXIT_FAILURE);
    }

    printf("Successfully created socket!\n");

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8181);

    int bindStatus;

    bindStatus = bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    if (bindStatus < 0) {

        perror("Bind() failed!");
        exit(0);
    }

    printf("Bind executed successfully\n");

    int clinum = 0;
    while (1) {
        socklen_t len = sizeof(cliaddr);
        
        char buffer[MAXLINE];

        int n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *) &cliaddr, &len);

        printf("New client connected! #%d\n", ++clinum);

        time_t rawtime;
        time (&rawtime);

        struct tm *timeinfo = localtime(&rawtime);
        
        // storing the time in the buf string
        char *buf = strdup(asctime(timeinfo));

        int sendLength = sendto(sockfd, buf, strlen(buf) + 1, 0, (struct sockaddr *) &cliaddr, len);

        if(sendLength > 0) {

            printf("Sent successfully!\n");
        }
    }    

    close (sockfd);

    return 0;
}