#pragma once

#include <common/udp/c/socket.h>

namespace udp::details
{

class BaseSocket
{
public:
	BaseSocket();

	BaseSocket(const BaseSocket&) = delete;
	BaseSocket& operator=(const BaseSocket&) = delete;

	virtual ~BaseSocket();

	void close() noexcept;

	[[nodiscard]] bool valid() const noexcept;

	explicit operator bool() const noexcept;

protected:
	BaseSocket(BaseSocket&& other) noexcept;

	BaseSocket& operator=(BaseSocket&& other) noexcept;

	explicit BaseSocket(socket_t socket);

	[[nodiscard]] socket_t native() const noexcept;

private:
	socket_t m_socket{ NET_INVALID_SOCKET };
};

} // namespace udp::details