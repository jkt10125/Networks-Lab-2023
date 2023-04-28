/**
 * @file pingnetinfo.c
 * @author Abhay Kumar Keshari (20CS10001)
 * @author Jay Kumar Thakur (20CS30024)
 *
 * @brief Assignment-6
 * @version 0.1
 * @date 2023-04-09
 *
 * @copyright Copyright (c) 2023
 *
 */

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
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <math.h>
#define MAX_SIZE 1500
#define DWORD 4
#define IPv4 4
#define PORT 20000

uint16_t checksum(const void *buff, size_t nbytes);
char *dnsLookup(const char *h_name, struct sockaddr_in *addr);
char *niLookup(int ni_family, struct sockaddr_in *addr);
struct sockaddr_in getIntermediateNode(int nhops);
double getBandwidth(int payload, double rtt1, double rtt2);
double getRTT(struct sockaddr_in imaddr, uint16_t ttl, size_t payload);
void printIP(const struct iphdr *ip);
void printTCP(const struct tcphdr *tcp);
void printUDP(const struct udphdr *udp);
void printICMPdata(const char *data);
void printICMP(const struct icmphdr *icmp);

uint16_t ip_id = 0;
int sockfd, n, T;
const int ON = 1;
char *src_ip, *dest_ip;
struct sockaddr_in srcaddr, destaddr;
FILE *logfile;
int main(int argc, char *argv[])
{
    /**
     * @brief Printing a lot of headers sent and recieved messes up the terminal
     *        making it difficult to read Latency and Bandwidth. Thus "logfile.log" is used
     *        to print all those information
     */
    logfile = fopen("logfile.log", "w");
    if (logfile == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
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

    printf("\nFor tracking packets sent and received, see \"logfile.log\"\n\n");

    if ((src_ip = niLookup(AF_INET, &srcaddr)) == NULL)
    {
        exit(EXIT_FAILURE);
    }
    if ((dest_ip = dnsLookup(argv[1], &destaddr)) == NULL)
    {
        exit(EXIT_FAILURE);
    }
    printf("Source: %s\n", src_ip);
    printf("Destination: %s\n", dest_ip);

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
    int ttl = 0;
    double prevrtt_avg = 0, prevrtt_avg2 = 0;
    printf("Hops \t %-20s \t %-20s \t %-20s \n", "IP", "Latency(in us)", "Bandwidth (in Mbps)");

    while (++ttl)
    {
        struct sockaddr_in imaddr = getIntermediateNode(ttl);

        if (imaddr.sin_addr.s_addr == 0)
        {
            printf("%d \t *******\n", ttl);
            continue;
        }

        size_t payload = 0;
        double rtt_avg = getRTT(imaddr, ttl, payload);

        payload = 1000;
        double rtt_avg2 = getRTT(imaddr, ttl, payload);
        double latency = abs(prevrtt_avg - rtt_avg);
        printf("%-2d \t %-20s \t %-20.3f \t %-20.3f \n", ttl, inet_ntoa(imaddr.sin_addr), latency, getBandwidth(payload, abs(prevrtt_avg - rtt_avg), abs(prevrtt_avg2 - rtt_avg2)));

        prevrtt_avg = rtt_avg;
        prevrtt_avg2 = rtt_avg2;
        if (imaddr.sin_addr.s_addr == destaddr.sin_addr.s_addr)
            break;
    }
    fclose(logfile);
    return 0;
}

double getRTT(struct sockaddr_in imaddr, uint16_t ttl, size_t payload)
{
    uint16_t buflen = sizeof(struct iphdr) + sizeof(struct icmphdr) + payload;
    char *buffer = (char *)malloc(buflen * sizeof(char));
    struct iphdr *ip = (struct iphdr *)buffer;
    struct icmphdr *icmp = (struct icmphdr *)(buffer + sizeof(struct iphdr));
    char *data = buffer + sizeof(struct iphdr) + sizeof(struct icmphdr);
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char *recv_buffer = (char *)malloc(MAX_SIZE * sizeof(char));
    ssize_t recvlen;
    struct timeval send_tv, recv_tv;
    struct iphdr *ip_reply;
    struct icmphdr *icmp_reply;

    int cnt = 0;
    uint16_t icmpID = ttl, seq = rand() % 0xFFFF;
    double rtt_avg = 0;
    for (int i = 0; i < n; i++)
    {
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

        if (sendto(sockfd, buffer, buflen, 0, (const struct sockaddr *)&imaddr, sizeof(imaddr)) < 0)
        {
            perror("sendto()");
            exit(EXIT_FAILURE);
        }
        fprintf(logfile, "-----------------------------------------------------------------\n");
        fprintf(logfile, "|                       Sent %4d bytes                         |\n", buflen);
        fprintf(logfile, "-----------------------------------------------------------------\n");
        printIP(ip);
        printICMP(icmp);
        fprintf(logfile, "\n\n");

        if (gettimeofday(&send_tv, NULL) < 0)
        {
            perror("gettimeofday");
            exit(EXIT_FAILURE);
        }

        int timeout = 1000 * T;
        struct pollfd fdset;
        struct timeval now, then;
        gettimeofday(&now, NULL);

        while (1)
        {
            fdset.fd = sockfd;
            fdset.events = POLLIN;

            int retval = poll(&fdset, 1, timeout);
            gettimeofday(&then, NULL);

            if (retval < 0)
            {
                perror("poll");
                exit(EXIT_FAILURE);
            }
            else if (retval == 0)
            {
                break;
            }
            if ((recvlen = recvfrom(sockfd, recv_buffer, MAX_SIZE, 0, (struct sockaddr *)&addr, &addrlen)) < 0)
            {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }
            fprintf(logfile, "-----------------------------------------------------------------\n");
            fprintf(logfile, "|                    Received %6ld bytes                      |\n", recvlen);
            fprintf(logfile, "-----------------------------------------------------------------\n");

            if (gettimeofday(&recv_tv, NULL) < 0)
            {
                perror("gettimeofday");
                exit(EXIT_FAILURE);
            }
            int rtt_m = 1000000 * (recv_tv.tv_sec - send_tv.tv_sec) + (recv_tv.tv_usec - send_tv.tv_usec);

            ip_reply = (struct iphdr *)recv_buffer;
            printIP(ip_reply);

            if (ip_reply->protocol == IPPROTO_ICMP)
            {
                icmp_reply = (struct icmphdr *)(recv_buffer + DWORD * ip_reply->ihl);
                printICMP(icmp_reply);

                if (icmp_reply->type == ICMP_ECHOREPLY)
                {
                    if (icmp->un.echo.id == icmp_reply->un.echo.id && icmp->un.echo.sequence == icmp_reply->un.echo.sequence)
                    {
                        rtt_avg = (rtt_avg * cnt + rtt_m) / (double)(cnt + 1);
                        cnt++;
                    }
                }
            }
            fprintf(logfile, "\n\n");
            timeout = 1000 * T - (1000 * (then.tv_sec - now.tv_sec) + 0.001 * (then.tv_usec - now.tv_usec));
        }
    }
    free(buffer);
    free(recv_buffer);
    return rtt_avg;
}

/**
 * @brief return the bandwidth in Mbps
 *
 * @param payload number of bytes of data
 * @param rtt1 round-trip time without payload
 * @param rtt2 round-trip time with payload
 */
double getBandwidth(int payload, double rtt1, double rtt2)
{
    double bw = 8 * payload / abs(rtt1 - rtt2);
    return bw;
}
/**
 * @brief returns the intermediate node address from source (and before destination) at nhops distance from source
 *
 * @param nhops distance of intermediate node from source
 */
struct sockaddr_in getIntermediateNode(int nhops)
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
    uint16_t icmpID = nhops;
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
        ip->ttl = nhops;
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

        if (sendto(sockfd, buffer, buflen, 0, (const struct sockaddr *)&destaddr, sizeof(destaddr)) < 0)
        {
            perror("sendto()");
            exit(EXIT_FAILURE);
        }
        fprintf(logfile, "-----------------------------------------------------------------\n");
        fprintf(logfile, "|                       Sent %4d bytes                         |\n", buflen);
        fprintf(logfile, "-----------------------------------------------------------------\n");
        printIP(ip);
        printICMP(icmp);
        fprintf(logfile, "\n\n");

        int timeout = 1000;
        struct pollfd fdset;
        struct timeval now, then;
        gettimeofday(&now, NULL);

        while (1)
        {
            fdset.fd = sockfd;
            fdset.events = POLLIN;

            int retval = poll(&fdset, 1, timeout);
            gettimeofday(&then, NULL);

            if (retval < 0)
            {
                perror("poll");
                exit(EXIT_FAILURE);
            }
            else if (retval == 0)
            {
                flag = 1;
                break;
            }
            if ((recvlen = recvfrom(sockfd, recv_buffer, MAX_SIZE, 0, (struct sockaddr *)&addr, &addrlen)) < 0)
            {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }

            fprintf(logfile, "-----------------------------------------------------------------\n");
            fprintf(logfile, "|                    Received %6ld bytes                      |\n", recvlen);
            fprintf(logfile, "-----------------------------------------------------------------\n");

            ip_reply = (struct iphdr *)recv_buffer;
            printIP(ip_reply);
            if (ip_reply->protocol == IPPROTO_ICMP)
            {
                icmp_reply = (struct icmphdr *)(recv_buffer + DWORD * ip_reply->ihl);
                printICMP(icmp_reply);
                if (icmp_reply->type == ICMP_ECHOREPLY)
                {
                    if (icmp->un.echo.id == icmp_reply->un.echo.id && icmp->un.echo.sequence == icmp_reply->un.echo.sequence)
                    {
                        fprintf(logfile, "\n\n");
                        break;
                    }
                }
                else if (icmp_reply->type == ICMP_TIME_EXCEEDED)
                {
                    struct iphdr *ip_timeout = (struct iphdr *)(recv_buffer + sizeof(struct iphdr) + sizeof(struct icmphdr));
                    if (ip->id == ip_timeout->id)
                    {
                        fprintf(logfile, "\n\n");

                        break;
                    }
                }
            }
            fprintf(logfile, "\n\n");
            timeout = 1000 - (1000 * (then.tv_sec - now.tv_sec) + 0.001 * (then.tv_usec - now.tv_usec));
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
        addr->sin_family = ni_family;
        addr->sin_port = htons(PORT);
        addr->sin_addr.s_addr = INADDR_ANY;
    }
    return strdup(inet_ntoa(((struct sockaddr_in *)it->ifa_addr)->sin_addr));
}

