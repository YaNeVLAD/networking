#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace dns
{

enum class RecordType : std::uint16_t
{
	A = 1,
	NS = 2,
	CNAME = 5,
	SOA = 6,
	PTR = 12,
	MX = 15,
	TXT = 16,
	AAAA = 28,
	RRSIG = 46,
	ANY = 255
};

enum class ErrorCode
{
	Timeout,
	NetworkError,
	InvalidPacket,
	InvalidResponse,
	ServerFailure,
	NameNotFound,
	ResolutionLimit
};

struct Error
{
	ErrorCode code;
	std::string message;
};

struct Record
{
	std::string name;
	RecordType type{};
	std::uint16_t record_class{};
	std::uint32_t ttl{};

	std::vector<std::byte> data;
	std::string value;
};

struct Message
{
	std::uint16_t id{};
	std::uint16_t flags{};

	std::vector<Record> answers;
	std::vector<Record> authority;
	std::vector<Record> additional;

	[[nodiscard]] bool truncated() const noexcept
	{
		return (flags & 0x0200) != 0;
	}

	[[nodiscard]] bool authoritative() const noexcept
	{
		return (flags & 0x0400) != 0;
	}

	[[nodiscard]] bool recursion_available() const noexcept
	{
		return (flags & 0x0080) != 0;
	}

	[[nodiscard]] std::uint8_t response_code() const noexcept
	{
		return static_cast<std::uint8_t>(flags & 0x000F);
	}
};

using Result = std::expected<Message, Error>;
using ResolveResult = std::expected<std::vector<std::string>, Error>;

[[nodiscard]] constexpr std::uint16_t to_value(const RecordType type) noexcept
{
	return static_cast<std::uint16_t>(type);
}

} // namespace dns