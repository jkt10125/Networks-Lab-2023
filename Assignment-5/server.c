#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mysocket.h"
#include <string.h>
#include <fcntl.h>
#define BUFLEN 100

int communicateWithClient(int sockfd)
{
    char *buf = NULL;
    size_t size = 0;
    int op;

    printf("Enter 0 to send, 1 to recieve, -1 to exit\n");
    scanf("%d", &op);
    getchar();
    if (op == -1)
        return -1;
    else if (op == 0)
    {
        int len = getline(&buf, &size, stdin);
        buf[len - 1] = 0;
        my_send(sockfd, buf, len, 0);
    }
    else
    {
        buf = (char *)malloc(1000);
        int recvlen = my_recv(sockfd, buf, 1000, 0);

        printf("%s\n", buf);
        free(buf);
    }
    return 0;
}
int main(int argc, char *argv[])
{
    if (argc != 2)
        exit(1);
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in servaddr, cliaddr;
    if ((sockfd = my_socket(AF_INET, SOCK_MyTCP, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }
    printf("socket created.....\n");
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));

    if (my_bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        perror("bind()");
        exit(1);
    }
    printf("socket binded.....\n");
    if (my_listen(sockfd, 10) < 0)
    {
        perror("listen()");
        exit(1);
    }
    printf("server listening.....\n");
    while (1)
    {
        clilen = sizeof(cliaddr);
        if ((newsockfd = my_accept(sockfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
        {
            perror("accept()");
            exit(1);
        }
        while (1)
        {
            if (communicateWithClient(newsockfd) < 0)
                break;
        }

        my_close(newsockfd);
    }

    my_close(sockfd);
    return 0;
}