void printIP(const struct iphdr *ip)
{
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|   version:%-2d  |   hlen:%-4d   |     tos:%-2d    |  totlen:%-4d  |\n", ip->version, ip->ihl, ip->tos, ntohs(ip->tot_len));
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|           id:%-6d           |%d|%d|%d|      frag_off:%-4d      |\n", ntohs(ip->id), ip->frag_off && (1 << 15), ip->frag_off && (1 << 14), ip->frag_off && (1 << 14), ip->frag_off);
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|    ttl:%-4d   |  protocol:%-2d  |         checksum:%-6d       |\n", ip->ttl, ip->protocol, ip->check);
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|                    source:%-16s                    |\n", inet_ntoa(*(struct in_addr *)&ip->saddr));
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|                 destination:%-16s                  |\n", inet_ntoa(*(struct in_addr *)&ip->daddr));
    fprintf(logfile, "-----------------------------------------------------------------\n");
}
void printTCP(const struct tcphdr *tcp)
{
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|         source:%-6d         |          dest:%-6d          |\n", ntohs(tcp->source), ntohs(tcp->dest));
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|                       sequence:%-8d                       |\n", tcp->seq);
    fprintf(logfile, "|                         ack:%-8d                          |\n", tcp->ack_seq);
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "| hlen:%-4d|reserved|%d|%d|%d|%d|%d|%d|          rwnd:%-6d          |\n", tcp->doff, tcp->urg, tcp->ack, tcp->psh, tcp->rst, tcp->syn, tcp->fin, tcp->window);
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|          check:%-6d         |         urgptr:%-6d         |\n", tcp->check, tcp->urg_ptr);
    fprintf(logfile, "-----------------------------------------------------------------\n");
}

