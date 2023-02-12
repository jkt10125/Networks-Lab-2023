#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef PORT
#define PORT 20000
#endif
#define MAXCLIENT 5
#define BUFLEN 50

// send size_n bytes, return -1 if fails
int sendString(int sockfd, const char *input, int size_n)
{
    int i, j;
    char buffer[BUFLEN];

    i = 0;
    while (i < size_n)
    {
        memset(buffer, 0, BUFLEN);
        j = 0;
        while (j < BUFLEN && i < size_n)
        {
            buffer[j++] = input[i++];
        }
        if (send(sockfd, buffer, j, 0) < 0)
        {
            perror("send() failed");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// recv until null-character is recieved, returns NULL if fails
char *recvString(int sockfd)
{
    int i, j, recvlen, capacity = 1;
    char buffer[BUFLEN];
    char *packet = (char *)malloc(capacity * sizeof(char));

    i = 0;
    memset(buffer, 0, BUFLEN);
    while (1)
    {
        recvlen = recv(sockfd, buffer, BUFLEN, 0);
        if (recvlen < 0)
        {
            perror("recv() failed");
            free(packet);
            return NULL;
        }
        if (recvlen == 0)
        {
            fprintf(stderr, "session terminated (Ctrl+C)\n");
            free(packet);
            return NULL;
        }
        if (capacity < i + recvlen)
        {
            capacity += recvlen;
            packet = (char *)realloc(packet, capacity * sizeof(char));
        }

        j = 0;
        while (j < recvlen)
        {
            packet[i++] = buffer[j++];
        }
        if (packet[i - 1] == '\0')
        {
            break;
        }
    }
    return packet;
}

void communicate(int newsockfd){
}

int main()
{
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Socket created\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Local address binded\n");

    if (listen(sockfd, MAXCLIENT) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server listening.....\n\n");

    while (1)
    {
        clilen = sizeof(cli_addr);

        if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0)
        {
            perror("accept() failed");
            continue;
        }

        fprintf(stdout, "%s::%u ### connected\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        pid_t pid = fork();

        if(pid < 0){
            perror("fork() failed");
            close(newsockfd);
            continue;
        }
        // child process
        if (pid==0)
        {
            close(sockfd);
            communicate(newsockfd);
            close(newsockfd);
            fprintf(stdout, "%s::%u ### disconnected\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            exit(EXIT_SUCCESS);
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}