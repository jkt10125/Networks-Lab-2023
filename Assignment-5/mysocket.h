#ifndef _MYSOCKET_H
#define _MYSOCKET_H
#define SOCK_MyTCP 42
#define SOCK_MyTCP_NONBLOCK (SOCK_MyTCP | SOCK_NONBLOCK)
#define SOCK_MyTCP_CLOEXEC (SOCK_MyTCP | SOCK_CLOEXEC)
#define SOCK_MyTCP_NONBLOCK_CLOEXEC (SOCK_MyTCP | SOCK_NONBLOCK | SOCK_CLOEXEC)
#define TABLE_SIZE 10

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct _entry
{
    char *msg;
    size_t msglen;
    int flags;
} Entry;

typedef struct _table
{
    struct _entry buffer[TABLE_SIZE];
    size_t start;
    size_t end;
    size_t size;
} Table;

/**
 * @brief   Create a new socket of type TYPE SOCK_MyTCP in domain DOMAIN, using
 *          protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
 *          Returns a file descriptor for the new socket, or -1 for errors.
 */
int my_socket(int domain, int type, int protocol);

/**
 * @brief   Give the socket FD the local address ADDR (which is LEN bytes long).
 */
int my_bind(int fd, const struct sockaddr *addr, socklen_t len);

/**
 * @brief   Prepare to accept connections on socket FD.
 *          N connection requests will be queued before further requests are refused.
 *          Returns 0 on success, -1 for errors.
 */
int my_listen(int fd, int n);

/**
 * @brief   Await a connection on socket FD.
 *          When a connection arrives, open a new socket to communicate with it,
 *          set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
 *          peer and *ADDR_LEN to the address's actual length, and return the
 *          new socket's descriptor, or -1 for errors.
 */
int my_accept(int fd, struct sockaddr *addr, socklen_t *addr_len);

/**
 * @brief   Open a connection on socket FD to peer at ADDR (which LEN bytes long).
 *          For connectionless socket types, just set the default address to send to
 *          and the only address from which to accept transmissions.
 *          Return 0 on success, -1 for errors.
 */
int my_connect(int fd, const struct sockaddr *addr, socklen_t len);

/**
 * @brief   Send N bytes as a message of BUF to socket FD.
 *          Returns the number sent or -1.
 */
ssize_t my_send(int fd, const void *buf, size_t n, int flags);

/**
 * @brief   Read a message into BUF from socket FD.
 *          Returns the number read or -1 for errors.
 */
ssize_t my_recv(int fd, void *buf, size_t n, int flags);

/**
 * @brief Close the file descriptor FD.
 */
int my_close(int fd);

#endif