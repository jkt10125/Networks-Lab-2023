#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>

#define BUFLEN 50

void reapProcesses(int sig)
{
    while (1)
    {
        pid_t pid = waitpid(-1, NULL, WNOHANG);
        if (pid <= 0)
            break;
    }
}

void toggleSIGCHLDBlock(int how)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(how, &mask, NULL);
}

void blockSIGCHLD()
{
    toggleSIGCHLDBlock(SIG_BLOCK);
}
void unblockSIGCHLD()
{
    toggleSIGCHLDBlock(SIG_UNBLOCK);
}

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
        sprintf(request + strlen(request), "%s %s %s\r\n", command, url, "HTTP/1.1");

        sprintf(request + strlen(request), "%s: %s\r\n", "Host", host);

        sprintf(request + strlen(request), "%s: ", "Accept");
        switch (getExtension(url))
        {
        case HTML:
            sprintf(request + strlen(request), "%s\r\n", "text/html");
            break;
        case PDF:
            sprintf(request + strlen(request), "%s\r\n", "application/pdf");
            break;
        case JPG:
            sprintf(request + strlen(request), "%s\r\n", "image/jpeg");
            break;
        case OTHER:
            sprintf(request + strlen(request), "%s\r\n", "text/*");
            break;
        }

        sprintf(request + strlen(request), "%s: %s\r\n", "Accept-Language", "en-US, en;q=0.9");

        sprintf(request + strlen(request), "%s: ", "Date");
        strftime(request + strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime));
        sprintf(request + strlen(request), "\r\n");

        rawtime -= 2 * 24 * 60 * 60;
        sprintf(request + strlen(request), "%s: ", "If-modified-since");
        strftime(request + strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime));
        sprintf(request + strlen(request), "\r\n");

        sprintf(request + strlen(request), "%s: %s\r\n", "Connection", "close");

        sprintf(request + strlen(request), "\r\n");
    }

    else if (strcmp(command, "PUT") == 0)
    {
        sprintf(request + strlen(request), "%s %s %s\r\n", command, url, "HTTP/1.1");

        sprintf(request + strlen(request), "%s: ", "Date");
        strftime(request + strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime));
        sprintf(request + strlen(request), "\r\n");

        sprintf(request + strlen(request), "%s: %s\r\n", "Content-Language", "en-US");

        FILE *fp = fopen(filepath, "r");
        if (fp == NULL)
        {
            perror("fopen");
            return request;
        }
        fseek(fp, 0L, SEEK_END);
        sprintf(request + strlen(request), "%s: %ld\r\n", "Content-length", ftell(fp));
        fclose(fp);

        sprintf(request + strlen(request), "%s: ", "Content-type");

        switch (getExtension(filepath))
        {
        case HTML:
            sprintf(request + strlen(request), "%s\r\n", "text/html");
            break;
        case PDF:
            sprintf(request + strlen(request), "%s\r\n", "application/pdf");
            break;
        case JPG:
            sprintf(request + strlen(request), "%s\r\n", "image/jpeg");
            break;
        case OTHER:
            sprintf(request + strlen(request), "%s\r\n", "text/*");
            break;
        }

        rawtime -= 2 * 24 * 60 * 60;
        struct tm *gtime = gmtime(&rawtime);
        sprintf(request + strlen(request), "%s: ", "If-modified-since");
        strftime(request + strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gtime);
        sprintf(request + strlen(request), "\r\n");

        sprintf(request + strlen(request), "%s: %s\r\n", "Connection", "close");

        sprintf(request + strlen(request), "\r\n");
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

    close(fd);

    return n;
}

int recvFile(int sockfd, int length, int type)
{
    int fd, n, torecv;
    char buffer[BUFLEN];
    char * args[3];

    if (type == -1)
    {
        while (1)
        {
            torecv = BUFLEN;
            if (length < torecv)
                torecv = length;
            n = recv(sockfd, buffer, torecv, 0);
            if (n < 0)
                return -1;
            length -= n;
            if (length == 0)
                break;
        }
        return 0;
    }
    if (type == HTML)
    {
        fd = open(".temp.html", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        args[0] = strdup("firefox");
        args[1] = strdup(".temp.html");
        args[2] = NULL;
    }
    else if (type == PDF)
    {
        fd = open(".temp.pdf", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        args[0] = strdup("acroread");
        args[1] = strdup(".temp.pdf");
        args[2] = NULL;
    }
    else if (type == JPG)
    {
        fd = open(".temp.jpg", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        args[0] = strdup("eog");
        args[1] = strdup(".temp.jpg");
        args[2] = NULL;
    }
    else
    {
        fd = open(".temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        args[0] = strdup("gedit");
        args[1] = strdup(".temp.txt");
        args[2] = NULL;
    }

    if (fd < 0)
        return -1;
    while (1)
    {
        torecv = BUFLEN;
        if (length < torecv)
            torecv = length;
        n = recv(sockfd, buffer, torecv, 0);
        if (n < 0)
            return -1;
        length -= n;
        if (length == 0)
            break;
    }
    close(fd);

    blockSIGCHLD();
    if (fork() == 0)
    {
        unblockSIGCHLD();
        execvp(args[0], args);
        exit(0);
    }
    unblockSIGCHLD();
    return 0;
}

char *getResponse(int sockfd)
{
    int i, j, ret, recvlen, capacity = 1;
    char buffer[BUFLEN];

    struct pollfd fdset;
    fdset.fd = sockfd;
    fdset.events = POLLIN;
    ret = poll(&fdset, 1, 3000);

    if (ret <= 0)
    {
        return NULL;
    }

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
        if (i >= 4 && packet[i - 4] == '\r' && packet[i - 3] == '\n' && packet[i - 2] == '\r' && packet[i - 1] == '\n')
        {
            break;
        }
    }
    return packet;
}

void processResponse(char *response, int sockfd)
{
    int status, type, len = -1, connection;
    char protocol[10], msg[50];
    char *header;

    strtok(response, "\r\n");
    sscanf(response, "%s %d %[^\n]", protocol, status, msg);

    while ((header = strtok(NULL, "\r\n")) != NULL)
    {
        char field[32], value[256];
        sscanf(header, "%[^:]: %[^\n]", field, value);
        if (strcmp(field, "Connection") == 0)
        {
            if (strcmp(field, "close") == 0)
                connection = 0;
            else
                connection = 1;
        }
        else if (strcmp(field, "Content-Length") == 0)
            len = atoi(value);
        else if (strcmp(field, "Content-Type") == 0)
        {
            if (strcmp(field, "text/html") == 0)
                type = HTML;
            else if (strcmp(field, "application/pdf") == 0)
                type = PDF;
            else if (strcmp(field, "image/jpeg") == 0)
                type = JPG;
            else if (strcmp(field, "text/*") == 0)
                type = OTHER;
            else
                type = -1;
        }
    }
    if (len != -1)
    {
        if (recvFile(sockfd, len, type) < 0)
        {
            perror("Could not recv content from server");
        }
    }
    if (connection == 0)
    {
        close(sockfd);
    }
}

int main()
{

    signal(SIGCHLD, reapProcesses);

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

        if ((response = getResponse(sockfd)) == NULL)
        {
            if (errno == 0)
                printf("Timeout! Try again\n");
            else
                perror("Could not response from server");
            continue;
        }
        processResponse(response, sockfd);
    }

    return 0;
}

// GET http://127.0.0.1/index.html:20000
