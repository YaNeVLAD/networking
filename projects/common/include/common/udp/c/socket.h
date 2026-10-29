#pragma once

#include <stddef.h>

#include <common/net/c/socket.h>
#include <common/net/config.h>

NET_C_START

int udp_net_init(void);
void udp_net_shutdown(void);

socket_t udp_socket_create(void);
void udp_socket_close(socket_t socket);

int udp_socket_bind(socket_t socket, unsigned short port);

int udp_socket_send_to(
	socket_t socket,
	const char* ip4,
	unsigned short port,
	const char* data,
	size_t size);

int udp_socket_recv_from(
	socket_t socket,
	char* buffer,
	size_t size,
	char* sender_ip4,
	size_t sender_ip4_size,
	unsigned short* sender_port);

int udp_socket_wait_for(socket_t socket, long timeout_ms);

NET_C_END