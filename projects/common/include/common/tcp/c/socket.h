#ifndef NETWORKING_SOCKET_H
#define NETWORKING_SOCKET_H

#include <stdint.h>

#include <common/config.h>

NET_C_START

typedef intptr_t socket_t;
#define NET_INVALID_SOCKET ((socket_t)(-1))
#define NET_SOCKET_ERROR ((socket_t)(-1))

int tcp_net_init(void);
void tcp_net_shutdown(void);

socket_t tcp_socket_create(void);
int tcp_socket_connect(socket_t socket, const char* ip4, unsigned short port);
int tcp_socket_send(socket_t socket, const char* data, size_t size);
int tcp_socket_recv(socket_t socket, char* buffer, size_t size);
void tcp_socket_close(socket_t socket);
int tcp_socket_bind(socket_t socket, unsigned short port);
int tcp_socket_listen(socket_t socket, int backlog);
int tcp_socket_wait_for(socket_t socket, long timeout_ms);
socket_t tcp_socket_accept(socket_t socket);
int tcp_socket_set_reuse_addr(socket_t socket, int reuse);

NET_C_END

#endif // NETWORKING_SOCKET_H
