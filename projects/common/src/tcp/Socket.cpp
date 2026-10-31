#include <common/tcp/Socket.hpp>

#include <stdexcept>

#include <../include/common/tcp/c/socket.h>

using namespace tcp;

Socket::Socket(const socket_t socket)
	: BaseSocket(socket)
{
}

void Socket::connect(const std::string_view ip4, const unsigned short port) const
{
	if (tcp_socket_connect(native(), std::string(ip4).c_str(), port) == NET_INVALID_SOCKET)
	{
		throw std::runtime_error("Failed to connect to " + std::string(ip4) + ":" + std::to_string(port));
	}
}

void Socket::send(const std::span<const std::byte> data) const
{
	std::size_t offset = 0;

	while (offset < data.size())
	{
		const auto remaining = data.subspan(offset);

		const int sent = tcp_socket_send(
			native(),
			reinterpret_cast<const char*>(remaining.data()),
			remaining.size());

		if (sent <= 0)
		{
			throw std::runtime_error("Failed to send fu TCP message");
		}

		offset += static_cast<std::size_t>(sent);
	}
}

void Socket::send(const std::string_view message) const
{
	send(std::span{ reinterpret_cast<const std::byte*>(message.data()), message.size() });
}

std::string Socket::receive(const size_t maxSize) const
{
	std::string buffer(maxSize, '\0');
	const int received = tcp_socket_recv(native(), &buffer[0], maxSize);
	if (received == NET_SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to receive " + buffer + " | Error: " + std::to_string(received));
	}
	buffer.resize(received);

	return buffer;
}