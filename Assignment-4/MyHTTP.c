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
        if (packet[i - 1] == '\n' && packet[i - 2] == '\n')
        {
            break;
        }
    }
    return packet;
}
int sendFile(int sockfd, const char *filepath)
{
    int fd, n;
    char buffer[BUFLEN];

    fd = open(filepath, O_RDONLY);
    if (fd < 0)
        return -1;
    printf("%d\n", fd);
    while ((n = read(fd, buffer, BUFLEN)) > 0)
    {
        if (send(sockfd, buffer, n, 0) < 0)
            return -1;
    }
    // if (send(sockfd, "", 1, 0) < 0)
    //     return -1;
    close(fd);
    return 0;
}

void communicate(int newsockfd)
{
    char *s = recvRequest(newsockfd);

    printf("%s", s);

    char command[5], url[256], http_version[10];
    strtok(s, "\n");
    sscanf(s, "%s %s %s", command, url, http_version);

    while (1)
    {
        char *header = strtok(NULL, "\n");
        if (header == NULL)
        {
            break;
        }
        if (header[0] == 0)
            break;
        char field[128], value[1024];
        sscanf(header, "%[^:]: %[^\n]", field, value);
    }

    printf("%s", generateResponse(command, "test.c"));

    if (strcmp(command, "GET")==0)
    {
        if (sendFile(newsockfd, url+1) < 0)
        {
            perror("Could not send file to client");
            return;
        }
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


char *getModifiedTime(char *filepath) {
    struct stat file_stat;

    if (stat(filepath, &file_stat) == -1) {
        perror("stat");
        return "-1";
    }

    return (ctime(&file_stat.st_mtime));
}

char *generateResponse(char *command, char *filepath) {
    char *response = strdup("");
    
    int status_code = 200;
    
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) status_code = 404;
    else fclose(fp);

    sprintf(response + strlen(response), "HTTP/1.1 %d OK\n", status_code);

    time_t rawtime = time(NULL);
    if (strcmp(command, "GET") == 0) {

        sprintf(response + strlen(response), "%s: ", "Expires");
        rawtime += 3 * 24 * 60 * 60;
        strftime(response + strlen(response), 31, "%a, %d %b %Y %H:%M:%S GMT\n", gmtime(&rawtime));

        sprintf(response + strlen(response), "%s: %s\n", "Cache-control", "no-store");


        if (status_code == 200) {

            sprintf(response + strlen(response), "%s: %s\n", "Content-language", "en-us");
            sprintf(response + strlen(response), "%s: ", "Content-type");

            switch (getExtension(filepath))
            {
                case HTML:
                    sprintf(response + strlen(response), "%s\n", "text/html");
                    break;
                case PDF:
                    sprintf(response + strlen(response), "%s\n", "application/pdf");
                    break;
                case JPG:
                    sprintf(response + strlen(response), "%s\n", "image/jpeg");
                    break;
                case OTHER:
                    sprintf(response + strlen(response), "%s\n", "text/*");
                    break;
            }

            FILE *fp = fopen(filepath, "r");
            if (fp == NULL)
            {
                perror("fopen");
                return response;
            }
            fseek(fp, 0L, SEEK_END);
            sprintf(response + strlen(response), "%s: %ld\n", "Content-length", ftell(fp));
            fclose(fp);

            sprintf(response + strlen(response), "%s: %s\n", "Last-modified", getModifiedTime(filepath));
        }
    }
    else {
        // do for PUT
    }
    return response;
}

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

        fprintf(stdout, "%s::%u ### connected\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

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
            fprintf(stdout, "%s::%u ### disconnected\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            exit(EXIT_SUCCESS);
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}