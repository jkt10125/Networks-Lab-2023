#include "mysocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <pthread.h>
#define BUFSIZE 100

int sockfd = -1;
int newsockfd = -1;
Table *send_table, *recv_table;
pthread_t threadR, threadS;
pthread_cond_t recvEmpty, recvFull, sendEmpty, sendFull, connected;
pthread_mutex_t recvMutex, sendMutex, mutex;

void init_table(Table **table)
{
    *table = (Table *)malloc(sizeof(Table));
    (*table)->start = 0;
    (*table)->end = 0;
    (*table)->size = 0;
}

void destroy_table(Table *table)
{
    for (int i = table->start, j = 0; j < table->size; i = (i + 1) % TABLE_SIZE, j++)
    {
        free(table->buffer[i].msg);
    }
    free(table);
}

void *SthreadRunner(void *param)
{

    while (1)
    {
        // wait until a connection is established
        pthread_mutex_lock(&mutex);
        while (newsockfd == -1)
            pthread_cond_wait(&connected, &mutex);
        pthread_mutex_unlock(&mutex);

        // possible cancellation point
        pthread_testcancel();

        // wait till send table is empty
        pthread_mutex_lock(&sendMutex);
        while (send_table->size == 0)
        {
            pthread_cond_wait(&sendFull, &sendMutex);
        }
        pthread_mutex_unlock(&sendMutex);

        // possible cancellation point
        pthread_testcancel();

        pthread_mutex_lock(&sendMutex);
        int flags = send_table->buffer[send_table->start].flags;
        int msglen = send_table->buffer[send_table->start].msglen;
        char *msg = (char *)malloc(msglen);
        memcpy(msg, send_table->buffer[send_table->start].msg, msglen);
        free(send_table->buffer[send_table->start].msg);
        send_table->start = (send_table->start + 1) % TABLE_SIZE;
        send_table->size--;
        pthread_cond_signal(&sendEmpty);
        pthread_mutex_unlock(&sendMutex);

        if (send(newsockfd, &msglen, sizeof(msglen), flags) < 0)
        {
            perror("send()");
            continue;
        }

        char buffer[BUFSIZE];
        int sendlen = 0;
        while (sendlen < msglen)
        {
            memset(buffer, 0, BUFSIZE);
            int buflen = 0;
            while (buflen < BUFSIZE && sendlen < msglen)
            {
                buffer[buflen++] = msg[sendlen++];
            }
            if (send(newsockfd, buffer, buflen, flags) < 0)
            {
                perror("send()");
                break;
            }
        }
        free(msg);
    }

    return param;
}

void *RthreadRunner(void *param)
{

    while (1)
    {
        // wait until a connection is established
        pthread_mutex_lock(&mutex);
        while (newsockfd == -1)
            pthread_cond_wait(&connected, &mutex);
        pthread_mutex_unlock(&mutex);

        // possible cancellation point
        pthread_testcancel();

        int peeklen, flag = 0;
        int size;
        do
        {
            peeklen = recv(newsockfd, &size, sizeof(size), MSG_PEEK);
            if (peeklen < 0)
            {
                flag = 1;
                break;
            }
        } while (peeklen < 4);

        if (flag)
            continue;

        recv(newsockfd, &size, sizeof(size), 0);
        char *buf = (char *)malloc(size);
        int recvlen = 0;
        while (recvlen != size)
        {
            int n = recv(newsockfd, buf + recvlen, size - recvlen, 0);
            if (n <= 0)
            {
                flag = 1;
                break;
            }
            recvlen += n;
        }
        if (flag)
            continue;

        // wait till recv table is full
        pthread_mutex_lock(&recvMutex);
        while (recv_table->size == TABLE_SIZE)
        {
            pthread_cond_wait(&recvEmpty, &recvMutex);
        }
        pthread_mutex_unlock(&recvMutex);

        // possible cancellation point
        pthread_testcancel();

        pthread_mutex_lock(&recvMutex);
        recv_table->buffer[recv_table->end].msg = buf;
        recv_table->buffer[recv_table->end].msglen = size;
        recv_table->buffer[recv_table->end].flags = 0;
        recv_table->end = (recv_table->end + 1) % TABLE_SIZE;
        recv_table->size++;
        pthread_cond_signal(&recvFull);
        pthread_mutex_unlock(&recvMutex);
    }
    return param;
}

