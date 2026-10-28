#pragma once

#include <span>
#include <string>
#include <string_view>

#include <common/tcp/details/BaseSocket.hpp>

namespace tcp
{

class Socket : public details::BaseSocket
{
public:
	using BaseSocket::BaseSocket;

	explicit Socket(socket_t socket);

	void connect(std::string_view ip4, unsigned short port) const;

	void send(std::span<const std::byte> data) const;
	void send(std::string_view message) const;

	[[nodiscard]] std::string receive(size_t maxSize) const;
};

} // namespace tcp