#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <netdb.h>
#define MAX_SIZE 1500
#define IP_MAXLEN 16
#define DWORD 4
#define IPv4 4
#define ALPHA 0.125
#define PORT 20000
uint16_t checksum(const void *buff, size_t nbytes);
char *dnsLookup(const char *h_name, struct sockaddr_in *addr);
char *niLookup(int ni_family, struct sockaddr_in *addr);
struct sockaddr_in getIntermediateNode(int ttl);
float getBandwidth(int payload, float rtt1, float rtt2);
void printIP(struct iphdr *ip);
void printICMP(struct icmphdr *icmp);

uint16_t ip_id = 0;
int sockfd, n, T;
const int ON = 1;
char *src_ip, *dest_ip;
struct sockaddr_in srcaddr, destaddr;

int main(int argc, char *argv[])
{
    if (getuid() != 0)
    {
        printf("This application requires root priviledges\n");
        exit(EXIT_FAILURE);
    }
    if (argc != 4)
    {
        printf("Invalid command\n");
        printf("Usage: <executable> <site_to_probe> <n> <T>\n");
        exit(EXIT_FAILURE);
    }

    if ((src_ip = niLookup(AF_INET, &srcaddr)) == NULL)
    {
        exit(EXIT_FAILURE);
    }
    if ((dest_ip = dnsLookup(argv[1], &destaddr)) == NULL)
    {
        exit(EXIT_FAILURE);
    }
    printf("%s\n", src_ip);
    printf("%s\n", dest_ip);

    n = atoi(argv[2]);
    T = atoi(argv[3]);

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, (const void *)&ON, sizeof(ON)) < 0)
    {
        perror("setsockopt with IP_HDRINCL option");
        exit(EXIT_FAILURE);
    }
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt with IP_HDRINCL option");
        exit(EXIT_FAILURE);
    }
    int ttl = 0;
    while (++ttl)
    {
        struct sockaddr_in imaddr = getIntermediateNode(ttl);
        struct hostent *imhost = gethostbyaddr(&imaddr.sin_addr, sizeof(imaddr.sin_addr), imaddr.sin_family);

        if (imaddr.sin_addr.s_addr == 0)
        {
            printf("%d\t*******\n", ttl);
            continue;
        }
        if (imhost == NULL)
            printf("%d %s\n", ttl, inet_ntoa(imaddr.sin_addr));
        else
            printf("%d %s\(%s\)\n", ttl, inet_ntoa(imaddr.sin_addr), imhost->h_name);
        int cnt = 0;
        float rtt_avg = 0;
        uint16_t icmpID = ttl, seq = rand() % 0xFFFF;
        for (int i = 0; i < n; i++)
        {
            uint16_t buflen = sizeof(struct iphdr) + sizeof(struct icmphdr);
            char *buffer = (char *)malloc(buflen * sizeof(char));
            struct iphdr *ip = (struct iphdr *)buffer;
            struct icmphdr *icmp = (struct icmphdr *)(buffer + sizeof(struct iphdr));
            struct sockaddr_in addr;
            socklen_t addrlen = sizeof(addr);
            char *recv_buffer = (char *)malloc(MAX_SIZE * sizeof(char));
            int recvlen;
            struct timeval send_tv, recv_tv;
            struct iphdr *ip_reply;
            struct icmphdr *icmp_reply;

            ip->version = IPv4;
            ip->ihl = sizeof(struct iphdr) / DWORD;
            ip->tos = 0;
            ip->tot_len = htons(buflen);
            ip->id = ++ip_id;
            ip->frag_off = 0;
            ip->ttl = 2 * ttl;
            ip->protocol = IPPROTO_ICMP;
            ip->check = 0;
            ip->saddr = srcaddr.sin_addr.s_addr;
            ip->daddr = imaddr.sin_addr.s_addr;
            ip->check = checksum(ip, sizeof(struct iphdr));

            icmp->type = ICMP_ECHO;
            icmp->code = 0;
            icmp->checksum = 0;
            icmp->un.echo.id = icmpID;
            icmp->un.echo.sequence = seq++;
            icmp->checksum = checksum(icmp, sizeof(struct icmphdr));

            assert(checksum(ip, sizeof(struct iphdr)) == 0);
            assert(checksum(icmp, sizeof(struct icmphdr)) == 0);

            // printIP(ip);
            // printICMP(icmp);

            if (sendto(sockfd, buffer, buflen, 0, (const struct sockaddr *)&imaddr, sizeof(imaddr)) < 0)
            {
                perror("sendto()");
                exit(EXIT_FAILURE);
            }
            if (gettimeofday(&send_tv, NULL) < 0)
            {
                perror("gettimeofday");
                exit(EXIT_FAILURE);
            }

            if ((recvlen = recvfrom(sockfd, recv_buffer, MAX_SIZE, 0, (struct sockaddr *)&addr, &addrlen)) < 0)
            {
                if (errno == EAGAIN)
                {
                    continue;
                }
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }
            if (gettimeofday(&recv_tv, NULL) < 0)
            {
                perror("gettimeofday");
                exit(EXIT_FAILURE);
            }
            int rtt_m = 1000000 * (recv_tv.tv_sec - send_tv.tv_sec) + (recv_tv.tv_usec - send_tv.tv_usec);
            printf("recieved %d bytes from %s: time = %d us\n", recvlen, inet_ntoa(addr.sin_addr), rtt_m);
            ip_reply = (struct iphdr *)recv_buffer;
            // printIP(ip_reply);
            if (ip_reply->protocol == IPPROTO_ICMP)
            {
                icmp_reply = (struct icmphdr *)(recv_buffer + DWORD * ip_reply->tot_len);
                // printICMP(icmp_reply);
                if (icmp_reply->type == ICMP_ECHOREPLY)
                {
                    rtt_avg = (rtt_avg * cnt + rtt_m) / (float)(cnt + 1);
                    cnt++;
                }
            }
        }
        if (imhost != NULL)
            printf("%d %s\(%s\): rtt_avg = %f us\n", ttl, inet_ntoa(imaddr.sin_addr), imhost->h_name, rtt_avg);
        else
            printf("%d %s: rtt_avg = %f us\n", ttl, inet_ntoa(imaddr.sin_addr), rtt_avg);

        cnt = 0;
        float rtt_avg2 = 0;
        int payload = 1000;

        for (int i = 0; i < n; i++)
        {
            uint16_t buflen = sizeof(struct iphdr) + sizeof(struct icmphdr) + payload;
            char *buffer = (char *)malloc(buflen * sizeof(char));
            struct iphdr *ip = (struct iphdr *)buffer;
            struct icmphdr *icmp = (struct icmphdr *)(buffer + sizeof(struct iphdr));
            char *data = buffer + sizeof(struct iphdr) + sizeof(struct icmphdr);
            struct sockaddr_in addr;
            socklen_t addrlen = sizeof(addr);
            char *recv_buffer = (char *)malloc(MAX_SIZE * sizeof(char));
            int recvlen;
            struct timeval send_tv, recv_tv;
            struct iphdr *ip_reply;
            struct icmphdr *icmp_reply;

            ip->version = IPv4;
            ip->ihl = sizeof(struct iphdr) / DWORD;
            ip->tos = 0;
            ip->tot_len = htons(buflen);
            ip->id = ++ip_id;
            ip->frag_off = 0;
            ip->ttl = 2 * ttl;
            ip->protocol = IPPROTO_ICMP;
            ip->check = 0;
            ip->saddr = srcaddr.sin_addr.s_addr;
            ip->daddr = imaddr.sin_addr.s_addr;
            ip->check = checksum(ip, sizeof(struct iphdr));

            icmp->type = ICMP_ECHO;
            icmp->code = 0;
            icmp->checksum = 0;
            icmp->un.echo.id = icmpID;
            icmp->un.echo.sequence = seq++;

            for (int i = 0; i < payload; i++)
                data[i] = rand() % 0x80;

            icmp->checksum = checksum(icmp, sizeof(struct icmphdr) + payload);

            assert(checksum(ip, sizeof(struct iphdr)) == 0);
            assert(checksum(icmp, sizeof(struct icmphdr) + payload) == 0);

            // printIP(ip);
            // printICMP(icmp);

            if (sendto(sockfd, buffer, buflen, 0, (const struct sockaddr *)&imaddr, sizeof(imaddr)) < 0)
            {
                perror("sendto()");
                exit(EXIT_FAILURE);
            }
            if (gettimeofday(&send_tv, NULL) < 0)
            {
                perror("gettimeofday");
                exit(EXIT_FAILURE);
            }

            if ((recvlen = recvfrom(sockfd, recv_buffer, MAX_SIZE, 0, (struct sockaddr *)&addr, &addrlen)) < 0)
            {
                if (errno == EAGAIN)
                {
                    continue;
                }
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }
            if (gettimeofday(&recv_tv, NULL) < 0)
            {
                perror("gettimeofday");
                exit(EXIT_FAILURE);
            }
            int rtt_m = 1000000 * (recv_tv.tv_sec - send_tv.tv_sec) + (recv_tv.tv_usec - send_tv.tv_usec);
            printf("recieved %d bytes from %s: time = %d us\n", recvlen, inet_ntoa(addr.sin_addr), rtt_m);
            ip_reply = (struct iphdr *)recv_buffer;
            // printIP(ip_reply);
            if (ip_reply->protocol == IPPROTO_ICMP)
            {
                icmp_reply = (struct icmphdr *)(recv_buffer + DWORD * ip_reply->tot_len);
                // printICMP(icmp_reply);
                if (icmp_reply->type == ICMP_ECHOREPLY)
                {
                    rtt_avg2 = (rtt_avg2 * cnt + rtt_m) / (float)(cnt + 1);
                    cnt++;
                }
            }
        }
        if (imhost != NULL)
            printf("%d %s\(%s\): rtt_avg2 = %f us\n", ttl, inet_ntoa(imaddr.sin_addr), imhost->h_name, rtt_avg2);
        else
            printf("%d %s: rtt_avg2 = %f us\n", ttl, inet_ntoa(imaddr.sin_addr), rtt_avg2);

        printf("Latency: %f us, Bandwidth: %f Mbps\n", rtt_avg, getBandwidth(payload, rtt_avg, rtt_avg2));
        if (imaddr.sin_addr.s_addr == destaddr.sin_addr.s_addr)
            break;
    }
    return 0;
}

