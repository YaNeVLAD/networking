#pragma once

#include <span>
#include <string>

#include <common/udp/details/BaseSocket.hpp>

namespace udp
{

struct Datagram
{
	std::string data;
	std::string senderIp4;
	unsigned short senderPort{};
};

class Socket : public details::BaseSocket
{
public:
	using BaseSocket::BaseSocket;

	explicit Socket(socket_t socket);

	void bind(unsigned short port) const;

	void send_to(
		std::string_view ip4,
		unsigned short port,
		std::span<const std::byte> data) const;

	void send_to(
		std::string_view ip4,
		unsigned short port,
		std::string_view message) const;

	[[nodiscard]] bool wait_for(long timeout_ms) const;

	[[nodiscard]] Datagram receive_from(size_t maxSize) const;
};

} // namespace udp