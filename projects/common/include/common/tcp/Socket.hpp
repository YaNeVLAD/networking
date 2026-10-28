#pragma once

#include <string>

#include <common/tcp/socket.h>

namespace tcp
{

class Socket
{
	struct NetInitializer
	{
		NetInitializer();
		~NetInitializer();
	};

public:
	Socket();

	explicit Socket(socket_t socket);

	Socket(const Socket& socket) = delete;
	Socket& operator=(const Socket& socket) = delete;

	Socket(Socket&& other) noexcept;

	Socket& operator=(Socket&& other) noexcept;

	~Socket();

	void connect(const std::string& ip, unsigned short port) const;

	void send(const std::string& message) const;

	void close();

	[[nodiscard]] std::string receive(size_t maxSize) const;

	[[nodiscard]] bool valid() const;

private:
	socket_t m_socket{};
};

} // namespace tcp