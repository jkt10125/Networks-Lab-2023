#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/in_systm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
    int sockfd, n, T;
    int on = 1;
    struct sockaddr srcaddr, destaddr;
    char src_site[256], *dest_site;
    struct hostent *dest, *src;
    if (getuid() != 0)
    {
        printf("This command requires root priviledges\n");
        exit(EXIT_FAILURE);
    }
    if (argc != 4)
    {
        printf("invalid command\n");
        printf("Usage: <executable> <site> <n> <T>\n");
        exit(EXIT_FAILURE);
    }
    if (gethostname(src_site, sizeof(src_site)) < 0)
    {
        perror("gethostname()");
        exit(EXIT_FAILURE);
    }
    dest_site = argv[1];
    n = atoi(argv[2]);
    T = atoi(argv[3]);
    if ((src = gethostbyname(src_site)) == NULL)
    {
        printf("%s: can't resolve, unknown source\n", dest_site);
        exit(EXIT_FAILURE);
    }
    else
    {
    }
    if ((dest = gethostbyname(dest_site)) == NULL)
    {
        printf("%s: can't resolve, unknown source\n", dest_site);
        exit(EXIT_FAILURE);
    }
    else
    {
    }
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, (const void *)&on, sizeof(on)) < 0)
    {
        perror("setsockopt with IP_HDRINCL option");
        exit(EXIT_FAILURE);
    }
    struct iphdr ip;
    return 0;
}