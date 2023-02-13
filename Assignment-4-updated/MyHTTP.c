#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>

#define MAXCLIENT 5
#define BUFLEN 50

enum FILE_TYPE
{
    HTML,
    PDF,
    JPG,
    OTHER
};

char *generateResponse(char *command, char *filepath);
int sendFile(int, const char *);

// recv until null-character is recieved, returns NULL if fails
char *recvRequest(int sockfd)
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

// send size_n bytes, return -1 if fails
int sendResponse(int sockfd, const char *request, int size_n)
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

void communicate(int newsockfd)
{

    char *request;
    if ((request = recvRequest(newsockfd)) == NULL)
    {
        perror("request");
        return;
    }

    char *content;
    char *headrequest = (char *)malloc(4096 * sizeof(char));

    for (int i = 0; i + 3 < strlen(request); i++)
    {
        headrequest[i] = request[i];
        if (request[i] == '\r' && request[i + 1] == '\n' && request[i + 2] == '\r' && request[i + 3] == '\n')
        {
            headrequest[i + 1] = request[i + 1];
            headrequest[i + 2] = request[i + 2];
            headrequest[i + 3] = request[i + 3];
            content = (char *)malloc(sizeof(char) * (strlen(request) - i - 3));
            for (int j = 0; j + i + 4 <= strlen(request); j++)
                content[j] = request[j + i + 4];
            break;
        }
    }

    printf("%s", headrequest);

    int status, type, len = -1, connection;
    char protocol[10], msg[50];
    char *header;

    char command[5], url[256], http_version[10];
    strtok(headrequest, "\n");
    sscanf(headrequest, "%s %s %s", command, url, http_version);

    // while ((header = strtok(NULL, "\r\n")) != NULL)
    // {
    //     char field[32], value[256];
    //     sscanf(header, "%[^:]: %[^\n]", field, value);
    //     if (strcmp(field, "Connection") == 0)
    //     {
    //         if (strcmp(value, "close") == 0)
    //             connection = 0;
    //         else
    //             connection = 1;
    //     }
    //     else if (strcmp(field, "Content-Length") == 0)
    //         len = atoi(value);
    //     else if (strcmp(field, "Content-Type") == 0)
    //     {
    //         strtok(value, ";");
    //         printf("%s\n", value);
    //         if (strcmp(value, "text/html") == 0)
    //             type = HTML;
    //         else if (strcmp(value, "application/pdf") == 0)
    //             type = PDF;
    //         else if (strcmp(value, "image/jpeg") == 0)
    //             type = JPG;
    //         else if (strcmp(value, "text/*") == 0)
    //             type = OTHER;
    //         else
    //             type = -1;
    //     }
    // }

    char *gen_response = generateResponse(command, &url[1]);

    sendResponse(newsockfd, gen_response, strlen(gen_response));

    sscanf(gen_response, "HTTP/1.1 %d", &status);

    if (status == 200 && strcmp(command, "GET") == 0)
    {
        if (sendFile(newsockfd, &url[1]) == -1)
        {
            perror("cant send");
            return;
        }
    }

    else if (status == 200 && strcmp(command, "PUT") == 0)
    {
        // todo
    }

    sendResponse(newsockfd, "", 1);

    printf("%s", gen_response);
}

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

struct tm *getModifiedTime(char *filepath)
{
    struct stat file_stat;

    if (stat(filepath, &file_stat) == -1)
    {
        perror("stat");
        return NULL;
    }

    return (gmtime(&file_stat.st_mtime));
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

char *generateResponsefor200(char *command, char *filepath)
{
    char *response = (char *)malloc(4096 * sizeof(char));
    memset(response, 0, 4096);
    sprintf(response + strlen(response), "HTTP/1.1 200 OK\n");
    if (strcmp(command, "GET") == 0)
    {

        sprintf(response + strlen(response), "%s: ", "Date");
        time_t rawtime = time(NULL);
        strftime(response + strlen(response), 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&rawtime));

        sprintf(response + strlen(response), "%s: ", "Expires");
        rawtime += 3 * 24 * 60 * 60;
        strftime(response + strlen(response), 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&rawtime));

