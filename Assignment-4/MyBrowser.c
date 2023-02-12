#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFLEN 50

enum FILE_TYPE
{
    HTML,
    PDF,
    JPG,
    OTHER
};
enum READ_STATUS
{
    INVALID,
    NONE,
    QUIT,
    OK
};

int getExtension(char *filename)
{
    char *ext = strrchr(filename, '.');
    if (ext == NULL)
        return OTHER;
    if (strcmp(ext, ".html") == 0)
        return HTML;
    if (strcmp(ext, ".pdf") == 0)
        return PDF;
    if (strcmp(ext, ".jpg") == 0)
        return JPG;
    return OTHER;
}

int readRequest(char *command, char *host, char *url, int *port_p, char *filepath)
{
    command[0] = 0;
    host[0] = 0;
    url[0] = 0;
    filepath[0] = 0;

    char *input = NULL;
    size_t size = 0;
    int n = getline(&input, &size, stdin);
    while (input[n - 1] == ' ' || input[n - 1] == '\t' || input[n - 1] == '\n')
        input[--n] = 0;
    while (input[0] == ' ' || input[0] == '\t')
        input++;

    sscanf(input, "%s http://%[^/]%s %s", command, host, url, filepath);

    // printf("Command: %s\n", command);
    // printf("Host: %s\n", host);
    // printf("URL: %s\n", url);
    // printf("Filename: %s\n", filepath);

    if (command[0] == 0)
        return NONE;

    if (strcmp(command, "QUIT") == 0)
    {
        if (strcmp(input, "QUIT"))
            return INVALID;
        return QUIT;
    }
    if (strcmp(command, "GET") && strcmp(command, "PUT"))
        return INVALID;
    if (host[0] == 0 || url[0] == 0)
        return INVALID;
    if ((strcmp(command, "GET") == 0) ^ (filepath[0] == 0))
        return INVALID;

    strcpy(url, strtok(url, ":"));
    char *temp = strtok(NULL, ":");
    if (temp == NULL)
        *port_p = 80;
    else
        *port_p = atoi(temp);
    return OK;
}

int getConnection(char *host, int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    // printf("%s, %d\n", host, port);

    serv_addr.sin_family = AF_INET;
    inet_aton(host, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        return -1;

    return sockfd;
}

char *getRequest(char *command, char *host, char *url, char *filepath)
{
    time_t rawtime = time(NULL);
    char *request = (char *)malloc(4096);
    if (strcmp(command, "GET") == 0)
    {
        sprintf(request + strlen(request), "%s %s %s\n", command, url, "HTTP/1.1");

        sprintf(request + strlen(request), "%s: %s\n", "Host", host);

        sprintf(request + strlen(request), "%s: ", "Accept");
        switch (getExtension(url))
        {
        case HTML:
            sprintf(request + strlen(request), "%s\n", "text/html");
            break;
        case PDF:
            sprintf(request + strlen(request), "%s\n", "application/pdf");
            break;
        case JPG:
            sprintf(request + strlen(request), "%s\n", "image/jpeg");
            break;
        case OTHER:
            sprintf(request + strlen(request), "%s\n", "text/*");
            break;
        }

        sprintf(request + strlen(request), "%s: %s\n", "Accept-Language", "en-US, en;q=0.9");

        sprintf(request + strlen(request), "%s: ", "Date");
        strftime(request + strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime));
        sprintf(request + strlen(request), "\n");

        rawtime -= 2 * 24 * 60 * 60;
        sprintf(request + strlen(request), "%s: ", "If-modified-since");
        strftime(request + strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime));
        sprintf(request + strlen(request), "\n");

        sprintf(request + strlen(request), "%s: %s\n", "Connection", "close");

        sprintf(request + strlen(request), "\n");
    }

    else if (strcmp(command, "PUT") == 0)
    {
        sprintf(request + strlen(request), "%s %s %s\n", command, url, "HTTP/1.1");

        sprintf(request + strlen(request), "%s: ", "Date");
        strftime(request + strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime));
        sprintf(request + strlen(request), "\n");

        sprintf(request + strlen(request), "%s: %s\n", "Content-Language", "en-US");

        FILE *fp = fopen(filepath, "r");
        if (fp == NULL)
        {
            perror("fopen");
            return request;
        }
        fseek(fp, 0L, SEEK_END);
        sprintf(request + strlen(request), "%s: %ld\n", "Content-length", ftell(fp));
        fclose(fp);

        sprintf(request + strlen(request), "%s: ", "Content-type");

        switch (getExtension(filepath))
        {
        case HTML:
            sprintf(request + strlen(request), "%s\n", "text/html");
            break;
        case PDF:
            sprintf(request + strlen(request), "%s\n", "application/pdf");
            break;
        case JPG:
            sprintf(request + strlen(request), "%s\n", "image/jpeg");
            break;
        case OTHER:
            sprintf(request + strlen(request), "%s\n", "text/*");
            break;
        }

        rawtime -= 2 * 24 * 60 * 60;
        struct tm *gtime = gmtime(&rawtime);
        sprintf(request + strlen(request), "%s: ", "If-modified-since");
        strftime(request + strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gtime);
        sprintf(request + strlen(request), "\n");

        sprintf(request + strlen(request), "%s: %s\n", "Connection", "close");

        sprintf(request + strlen(request), "\n");
    }
    return request;
}

