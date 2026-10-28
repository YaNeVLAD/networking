#pragma once

#include <common/tcp/Socket.hpp>
#include <common/tcp/details/BaseSocket.hpp>

namespace tcp
{

class Listener : public details::BaseSocket
{
public:
	using BaseSocket::BaseSocket;

	void bind(unsigned short port) const;
	void listen(int backlog = 32) const;
	void set_reuse_address(bool reuse) const;

	[[nodiscard]] Socket accept() const;
};

} // namespace tcp