#include <common/udp/c/socket.h>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <limits.h>

#pragma comment(lib, "ws2_32.lib")

int udp_net_init(void)
{
	WSADATA wsaData;
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void udp_net_shutdown(void)
{
	WSACleanup();
}

socket_t udp_socket_create(void)
{
	const SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (s == INVALID_SOCKET)
	{
		return NET_INVALID_SOCKET;
	}

	return (socket_t)s;
}

void udp_socket_close(const socket_t socket)
{
	closesocket((SOCKET)socket);
}

int udp_socket_bind(const socket_t socket, const unsigned short port)
{
	const SOCKET s = (SOCKET)socket;

	struct sockaddr_in address = { 0 };

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	if (bind(s, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		return NET_SOCKET_ERROR;
	}

	return 0;
}

int udp_socket_send_to(
	const socket_t socket,
	const char* ip4,
	const unsigned short port,
	const char* data,
	const size_t size)
{
	if (size > INT_MAX)
	{
		return NET_SOCKET_ERROR;
	}

	const SOCKET s = (SOCKET)socket;

	struct sockaddr_in address = { 0 };

	address.sin_family = AF_INET;
	address.sin_port = htons(port);

	if (inet_pton(AF_INET, ip4, &address.sin_addr) != 1)
	{
		return NET_SOCKET_ERROR;
	}

	const int sent = sendto(
		s,
		data,
		(int)size,
		0,
		(struct sockaddr*)&address,
		sizeof(address));

	if (sent == SOCKET_ERROR)
	{
		return NET_SOCKET_ERROR;
	}

	return sent;
}

int udp_socket_recv_from(
	const socket_t socket,
	char* buffer,
	const size_t size,
	char* sender_ip4,
	const size_t sender_ip4_size,
	unsigned short* sender_port)
{
	if (size > INT_MAX || sender_ip4_size < INET_ADDRSTRLEN)
	{
		return NET_SOCKET_ERROR;
	}

	const SOCKET s = (SOCKET)socket;

	struct sockaddr_in sender;
	int sender_size = sizeof(sender);

	const int received = recvfrom(
		s,
		buffer,
		(int)size,
		0,
		(struct sockaddr*)&sender,
		&sender_size);

	if (received == SOCKET_ERROR)
	{
		return NET_SOCKET_ERROR;
	}

	if (inet_ntop(
			AF_INET,
			&sender.sin_addr,
			sender_ip4,
			(DWORD)sender_ip4_size)
		== NULL)
	{
		return NET_SOCKET_ERROR;
	}

	*sender_port = ntohs(sender.sin_port);

	return received;
}

int udp_socket_wait_for(const socket_t socket, const long timeout_ms)
{
	const SOCKET s = (SOCKET)socket;

	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(s, &read_set);

	const struct timeval timeout = {
		.tv_sec = timeout_ms / 1000,
		.tv_usec = (timeout_ms % 1000) * 1000
	};

	return select(0, &read_set, NULL, NULL, &timeout);
}