// send size_n bytes, return -1 if fails
int sendRequest(int sockfd, const char *request, int size_n)
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
            buffer[j++] = request[i++];
        }
        if (send(sockfd, buffer, j, 0) < 0)
            return -1;
    }
    return 0;
}

int sendFile(int sockfd, const char *filepath)
{
    int fd, n;
    char buffer[BUFLEN];

    fd = open(filepath, O_RDONLY);
    if (fd < 0)
        return -1;

    while ((n = read(fd, buffer, BUFLEN)) > 0)
    {
        if (send(sockfd, buffer, n, 0) < 0)
            return -1;
    }
    if (send(sockfd, "", 1, 0) < 0)
        return -1;
    close(fd);
    return 0;
}

int recvFile(int sockfd, const char *filepath)
{
    int fd, n;
    char buffer[BUFLEN];

    fd = open(filepath, O_WRONLY | O_CREAT);
    if (fd < 0)
        return -1;

    while ((n = recv(sockfd, buffer, BUFLEN, 0)) > 0)
    {
        if (write(fd, buffer, n) < 0)
            return -1;
    }
    close(fd);
    return 0;
}

int main()
{

    while (1)
    {
        char command[5], host[16], url[256], filepath[256], *request, *response;
        int port, sockfd;

        printf("MyOwnBrowser>");

        int status = readRequest(command, host, url, &port, filepath);
        if (status == INVALID)
        {
            printf("Invalid request!\n");
            continue;
        }
        if (status == NONE)
            continue;
        if (status == QUIT)
            break;

        if ((sockfd = getConnection(host, port)) < 0)
        {
            perror("Could not connect to server");
            continue;
        }

        request = getRequest(command, host, url, filepath);

        printf("%s\n", request);

        if (sendRequest(sockfd, request, strlen(request)) < 0)
        {
            perror("Could not send request to server");
            continue;
        }
        if (strcmp(command, "PUT") == 0)
        {
            if (sendFile(sockfd, filepath) < 0)
            {
                perror("Could not send file to server");
                continue;
            }
        }
        if (strcmp(command, "GET") == 0)
        {
            if(recvFile(sockfd, "temp.html")<0){
                perror("Could not recv file from server");
                continue;
            }
        }
        // if ((response = getResponse(sockfd)) == NULL)
        // {
        //     break;
        // }
        // processResponse(response);
        // close(sockfd);
        // fprintf(stdout, "Connection closed\n");
    }

    return 0;
}

// GET http://127.0.0.1/index.html:20000


