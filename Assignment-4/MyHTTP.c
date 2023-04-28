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
#define BUFLEN 100

enum FILE_TYPE
{
    HTML,
    PDF,
    JPG,
    OTHER
};

char *generateResponse(char *command, char *filepath);
struct tm *getModifiedTime(char *);
int sendFile(int, const char *);

char *recvRequest(int sockfd, int *requestlen)
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
            fprintf(stderr, "session terminated by client\n");
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
    *requestlen = i;
    return packet;
}

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
        int k;
        if ((k = send(sockfd, buffer, j, 0)) < 0)
            return -1;
    }
    return 0;
}

int putFile(char *filepath, char *content, int size_n)
{
    int fd, curr, j, n;
    char buf[BUFLEN];

    if ((fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC)) == -1)
    {
        return -1;
    }
    curr = 0;
    while (1)
    {
        j = 0;
        while (j < BUFLEN && curr + j < size_n)
        {
            buf[j] = content[curr + j];
            j++;
        }
        if ((n = write(fd, buf, j)) < 0)
            return -1;
        curr += n;
        if (curr == size_n)
            break;
    }
    close(fd);
    return 0;
}
void communicate(int newsockfd)
{
    char *request;
    int requestlen;
    if ((request = recvRequest(newsockfd, &requestlen)) == NULL)
    {
        perror("request");
        return;
    }

    char *content;
    int contentlen;
    char *headrequest = (char *)malloc(4096 * sizeof(char));
    memset(headrequest, 0, 4096);

    for (int i = 0; i + 3 < requestlen; i++)
    {
        headrequest[i] = request[i];
        if (request[i] == '\r' && request[i + 1] == '\n' && request[i + 2] == '\r' && request[i + 3] == '\n')
        {
            headrequest[i + 1] = request[i + 1];
            headrequest[i + 2] = request[i + 2];
            headrequest[i + 3] = request[i + 3];
            contentlen = (requestlen - i - 4);
            content = (char *)malloc(sizeof(char) * contentlen);
            for (int j = 0; j + i + 4 < requestlen; j++)
                content[j] = request[j + i + 4];
            break;
        }
    }

    printf("%s", headrequest);

    int status, type, len = -1, connection;
    char protocol[10], msg[50];
    char *header, *time_to_check;

    char command[5], url[256], http_version[10];
    strtok(headrequest, "\n");
    sscanf(headrequest, "%s %s %s", command, url, http_version);

    FILE *fp = fopen("AccessLog.txt", "a");
    if (fp == 0)
    {
        perror("fopen: AccessLog.txt");
    }
    fprintf(fp, "%s:%s\n", command, url);
    fclose(fp);

    while ((header = strtok(NULL, "\r\n")) != NULL)
    {
        char field[32], value[256];
        sscanf(header, "%[^:]: %[^\n]", field, value);
        if (strcmp(field, "Connection") == 0)
        {
            if (strcmp(value, "close") == 0)
                connection = 0;
            else
                connection = 1;
        }
        else if (strcmp(field, "Content-Length") == 0)
            len = atoi(value);
        else if (strcmp(field, "Content-Type") == 0)
        {
            strtok(value, ";");
            if (strcmp(value, "text/html") == 0)
                type = HTML;
            else if (strcmp(value, "application/pdf") == 0)
                type = PDF;
            else if (strcmp(value, "image/jpeg") == 0)
                type = JPG;
            else if (strcmp(value, "text/*") == 0)
                type = OTHER;
            else
                type = -1;
        }
        else if (strcmp(field, "If-modified-since") == 0)
        {
            time_to_check = value;
        }
    }

    char *gen_response = generateResponse(command, &url[1]);

    sscanf(gen_response, "HTTP/1.1 %d", &status);

    if (status == 200 && strcmp(command, "GET") == 0)
    {
        time_t timestamp = mktime(getModifiedTime(url + 1));

        if (sendResponse(newsockfd, gen_response, strlen(gen_response)) < 0)
        {
            perror("send response");
        }

        if (sendFile(newsockfd, &url[1]) == -1)
        {
            perror("cant send");
            return;
        }
    }

    else if (status == 200 && strcmp(command, "PUT") == 0)
    {
        if (len != -1)
        {
            int curr_byte = contentlen;
            char buffer[BUFLEN];
            int recv_byte;
            while (1)
            {
                if (curr_byte >= len + 1)
                    break;
                recv_byte = recv(newsockfd, buffer, BUFLEN, 0);
                if (recv_byte == 0)
                {
                    perror("connection closed by client");
                    return;
                }
                content = (char *)realloc(content, sizeof(char) * (curr_byte + recv_byte));
                int j = 0;
                while (j < recv_byte)
                {
                    content[curr_byte + j] = buffer[j];
                    j++;
                }
                curr_byte += recv_byte;
            }
        }
        else
            len = contentlen - 1;

        if (putFile(url + 1, content, len) < 0)
        {
            perror("put");
        }

        if (sendResponse(newsockfd, gen_response, strlen(gen_response)) < 0)
        {
            perror("send response");
        }
    }
    else
    {
        if (sendResponse(newsockfd, gen_response, strlen(gen_response)) < 0)
        {
            perror("send response");
        }
    }

    sendResponse(newsockfd, "", 1);

    // printf("%s", gen_response);
    int idx = 0;
    while (1)
    {
        printf("%c", gen_response[idx]);
        if (
            idx > 2 &&
            gen_response[idx] == '\n' &&
            gen_response[idx - 1] == '\r' &&
            gen_response[idx - 2] == '\n' &&
            gen_response[idx - 3] == '\r')
            break;
        idx++;
    }
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
        int k;
        if ((k = send(sockfd, buffer, n, 0)) < 0)
            return -1;
    }

    close(fd);

    return n;
}

char *generateResponsefor200(char *command, char *filepath)
{
    char *response = (char *)malloc(4096 * sizeof(char));
    memset(response, 0, 4096);
    sprintf(response + strlen(response), "HTTP/1.1 200 OK\r\n");
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
        sprintf(response + strlen(response), "%s: %ld\r\n", "Content-Length", ftell(fp));
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
    }

    else if (strcmp(command, "PUT") == 0)
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

    return response;
}

char *generateResponse(char *command, char *filepath)
{

    if (strcmp(command, "GET") == 0)
    {
        if (access(filepath, F_OK) == -1)
            return generateResponsefor404();

        if (access(filepath, R_OK) == -1)
        {
            return generateResponsefor403();
        }
        return generateResponsefor200(command, filepath);
    }
    else if (strcmp(command, "PUT") == 0)
    {
        if (access(filepath, F_OK) != -1 && access(filepath, W_OK) == -1)
        {
            return generateResponsefor403();
        }
        return generateResponsefor200(command, filepath);
    }
    return generateResponsefor400();
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: <executable>  <port>\n");
        return 1;
    }
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
        if (pid == 0)
        {
            close(sockfd);
            FILE *fp = fopen("AccessLog.txt", "a");
            if (fp == 0)
            {
                perror("fopen: AccessLog.txt");
            }
            char log[20];
            memset(log, 0, sizeof(log));
            time_t rawtime = time(NULL);
            strftime(log, sizeof(log), "%d%m%y:%H%M%S", gmtime(&rawtime));
            fprintf(fp, "%s:%s:%u:", log, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            fclose(fp);
            communicate(newsockfd);
            close(newsockfd);

            exit(EXIT_SUCCESS);
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}