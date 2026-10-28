#include <common/tcp/Listener.hpp>

#include <stdexcept>

using namespace tcp;

void Listener::bind(const unsigned short port) const
{
	if (tcp_socket_bind(native(), port) == NET_SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to bind socket to port " + std::to_string(port));
	}
}

void Listener::listen(const int backlog) const
{
	if (tcp_socket_listen(native(), backlog) == NET_SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to listen on socket");
	}
}

void Listener::set_reuse_address(const bool reuse) const
{
	if (tcp_socket_set_reuse_addr(native(), reuse ? 1 : 0) == NET_SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to set socket reuse address option");
	}
}

Socket Listener::accept() const
{
	return Socket{ tcp_socket_accept(native()) };
}