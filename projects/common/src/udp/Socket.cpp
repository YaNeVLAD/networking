#include <common/udp/Socket.hpp>

#include <array>
#include <stdexcept>

using namespace udp;

Socket::Socket()
{
	bind(0);
}

Socket::Socket(const socket_t socket)
	: BaseSocket(socket)
{
	bind(0);
}

void Socket::bind(const unsigned short port) const
{
	if (udp_socket_bind(native(), port) == NET_SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to bind UDP socket to port " + std::to_string(port));
	}
}

void Socket::send_to(
	const std::string_view ip4,
	const unsigned short port,
	const std::span<const std::byte> data) const
{
	const std::string address(ip4);

	const int sent = udp_socket_send_to(
		native(),
		address.c_str(),
		port,
		reinterpret_cast<const char*>(data.data()),
		data.size());

	if (sent == NET_SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to send UDP datagram to " + address + ":" + std::to_string(port));
	}

	if (static_cast<size_t>(sent) != data.size())
	{
		throw std::runtime_error("Incomplete UDP datagram send");
	}
}

void Socket::send_to(
	const std::string_view ip4,
	const unsigned short port,
	const std::string_view message) const
{
	send_to(
		ip4,
		port,
		std::span{
			reinterpret_cast<const std::byte*>(message.data()),
			message.size() });
}

bool Socket::wait_for(const long timeout_ms) const
{
	const int result = udp_socket_wait_for(native(), timeout_ms);

	if (result < 0)
	{
		throw std::runtime_error("Failed to wait for UDP datagram");
	}

	return result > 0;
}

Datagram Socket::receive_from(const size_t maxSize) const
{
	if (maxSize == 0)
	{
		throw std::invalid_argument("UDP receive buffer size must be positive");
	}

	Datagram datagram;
	datagram.data.resize(maxSize);

	std::array<char, 32> senderIp4{};

	const int received = udp_socket_recv_from(
		native(),
		datagram.data.data(),
		maxSize,
		senderIp4.data(),
		senderIp4.size(),
		&datagram.senderPort);

	if (received == NET_SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to receive UDP datagram");
	}

	datagram.data.resize(received);
	datagram.senderIp4 = senderIp4.data();

	return datagram;
}