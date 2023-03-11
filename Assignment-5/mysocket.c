#include "mysocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <pthread.h>

int my_socket(int domain, int type, int protocol)
{
    if (type != SOCK_MyTCP && type != SOCK_MyTCP_NONBLOCK && type != SOCK_MyTCP_CLOEXEC && type != SOCK_MyTCP_NONBLOCK_CLOEXEC)
    {
        errno = EINVAL;
        return -1;
    }
    return socket(domain, type, protocol);
}

int my_bind(int fd, const struct sockaddr *addr, socklen_t len)
{
    return bind(fd, addr, len);
}

int my_listen(int fd, int n)
{
    return listen(fd, n);
}

int my_accept(int fd, struct sockaddr *addr, socklen_t *addr_len)
{
    return accept(fd, addr, addr_len);
}

int my_connect(int fd, const struct sockaddr *restrict addr, socklen_t len)
{
    return connect(fd, addr, len);
}

ssize_t my_send(int fd, const void *buf, size_t n, int flags)
{
    return send(fd, buf, n, flags);
}

ssize_t my_recv(int fd, void *buf, size_t n, int flags)
{
    return recv(fd, buf, n, flags);
}

int my_close(int fd)
{
    return close(fd);
}