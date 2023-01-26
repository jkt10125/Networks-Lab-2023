/*
** Date: 19/01/2023 | Time: 19:49:15
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


int main() {

    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {

        perror ("Socket Creation Failed!");
        exit(EXIT_FAILURE);
    }

    memset (&servaddr, 0, sizeof(servaddr));
    printf("Socket Created Successfully!\n");

    // server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8181);
    inet_aton("127.0.0.1", &servaddr.sin_addr);

    struct pollfd fdset[1];
    int timeout = 3000;
    
    fdset[0].fd = sockfd;
    fdset[0].events = POLLIN;

    int ret;

    char msg[] = "time()";

    int send_buflen = strlen(msg);

    sendto(sockfd, msg, strlen(msg) + 1, 0, (struct sockaddr *) &servaddr, sizeof(servaddr));

    int tries = 5;
    do {

        ret = poll(fdset, 1, timeout);
        if (ret < 0) {

            perror("something went wrong in poll()\n");
            exit(0);
        }

        if (ret > 0) {
           
            break;
        }

        printf("Connection not Established! RETRYING...\n");
    
    } while (tries--);

    if (!ret) {

        perror("Connection Timeout");
        exit(0);
    }

    
    int buflen = 50;
    char *recv_buf = (char *) malloc(sizeof(char) * buflen);

    int len = sizeof(servaddr);
    recvfrom(sockfd, recv_buf, buflen, 0, (struct sockaddr *) &servaddr, &len);

    printf("Current Date and Time is :\n");
    printf ("%s\n", recv_buf);

    close(sockfd);

    return 0;
}