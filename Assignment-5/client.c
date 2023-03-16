#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mysocket.h"
#include <fcntl.h>
#define BUFLEN 100
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        exit(1);
    }
    int sockfd, recvlen;
    struct sockaddr_in servaddr;
    if ((sockfd = my_socket(AF_INET, SOCK_MyTCP, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }
    inet_aton("127.0.0.1", &servaddr.sin_addr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));

    if ((my_connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0))
    {
        perror("connect()");
        exit(1);
    }
    while (1)
    {
        char *buf = NULL;
        size_t size = 0;
        int op;

        printf("Enter 0 to send, 1 to recieve, -1 to exit\n");
        scanf("%d", &op);
        getchar();
        if (op == -1)
            break;
        else if (op == 0)
        {
            int len = getline(&buf, &size, stdin);
            buf[len - 1] = 0;
            my_send(sockfd, buf, len, 0);
        }
        else
        {
            buf = (char *)malloc(1000);
            recvlen = my_recv(sockfd, buf, 1000, 0);

            printf("%s\n", buf);
            free(buf);
        }
    }
    my_close(sockfd);
    return 0;
}