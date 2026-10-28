#include <common/tcp/socket.h>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int tcp_net_init(void)
{
	WSADATA wsaData;
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void tcp_net_shutdown(void)
{
	WSACleanup();
}

socket_t tcp_socket_create(void)
{
	const SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		return NET_INVALID_SOCKET;
	}

	return (socket_t)s;
}

int tcp_socket_connect(const socket_t socket, const char* ip4, const unsigned short port)
{
	const SOCKET s = (SOCKET)socket;

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, ip4, &server_addr.sin_addr) <= 0)
	{
		return -1;
	}

	if (connect(s, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
	{
		return -1;
	}

	return 0;
}

int tcp_socket_send(const socket_t socket, const char* data, const size_t size)
{
	const int result =  send((SOCKET)socket, data, (int)size, 0);
	if (result == SOCKET_ERROR)
	{
		return NET_SOCKET_ERROR;
	}

	return result;
}

int tcp_socket_recv(const socket_t socket, char* buffer, const size_t size)
{
	return recv((SOCKET)socket, buffer, (int)size, 0);
}

void tcp_socket_close(const socket_t socket)
{
	closesocket((SOCKET)socket);
}