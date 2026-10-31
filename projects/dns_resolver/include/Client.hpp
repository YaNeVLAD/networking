
#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <string_view>

#include "Codec.hpp"

namespace dns
{

class Client
{
public:
	explicit Client(std::chrono::milliseconds timeout = std::chrono::seconds(2));

	[[nodiscard]] Result query(
		std::string_view server,
		std::string_view domain,
		RecordType type) const;

private:
	[[nodiscard]] Result exchange_udp(
		std::string_view server,
		const codec::Buffer& request,
		std::uint16_t id) const;

	static [[nodiscard]] Result exchange_tcp(
		std::string_view server,
		const codec::Buffer& request,
		std::uint16_t id) ;

	std::chrono::milliseconds m_timeout;
};

} // namespace dns
