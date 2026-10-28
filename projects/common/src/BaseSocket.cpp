#include <common/tcp/details/BaseSocket.hpp>

#include <stdexcept>

using namespace tcp::details;

namespace
{

struct NetInitializer
{
	NetInitializer() { tcp_net_init(); }
	~NetInitializer() { tcp_net_shutdown(); }
};

NetInitializer s_initializer;

} // namespace

BaseSocket::BaseSocket()
	: BaseSocket(tcp_socket_create())
{
}

BaseSocket::~BaseSocket()
{
	close();
}

void BaseSocket::close() noexcept
{
	if (m_socket != NET_INVALID_SOCKET)
	{
		tcp_socket_close(m_socket);
		m_socket = NET_INVALID_SOCKET;
	}
}

bool BaseSocket::valid() const noexcept
{
	return m_socket != NET_INVALID_SOCKET;
}

BaseSocket::operator bool() const noexcept
{
	return valid();
}

BaseSocket::BaseSocket(const socket_t socket)
	: m_socket(socket)
{
	if (m_socket == NET_INVALID_SOCKET)
	{
		throw std::runtime_error("Failed to create TCP socket from invalid socket_t");
	}
}

BaseSocket::BaseSocket(BaseSocket&& other) noexcept
	: m_socket(std::exchange(other.m_socket, NET_INVALID_SOCKET))
{
}

BaseSocket& BaseSocket::operator=(BaseSocket&& other) noexcept
{
	if (this != &other)
	{
		close();
		m_socket = other.m_socket;
		other.m_socket = NET_INVALID_SOCKET;
	}

	return *this;
}

socket_t BaseSocket::native() const noexcept
{
	return m_socket;
}