void printUDP(const struct udphdr *udp)
{
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|         source:%-6d         |          dest:%-6d          |\n", ntohs(udp->source), ntohs(udp->dest));
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|           len:%-6d          |          check:%-6d         |\n", ntohs(udp->len), udp->check);
    fprintf(logfile, "-----------------------------------------------------------------\n");
}
void printICMPdata(const char *data)
{
    struct iphdr *ip = (struct iphdr *)(data);
    printIP(ip);
    if (ip->protocol == IPPROTO_ICMP)
    {
        struct icmphdr *icmp = (struct icmphdr *)(data + sizeof(struct iphdr));
        fprintf(logfile, "-----------------------------------------------------------------\n");
        fprintf(logfile, "|    type:%-2d    |    code:%-2d    |        checksum:%-6d        |\n", icmp->type, icmp->code, icmp->checksum);
        fprintf(logfile, "-----------------------------------------------------------------\n");
    }
    if (ip->protocol == IPPROTO_TCP)
    {
        struct tcphdr *tcp = (struct tcphdr *)(data + sizeof(struct iphdr));
        printTCP(tcp);
    }
    else if (ip->protocol == IPPROTO_UDP)
    {
        struct udphdr *udp = (struct udphdr *)(data + sizeof(struct iphdr));
        printUDP(udp);
    }
    else
    {
        fprintf(logfile, "-----------------------------------------------------------------\n");
        fprintf(logfile, "|                       Unknown Protocol                        |\n");
        fprintf(logfile, "-----------------------------------------------------------------\n");
    }
}
void printICMP(const struct icmphdr *icmp)
{
    fprintf(logfile, "-----------------------------------------------------------------\n");
    fprintf(logfile, "|    type:%-2d    |    code:%-2d    |        checksum:%-6d        |\n", icmp->type, icmp->code, icmp->checksum);
    fprintf(logfile, "-----------------------------------------------------------------\n");
    if (icmp->type == ICMP_ECHO || icmp->type == ICMP_ECHOREPLY)
    {
        fprintf(logfile, "|           id:%-6d           |        sequence:%-6d        |\n", icmp->un.echo.id, icmp->un.echo.sequence);
        fprintf(logfile, "-----------------------------------------------------------------\n");
    }
    else if (icmp->type == ICMP_TIME_EXCEEDED)
    {
    }
    else if (icmp->type == ICMP_SOURCE_QUENCH || icmp->type == ICMP_DEST_UNREACH || icmp->type == ICMP_REDIRECT || icmp->type == ICMP_PARAMETERPROB)
    {
        char *data = (char *)((char *)icmp + sizeof(struct icmphdr));
        printICMPdata(data);
    }
    else if (icmp->type == ICMP_TIMESTAMP)
    {
        fprintf(logfile, "|           id:%-6d           |        sequence:%-6d        |\n", icmp->un.echo.id, icmp->un.echo.sequence);
        fprintf(logfile, "-----------------------------------------------------------------\n");
        uint32_t *ts = (uint32_t *)((char *)icmp + sizeof(struct icmphdr));
        fprintf(logfile, "|                       Originate:%-6d                        |\n", *ts);
        fprintf(logfile, "-----------------------------------------------------------------\n");
    }
    else if (icmp->type == ICMP_TIMESTAMPREPLY)
    {
        fprintf(logfile, "|           id:%-6d           |        sequence:%-6d        |\n", icmp->un.echo.id, icmp->un.echo.sequence);
        fprintf(logfile, "-----------------------------------------------------------------\n");
        uint32_t *ts = (uint32_t *)((char *)icmp + sizeof(struct icmphdr));
        fprintf(logfile, "|                       Originate:%-6d                        |\n", *ts);
        fprintf(logfile, "-----------------------------------------------------------------\n");
        fprintf(logfile, "|                        Receive:%-6d                         |\n", *ts);
        fprintf(logfile, "-----------------------------------------------------------------\n");
        fprintf(logfile, "|                        Transmit:%-6d                        |\n", *ts);
        fprintf(logfile, "-----------------------------------------------------------------\n");
    }
    else if (icmp->type == ICMP_ADDRESS || icmp->type == ICMP_ADDRESSREPLY)
    {
        fprintf(logfile, "|           id:%-6d           |        sequence:%-6d        |\n", icmp->un.echo.id, icmp->un.echo.sequence);
        fprintf(logfile, "-----------------------------------------------------------------\n");
        uint32_t *ts = (uint32_t *)((char *)icmp + sizeof(struct icmphdr));
        fprintf(logfile, "|                     Address mask:%-6d                       |\n", *ts);
        fprintf(logfile, "-----------------------------------------------------------------\n");
    }
}
