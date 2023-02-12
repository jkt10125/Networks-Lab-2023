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

#define IP "127.0.0.1"
#ifndef PORT
#define PORT 20000
#endif
#define BUFLEN 50

enum CONTENT_TYPE
{
    HTML,
    PDF,
    JPG,
    OTHER
};

int getExtension(char *url)
{
    char *ext = strrchr(url, '.');
    if (ext == NULL)
        return OTHER;
    if (strcmp(ext, ".html") == 0)
        return HTML;
    if (strcmp(ext + 1, ".pdf"))
        return PDF;
    if (strcmp(ext + 1, ".jpg"))
        return JPG;
    return OTHER;
}


int readRequest(char *command, char *host, char *url, int* port_p, char *filepath)
{
    // command = (char *)malloc(5 * sizeof(char));
    // host = (char *)malloc(16 * sizeof(char));
    // url = (char *)malloc(1024 * sizeof(char));
    scanf("%[^ ]s", command);
    perror(command);
    if (strcmp(command, "QUIT") == 0)
    {
        return -1;
    }
    getchar();
    scanf("http://%[^/]s", host);
    perror(host);
    scanf("%s", url);
    perror(url);
    *port_p = 80;
    for (int i = 0; url[i]; i++)
    {
        if (url[i] == ':')
        {
            *port_p = atoi(url + i + 1);
            url[i] = 0;
            break;
        }
    }
    printf("%d\n", *port_p);
    // if(strcmp(command, "PUT")==0){
    //     getchar();
    //     scanf("%s", filepath);
    //     getchar();
    // }

    return 0;
}

int getConnection(char *host, int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect() failed");
        return -1;
    }

    return sockfd;
}

char *getRequest(char *command, char *host, char *url, char *filepath)
{
    time_t rawtime = time(NULL);
    char *request = (char *)malloc(4096);
    if (strcmp(command, "GET") == 0)
    {   
        printf("GET\n");
        sprintf(request+strlen(request), "%s %s %s\n", command, url, "HTTP/1.1");
        printf("%s\n", request);
        sprintf(request+strlen(request), "%s: %s\n", "Host", host);

        sprintf(request+strlen(request), "%s: ", "Accept");
        switch (getExtension(url))
        {
        case HTML:
            sprintf(request+strlen(request), "%s\n", "text/html");
            break;
        case PDF:
            sprintf(request+strlen(request), "%s\n", "application/pdf");
            break;
        case JPG:
            sprintf(request+strlen(request), "%s\n", "image/jpeg");
            break;
        case OTHER:
            sprintf(request+strlen(request), "%s\n", "text/*");
            break;
        default:
            break;
        }
        sprintf(request+strlen(request), "%s: %s\n", "Accept-Language", "en-US, en;q=0.9");

        sprintf(request+strlen(request), "%s: ", "Date");
        strftime(request+strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime));
        sprintf(request+strlen(request), "\n");

        rawtime -= 2 * 24 * 60 * 60;
        sprintf(request+strlen(request), "%s: ", "If-modified-since");
        strftime(request+strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime));
        sprintf(request+strlen(request), "\n");

        sprintf(request+strlen(request), "%s: %s\n", "Connection", "close");
        sprintf(request+strlen(request), "\n");
    }

    else if (strcmp(command, "PUT") == 0)
    {
        printf("GET\n");
        sprintf(request+strlen(request), "%s %s %s\n", command, url, "HTTP/1.1");     
        printf("GET\n");

        sprintf(request+strlen(request), "%s: ", "Date");
        strftime(request+strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&rawtime));
        sprintf(request+strlen(request), "\n");

        sprintf(request+strlen(request), "%s: %s\n", "Content-Language", "en-US");
        FILE *fp = fopen(filepath, "r");
        fseek(fp, 0L, SEEK_END);
        sprintf(request+strlen(request), "%s: %ld\n", "Content-length", ftell(fp));
        fclose(fp);
        sprintf(request+strlen(request), "%s: ", "Content-type");
        switch (getExtension(filepath))
        {
        case HTML:
            sprintf(request+strlen(request), "%s\n", "text/html");
            break;
        case PDF:
            sprintf(request+strlen(request), "%s\n", "application/pdf");
            break;
        case JPG:
            sprintf(request+strlen(request), "%s\n", "image/jpeg");
            break;
        case OTHER:
            sprintf(request+strlen(request), "%s\n", "text/*");
            break;
        default:
            break;
        }
        rawtime -= 2 * 24 * 60 * 60;
        struct tm *gtime = gmtime(&rawtime);
        sprintf(request+strlen(request), "%s: ", "If-modified-since");
        strftime(request+strlen(request), 30, "%a, %d %b %Y %H:%M:%S GMT", gtime);
        sprintf(request+strlen(request), "\n");

        sprintf(request+strlen(request), "%s: %s", "Connection", "close");
        sprintf(request+strlen(request), "\n");
    }
    return request;
}

int main()
{

    while (1)
    {
        char command[5], host[16], url[256], filepath[256], *request, *response;
        int port, sockfd;
        printf("MyOwnBrowser>");
        if (readRequest(command, host, url, &port, filepath) < 0)
            break;
        perror(command);
        perror(host);
        perror(url);
        printf("%d\n", port);
        if(strcmp(command, "GET")==0) perror(filepath);
        printf("%s %s %s %d %s\n", command, host, url, port, filepath);
        if ((sockfd = getConnection(host, port)) < 0)
        {
            perror("Cound not connect");
            continue;
        }
        request = getRequest(command, host, url, filepath);
        printf("%s\n", request);
        // if (sendRequest(sockfd, request) < 0)
        //     break;
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