float getBandwidth(int payload, float rtt1, float rtt2)
{
    float bw = 8 * payload / (rtt1 - rtt2);
    if (bw < 0)
        bw *= -1;
    return bw;
}
struct sockaddr_in getIntermediateNode(int ttl)
{
    uint16_t buflen = sizeof(struct iphdr) + sizeof(struct icmphdr);
    char *buffer = (char *)malloc(buflen * sizeof(char));
    struct iphdr *ip = (struct iphdr *)buffer;
    struct icmphdr *icmp = (struct icmphdr *)(buffer + sizeof(struct iphdr));
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char *recv_buffer = (char *)malloc(MAX_SIZE * sizeof(char));
    ssize_t recvlen;
    struct iphdr *ip_reply;
    struct icmphdr *icmp_reply;
    uint16_t icmpID = ttl;
    uint16_t seq = rand() % 0xFFFF;
    int flag = 0;

    for (int i = 0; i < 5; i++)
    {
        ip->version = IPv4;
        ip->ihl = sizeof(struct iphdr) / DWORD;
        ip->tos = 0;
        ip->tot_len = htons(buflen);
        ip->id = ++ip_id;
        ip->frag_off = 0;
        ip->ttl = ttl;
        ip->protocol = IPPROTO_ICMP;
        ip->check = 0;
        ip->saddr = srcaddr.sin_addr.s_addr;
        ip->daddr = destaddr.sin_addr.s_addr;
        ip->check = checksum(ip, sizeof(struct iphdr));

        icmp->type = ICMP_ECHO;
        icmp->code = 0;
        icmp->checksum = 0;
        icmp->un.echo.id = icmpID;
        icmp->un.echo.sequence = seq++;
        icmp->checksum = checksum(icmp, sizeof(struct icmphdr));

        assert(checksum(ip, sizeof(struct iphdr)) == 0);
        assert(checksum(icmp, sizeof(struct icmphdr)) == 0);

        // printIP(ip);
        // printICMP(icmp);

        if (sendto(sockfd, buffer, buflen, 0, (const struct sockaddr *)&destaddr, sizeof(destaddr)) < 0)
        {
            perror("sendto()");
            exit(EXIT_FAILURE);
        }
        if ((recvlen = recvfrom(sockfd, recv_buffer, MAX_SIZE, 0, (struct sockaddr *)&addr, &addrlen)) < 0)
        {
            if (errno == EAGAIN)
            {
                flag = 1;
            }
            else
            {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            printf("recieved %d bytes from %s\n", recvlen, inet_ntoa(addr.sin_addr));

            ip_reply = (struct iphdr *)recv_buffer;
            // printIP(ip_reply);
            if (ip_reply->protocol == IPPROTO_ICMP)
            {
                icmp_reply = (struct icmphdr *)(recv_buffer + DWORD * ip_reply->ihl);
                // printICMP(icmp_reply);
                if (icmp_reply->type == ICMP_ECHOREPLY || icmp_reply->type == ICMP_TIME_EXCEEDED)
                {
                    if (icmp->un.echo.id == icmp_reply->un.echo.id && icmp->un.echo.sequence == icmp_reply->un.echo.sequence)
                    {
                    }
                }
            }
        }
    }
    free(buffer);
    free(recv_buffer);
    if (flag == 1)
    {
        addr.sin_addr.s_addr = 0;
    }
    return addr;
}

/**
 * @brief returns 16-bit checksum
 *
 * @param buff bitstream whose checksum is to be computed
 * @param nbytes number of bytes in buff
 */
uint16_t checksum(const void *buff, size_t nbytes)
{
    uint64_t sum = 0;
    uint16_t *words = (uint16_t *)buff;
    size_t _16bitword = nbytes / 2;
    while (_16bitword--)
    {
        sum += *(words++);
    }
    if (nbytes & 1)
    {
        sum += (uint16_t)(*(uint8_t *)words) << 0x0008;
    }
    sum = ((sum >> 16) + (sum & 0xFFFF));
    sum += (sum >> 16);
    return (uint16_t)(~sum);
}

/**
 * @brief resolves a hostname into an IP address in numbers-and-dots notation
 *
 * @param h_name hostname
 * @param addr sockaddr of hostname filled up, if NOT null
 */
char *dnsLookup(const char *h_name, struct sockaddr_in *addr)
{
    struct hostent *host;
    if ((host = gethostbyname(h_name)) == NULL || host->h_addr_list[0] == NULL)
    {
        printf("%s: can't resolve\n", h_name);
        switch (h_errno)
        {
        case HOST_NOT_FOUND:
            printf("Host not found\n");
            break;
        case TRY_AGAIN:
            printf("Non-authoritative. Host not found\n");
            break;
        case NO_DATA:
            printf("Valid name, no data record of requested type\n");
            break;
        case NO_RECOVERY:
            printf("Non-recoverable error\n");
            break;
        }
        return NULL;
    }
    // check AF_INET
    if (addr != NULL)
    {
        addr->sin_family = host->h_addrtype;
        addr->sin_port = htons(PORT);
        addr->sin_addr = *(struct in_addr *)host->h_addr_list[0];
    }
    return strdup(inet_ntoa(*(struct in_addr *)host->h_addr_list[0]));
}

/**
 * @brief returns an IP address in numbers-and-dots notation to the network interface of specified family in the local system
 *
 * @param ni_family family of network interface
 * @param addr sockaddr of network interface filled up, if NOT null
 */
char *niLookup(int ni_family, struct sockaddr_in *addr)
{
    struct ifaddrs *ifaddr, *it;
    if (getifaddrs(&ifaddr) < 0)
    {
        perror("getifaddrs");
        return NULL;
    }
    it = ifaddr;
    while (it != NULL)
    {
        if (it->ifa_addr != NULL && it->ifa_addr->sa_family == ni_family && !(it->ifa_flags & IFF_LOOPBACK) && (it->ifa_flags & IFF_RUNNING))
        {
            if (addr != NULL)
            {
                addr->sin_family = ((struct sockaddr_in *)it->ifa_addr)->sin_family;
                addr->sin_port = ((struct sockaddr_in *)it->ifa_addr)->sin_port;
                addr->sin_addr = ((struct sockaddr_in *)it->ifa_addr)->sin_addr;
            }
            break;
        }
        it = it->ifa_next;
    }
    freeifaddrs(ifaddr);

    if (it == NULL)
    {
        printf("No active network interface of family: %d found\n", ni_family);
        return NULL;
    }
    return strdup(inet_ntoa(((struct sockaddr_in *)it->ifa_addr)->sin_addr));
}

void printIP(struct iphdr *ip)
{
    printf("-----------------------------------------------------------------\n");
    printf("|   version:%-2d  |   hlen:%-4d   |     tos:%-2d    |  totlen:%-4d  |\n", ip->version, ip->ihl, ip->tos, ntohs(ip->tot_len));
    printf("-----------------------------------------------------------------\n");
    printf("|           id:%-6d           |%d|%d|%d|      frag_off:%-4d      |\n", ntohs(ip->id), ip->frag_off && (1 << 15), ip->frag_off && (1 << 14), ip->frag_off && (1 << 14), ip->frag_off);
    printf("-----------------------------------------------------------------\n");
    printf("|    ttl:%-4d   |  protocol:%-2d  |         checksum:%-6d       |\n", ip->ttl, ip->protocol, ip->check);
    printf("-----------------------------------------------------------------\n");
    printf("|                    source:%-16s                    |\n", inet_ntoa(*(struct in_addr *)&ip->saddr));
    printf("-----------------------------------------------------------------\n");
    printf("|                 destination:%-16s                  |\n", inet_ntoa(*(struct in_addr *)&ip->daddr));
    printf("-----------------------------------------------------------------\n");
}

void printICMP(struct icmphdr *icmp)
{
    printf("-----------------------------------------------------------------\n");
    printf("|    type:%-2d    |    code:%-2d    |        checksum:%-6d        |\n", icmp->type, icmp->code, icmp->checksum);
    printf("-----------------------------------------------------------------\n");
    if (icmp->type == ICMP_ECHO || icmp->type == ICMP_ECHOREPLY)
    {
        printf("|           id:%-6d           |        sequence:%-6d        |\n", icmp->un.echo.id, icmp->un.echo.sequence);
        printf("-----------------------------------------------------------------\n");
    }
    if (icmp->type == ICMP_TIME_EXCEEDED)
    {
    }
    if (icmp->type == ICMP_SOURCE_QUENCH)
    {
    }
    if (icmp->type == ICMP_DEST_UNREACH)
    {
    }
}

// void *getPacket(uint8_t ttl, uint32_t saddr, uint32_t daddr, uint16_t icmp_id, uint16_t icmp_seq, char *msg, size_t msglen)
// {
//     if (msg == NULL)
//         msglen = 0;
//     int packlen = sizeof(struct iphdr) + sizeof(struct icmphdr) + msglen;
//     char *buffer = (char *)malloc(packlen * sizeof(char));
//     struct iphdr *ip = (struct iphdr *)buffer;
//     struct icmphdr *icmp = (struct icmphdr *)(buffer + sizeof(struct iphdr));
//     char *data = (char *)(buffer + sizeof(struct iphdr) + sizeof(struct icmphdr));
//     ip->version = IPv4;
//     ip->ihl = sizeof(struct iphdr) / DWORD;
//     ip->tos = 0;
//     ip->tot_len = htons(packlen);
//     ip->id = ++ip_id;
//     ip->frag_off = 0;
//     ip->ttl = ttl;
//     ip->protocol = IPPROTO_ICMP;
//     ip->check = 0;
//     ip->saddr = saddr;
//     ip->daddr = daddr;
//     ip->check = checksum(ip, sizeof(struct iphdr));

//     icmp->type = ICMP_ECHO;
//     icmp->code = 0;
//     icmp->checksum = 0;
//     icmp->un.echo.id = icmp_id;
//     icmp->un.echo.sequence = icmp_seq;

//     for (int i = 0; i < msglen; i++)
//         data[i] = msg[i];

//     icmp->checksum = checksum(icmp, sizeof(struct icmphdr) + msglen);

//     assert(checksum(ip, sizeof(struct iphdr)) == 0);
//     assert(checksum(icmp, sizeof(struct icmphdr) + sizeof(msg)) == 0);

//     return buffer;
// }
