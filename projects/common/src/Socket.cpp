#include <common/tcp/Socket.hpp>

#include <stdexcept>

using namespace tcp;

Socket::NetInitializer::NetInitializer() { tcp_net_init(); }

Socket::NetInitializer::~NetInitializer() { tcp_net_shutdown(); }

Socket::Socket()
	: m_socket(NET_INVALID_SOCKET)
{
	static NetInitializer initializer;
	if (m_socket = tcp_socket_create(); m_socket == NET_INVALID_SOCKET)
	{
		throw std::runtime_error("Failed to create TCP socket");
	}
}

Socket::Socket(const socket_t socket)
	: m_socket(socket)
{
}

Socket::Socket(Socket&& other) noexcept
	: m_socket(other.m_socket)
{
	other.m_socket = NET_INVALID_SOCKET;
}

Socket& Socket::operator=(Socket&& other) noexcept
{
	if (this != &other)
	{
		close();
		m_socket = other.m_socket;
		other.m_socket = NET_INVALID_SOCKET;
	}

	return *this;
}

Socket::~Socket()
{
	close();
}

void Socket::connect(const std::string& ip, const unsigned short port) const
{
	if (tcp_socket_connect(m_socket, ip.c_str(), port) == NET_INVALID_SOCKET)
	{
		throw std::runtime_error("Failed to connect to " + ip + ":" + std::to_string(port));
	}
}

void Socket::send(const std::string& message) const
{
	if (const auto sent = tcp_socket_send(m_socket, message.c_str(), message.length()); sent == NET_SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to send " + message + " | Error: " + std::to_string(sent));
	}
}

void Socket::close()
{
	if (m_socket != NET_INVALID_SOCKET)
	{
		tcp_socket_close(m_socket);
		m_socket = NET_INVALID_SOCKET;
	}
}

std::string Socket::receive(const size_t maxSize) const
{
	std::string buffer(maxSize, '\0');
	const int received = tcp_socket_recv(m_socket, &buffer[0], maxSize);
	if (received == NET_SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to receive " + buffer + " | Error: " + std::to_string(received));
	}
	buffer.resize(received);

	return buffer;
}

bool Socket::valid() const
{
	return m_socket != NET_INVALID_SOCKET;
}