        sprintf(response + strlen(response), "%s: %s\r\n", "Connection", "close");
        FILE *fp = fopen(filepath, "r");
        if (fp == NULL)
        {
            perror("fopen");
            return response;
        }
        fseek(fp, 0L, SEEK_END);
        sprintf(response + strlen(response), "%s: %ld\r\n", "Content-length", ftell(fp));
        fclose(fp);
        sprintf(response + strlen(response), "%s: %s\r\n", "Content-Language", "en-us");
        sprintf(response + strlen(response), "%s: ", "Content-Type");

        switch (getExtension(filepath))
        {
        case HTML:
            sprintf(response + strlen(response), "%s\r\n", "text/html");
            break;
        case PDF:
            sprintf(response + strlen(response), "%s\r\n", "application/pdf");
            break;
        case JPG:
            sprintf(response + strlen(response), "%s\r\n", "image/jpeg");
            break;
        case OTHER:
            sprintf(response + strlen(response), "%s\r\n", "text/*");
            break;
        }

        sprintf(response + strlen(response), "%s: %s\r\n", "Cache-Control", "no-store");

        sprintf(response + strlen(response), "%s: ", "Last-Modified");
        strftime(response + strlen(response), 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", getModifiedTime(filepath));
        sprintf(response + strlen(response), "\r\n");

        // SEND THE MENTIONED FILE
    }

    else if (strcmp(command, "PUT"))
    {
        sprintf(response + strlen(response), "%s: %s\r\n", "Connection", "close");
        sprintf(response + strlen(response), "%s: ", "Date");
        time_t rawtime = time(NULL);
        strftime(response + strlen(response), 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&rawtime));
        sprintf(response + strlen(response), "%s: %s\r\n", "Cache-Control", "no-store");
        sprintf(response + strlen(response), "\r\n");
    }
    return response;
}

char *generateResponsefor400()
{
    char *response = (char *)malloc(4096 * sizeof(char));

    char *html_str = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>\n400 Bad Request\n</title>\n</head>\n<body>\n<h1>Bad Request</h1>\n<p>Unknown error has occurred.</p>\n</body>\n</html>\r\n";

    memset(response, 0, 4096);
    sprintf(response + strlen(response), "HTTP/1.1 400 Bad Request\n");
    sprintf(response + strlen(response), "%s: %s\r\n", "Connection", "close");
    sprintf(response + strlen(response), "%s: %ld\r\n", "Content-Length", strlen(html_str));
    sprintf(response + strlen(response), "%s: %s\r\n", "Content-Type", "text/html");
    sprintf(response + strlen(response), "%s: %s\r\n", "Content-Language", "en-us");

    time_t rawtime = time(NULL);
    sprintf(response + strlen(response), "%s: ", "Date");
    strftime(response + strlen(response), 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&rawtime));

    sprintf(response + strlen(response), "\r\n");

    sprintf(response + strlen(response), "%s", html_str);
    // response[strlen(response)] = '\0';

    return response;
}

char *generateResponsefor403()
{
    char *response = (char *)malloc(4096 * sizeof(char));

    char *html_str = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>\n403 Forbidden\n</title>\n</head>\n<body>\n<h1>Forbidden</h1>\n<p>You don't have permission to access this resource.</p>\n</body>\n</html>\r\n";

    memset(response, 0, 4096);
    sprintf(response + strlen(response), "HTTP/1.1 403 Forbidden\n");
    sprintf(response + strlen(response), "%s: %s\r\n", "Connection", "close");
    sprintf(response + strlen(response), "%s: %ld\r\n", "Content-Length", strlen(html_str));
    sprintf(response + strlen(response), "%s: %s\r\n", "Content-Type", "text/html");
    sprintf(response + strlen(response), "%s: %s\r\n", "Content-Language", "en-us");

    time_t rawtime = time(NULL);
    sprintf(response + strlen(response), "%s: ", "Date");
    strftime(response + strlen(response), 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&rawtime));

    sprintf(response + strlen(response), "\r\n");

    sprintf(response + strlen(response), "%s", html_str);
    // response[strlen(response)] = '\0';

    return response;
}