int my_socket(int domain, int type, int protocol)
{
    if (sockfd != -1)
    {
        errno = ENOBUFS;
        return -1;
    }

    if (type != SOCK_MyTCP && type != SOCK_MyTCP_NONBLOCK && type != SOCK_MyTCP_CLOEXEC && type != SOCK_MyTCP_NONBLOCK_CLOEXEC)
    {
        errno = EINVAL;
        return -1;
    }

    sockfd = socket(domain, SOCK_STREAM, protocol);

    if (sockfd > 0)
    {
        init_table(&send_table);
        init_table(&recv_table);
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_init(&sendMutex, NULL);
        pthread_mutex_init(&recvMutex, NULL);
        pthread_cond_init(&connected, NULL);
        pthread_cond_init(&recvEmpty, NULL);
        pthread_cond_init(&recvFull, NULL);
        pthread_cond_init(&sendEmpty, NULL);
        pthread_cond_init(&sendFull, NULL);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&threadS, &attr, SthreadRunner, NULL);
        pthread_create(&threadR, &attr, RthreadRunner, NULL);
    }

    return sockfd;
}

int my_bind(int fd, const struct sockaddr *addr, socklen_t len)
{
    if (sockfd == -1 || fd != sockfd)
    {
        errno = ENOTSOCK;
        return -1;
    }
    return bind(fd, addr, len);
}

int my_listen(int fd, int n)
{
    if (sockfd == -1 || fd != sockfd)
    {
        errno = ENOTSOCK;
        return -1;
    }
    return listen(fd, n);
}

int my_accept(int fd, struct sockaddr *addr, socklen_t *addr_len)
{
    if (sockfd == -1 || fd != sockfd)
    {
        errno = ENOTSOCK;
        return -1;
    }
    int retfd = accept(fd, addr, addr_len);
    if (retfd < 0)
        return retfd;
    pthread_mutex_lock(&mutex);
    newsockfd = retfd;
    pthread_cond_broadcast(&connected);
    pthread_mutex_unlock(&mutex);
    return retfd;
}

int my_connect(int fd, const struct sockaddr *restrict addr, socklen_t len)
{
    if (sockfd == -1 || fd != sockfd)
    {
        errno = ENOTSOCK;
        return -1;
    }
    int retVal = connect(fd, addr, len);
    if (retVal < 0)
        return retVal;
    pthread_mutex_lock(&mutex);
    newsockfd = fd;
    pthread_cond_broadcast(&connected);
    pthread_mutex_unlock(&mutex);
    return retVal;
}

ssize_t my_send(int fd, const void *buf, size_t n, int flags)
{
    if (newsockfd == -1 || fd != newsockfd)
    {
        errno = EBADF;
        return -1;
    }

    pthread_mutex_lock(&sendMutex);
    while (send_table->size == TABLE_SIZE)
    {
        pthread_cond_wait(&sendEmpty, &sendMutex);
    }
    send_table->buffer[send_table->end].msg = (char *)malloc(n);
    memcpy(send_table->buffer[send_table->end].msg, buf, n);
    send_table->buffer[send_table->end].msglen = n;
    send_table->buffer[send_table->end].flags = flags;
    send_table->end = (send_table->end + 1) % TABLE_SIZE;
    send_table->size++;
    pthread_cond_signal(&sendFull);
    pthread_mutex_unlock(&sendMutex);

    return n;
}

ssize_t my_recv(int fd, void *buf, size_t n, int flags)
{
    if (newsockfd == -1 || fd != newsockfd)
    {
        errno = EBADF;
        return -1;
    }

    pthread_mutex_lock(&recvMutex);
    while (recv_table->size == 0)
    {
        pthread_cond_wait(&recvFull, &recvMutex);
    }
    int len = (n > recv_table->buffer[recv_table->start].msglen) ? recv_table->buffer[recv_table->start].msglen : n;
    memcpy(buf, recv_table->buffer[recv_table->start].msg, len);
    free(recv_table->buffer[recv_table->start].msg);
    recv_table->start = (recv_table->start + 1) % TABLE_SIZE;
    recv_table->size--;
    pthread_cond_signal(&recvEmpty);
    pthread_mutex_unlock(&recvMutex);

    return len;
}

int my_close(int fd)
{
    usleep(1000);
    if (sockfd != -1 && fd == sockfd)
    {
        sockfd = -1;
        pthread_cancel(threadS);
        pthread_cancel(threadR);
        pthread_cond_signal(&sendFull);
        pthread_cond_signal(&recvEmpty);
        pthread_cond_broadcast(&connected);
        pthread_join(threadS, NULL);
        pthread_join(threadR, NULL);
        destroy_table(send_table);
        destroy_table(recv_table);
    }
    else if (newsockfd != -1 && fd == newsockfd)
    {
        pthread_mutex_lock(&mutex);
        newsockfd = -1;
        pthread_mutex_unlock(&mutex);

        pthread_mutex_lock(&sendMutex);
        destroy_table(send_table);
        init_table(&send_table);
        pthread_mutex_unlock(&sendMutex);

        pthread_mutex_lock(&recvMutex);
        destroy_table(recv_table);
        init_table(&recv_table);
        pthread_mutex_unlock(&recvMutex);
    }
    return close(fd);
}