char *generateResponsefor404()
{
    char *response = (char *)malloc(4096 * sizeof(char));

    char *html_str = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n<html>\n<head>\n<title>\n404 Not Found\n</title>\n</head>\n<body>\n<h1>Not Found</h1>\n<p>The page or file you requested doesn't exists</p>\n</body>\n</html>\r\n";

    memset(response, 0, 4096);
    sprintf(response + strlen(response), "HTTP/1.1 404 Not Found\n");
    sprintf(response + strlen(response), "%s: %s\r\n", "Connection", "close");
    sprintf(response + strlen(response), "%s: %ld\r\n", "Content-Length", strlen(html_str));
    sprintf(response + strlen(response), "%s: %s\r\n", "Content-Type", "text/html");
    sprintf(response + strlen(response), "%s: %s\r\n", "Content-Language", "en-us");

    time_t rawtime = time(NULL);
    sprintf(response + strlen(response), "%s: ", "Date");
    strftime(response + strlen(response), 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", gmtime(&rawtime));

    sprintf(response + strlen(response), "\r\n");

    sprintf(response + strlen(response), "%s", html_str);
    // response[strlen(response)] = '\0';

    return response;
}

char *generateResponse(char *command, char *filepath)
{
    if (access(filepath, F_OK) == -1)
        return generateResponsefor404();
    if (strcmp(command, "GET") == 0)
    {
        if (access(filepath, R_OK) == -1)
        {
            return generateResponsefor403();
        }
        return generateResponsefor200(command, filepath);
    }
    else if (strcmp(command, "PUT") == 0)
    {
        if (access(filepath, W_OK) == -1)
        {
            return generateResponsefor403();
        }
        return generateResponsefor200(command, filepath);
    }
    return generateResponsefor400();
}

// char *processRequest(char *request, int sockfd)
// {
//     char *header, *response;
//     time_t If_modified_since;
//     int Content_Length, connection = 1;

//     char command[10], url[10], http_protocol[10];
//     strtok(request, "\r\n");
//     sscanf(request, "%s %s %[^\n]", command, url, http_protocol);

//     response = generateResponse(command, &url[1]);
//     response = realloc(response, 409600 * sizeof(char));
//     // memset(&response[sizeof(response)], 0, 409600);

//     while ((header = strtok(NULL, "\r\n")) != NULL)
//     {
//         char field[32], value[256];
//         sscanf(header, "%[^:]: %[^\n]", field, value);
//         if (strcmp(field, "Connection") == 0)
//         {
//             if (strcmp(field, "close") == 0)
//                 connection = 0;
//         }
//         else if (strcmp(field, "Content-Length") == 0)
//             Content_Length = atoi(value);
//         else if (strcmp(field, "If-modified-since") == 0)
//         {
//             // If_modified_since = mktime(value);
//         }
//     }

//     if (strcmp(command, "GET") == 0)
//     {
//         if (mktime(getModifiedTime(&url[1])) > If_modified_since)
//         {
//         }
//     }

//     return response;
// }


int main(int argc, char *argv[])
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
    serv_addr.sin_port = htons(atoi(argv[1]));

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

        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork() failed");
            close(newsockfd);
            continue;
        }
        // child process
        if (pid == 0)
        {
            close(sockfd);
            communicate(newsockfd);
            close(newsockfd);
            exit(EXIT_SUCCESS);
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}