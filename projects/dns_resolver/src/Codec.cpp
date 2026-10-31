#include "../include/Codec.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <ranges>
#include <unordered_set>

#include <common/net/Utils.hpp>

using namespace dns;
using namespace dns::codec;

namespace
{

template <std::unsigned_integral T>
std::expected<T, Error> net_read(
	const Data packet,
	const std::size_t offset)
{
	if (offset > packet.size() || sizeof(T) > packet.size() - offset)
	{
		return std::unexpected(Error{
			.code = ErrorCode::InvalidPacket,
			.message = "Unexpected end of DNS packet",
		});
	}

	T value{};
	std::memcpy(&value, packet.data() + offset, sizeof(T));

	return net::conv::to_host(value);
}

template <std::unsigned_integral T>
void net_write(Buffer& buffer, const T value)
{
	const T net_value = net::conv::to_network(value);

	const auto* bytes = reinterpret_cast<const std::byte*>(&net_value);

	buffer.insert(
		buffer.end(),
		bytes,
		bytes + sizeof(T));
}

std::unexpected<Error> invalid_packet(std::string message)
{
	return std::unexpected(
		Error{
			.code = ErrorCode::InvalidPacket,
			.message = std::move(message),
		});
}

std::string ip4_to_string(const Data data)
{
	return std::format(
		"{}.{}.{}.{}",
		std::to_integer<unsigned>(data[0]),
		std::to_integer<unsigned>(data[1]),
		std::to_integer<unsigned>(data[2]),
		std::to_integer<unsigned>(data[3]));
}

std::string ip6_to_string(const Data data)
{
	std::array<std::uint16_t, 8> groups{};

	for (std::size_t i = 0; i < groups.size(); ++i)
	{
		groups[i] = (std::to_integer<std::uint16_t>(data[i * 2]) << 8) | std::to_integer<std::uint16_t>(data[i * 2 + 1]);
	}

	std::size_t bestStart = groups.size();
	std::size_t bestLength = 1;

	for (std::size_t i = 0; i < groups.size();)
	{
		if (groups[i] != 0)
		{
			++i;
			continue;
		}

		const auto start = i;

		while (i < groups.size() && groups[i] == 0)
		{
			++i;
		}

		if (i - start > bestLength)
		{
			bestStart = start;
			bestLength = i - start;
		}
	}

	std::string result;

	for (std::size_t i = 0; i < groups.size();)
	{
		if (i == bestStart)
		{
			result += "::";
			i += bestLength;
			continue;
		}

		if (!result.empty() && result.back() != ':')
		{
			result += ':';
		}

		result += std::format("{:x}", groups[i]);
		++i;
	}

	return result.empty() ? "::" : result;
}

std::expected<std::string, Error> decode_record_value(
	const Data packet,
	const std::size_t start,
	const std::size_t length,
	const RecordType type)
{
	const Data data = packet.subspan(start, length);

	switch (type)
	{
	case RecordType::A: {
		if (length != 4)
		{
			return invalid_packet("Invalid A record");
		}

		return ip4_to_string(data);
	}

	case RecordType::AAAA: {
		if (length != 16)
		{
			return invalid_packet("Invalid AAAA record");
		}

		return ip6_to_string(data);
	}

	case RecordType::NS:
	case RecordType::CNAME:
	case RecordType::PTR: {
		std::size_t offset = start;

		auto name = decode_name(packet, offset);

		if (!name)
			return std::unexpected(name.error());

		if (offset != start + length)
		{
			return invalid_packet("Invalid domain name RDATA length");
		}

		return *name;
	}

	case RecordType::MX: {
		if (length < 3)
		{
			return invalid_packet("Invalid MX record");
		}

		auto preference = net_read<std::uint16_t>(packet, start);

		if (!preference)
		{
			return std::unexpected(preference.error());
		}

		std::size_t offset = start + 2;

		auto exchange = decode_name(packet, offset);

		if (!exchange)
		{
			return std::unexpected(exchange.error());
		}

		if (offset != start + length)
		{
			return invalid_packet("Invalid MX RDATA");
		}

		return std::format("{} {}", *preference, *exchange);
	}

	case RecordType::TXT: {
		std::string result;
		std::size_t offset = 0;

		while (offset < data.size())
		{
			const auto size = std::to_integer<std::size_t>(data[offset++]);

			if (size > data.size() - offset)
			{
				return invalid_packet("Invalid TXT record");
			}

			if (!result.empty())
			{
				result += ' ';
			}

			result.append(reinterpret_cast<const char*>(data.data() + offset), size);

			offset += size;
		}

		return result;
	}

	default:
		return std::format("<raw data {} bytes>", length);
	}
}

} // namespace

namespace dns::codec
{

std::expected<Record, Error> parse_record(const Data packet, std::size_t& offset)
{
	Record record;
	auto name = decode_name(packet, offset);

	if (!name)
	{
		return std::unexpected(name.error());
	}

	record.name = std::move(*name);

	auto type = net_read<std::uint16_t>(packet, offset);
	auto recordClass = net_read<std::uint16_t>(packet, offset + 2);
	auto ttl = net_read<std::uint32_t>(packet, offset + 4);
	auto length = net_read<std::uint16_t>(packet, offset + 8);

	if (!type || !recordClass || !ttl || !length)
	{
		return invalid_packet("Invalid resource record header");
	}

	offset += 10;

	if (*length > packet.size() - offset)
	{
		return invalid_packet("RDATA exceeds packet bounds");
	}

	record.type = static_cast<RecordType>(*type);
	record.record_class = *recordClass;
	record.ttl = *ttl;

	record.data.assign(
		packet.begin() + static_cast<std::ptrdiff_t>(offset),
		packet.begin() + static_cast<std::ptrdiff_t>(offset + *length));

	auto value = decode_record_value(
		packet,
		offset,
		*length,
		record.type);

	if (!value)
	{
		return std::unexpected(value.error());
	}

	record.value = std::move(*value);

	offset += *length;

	return record;
}

std::expected<Buffer, Error> encode_name(const std::string_view name)
{
	Buffer result;

	if (name.empty())
	{
		return invalid_packet("Empty domain name");
	}

	if (name == ".")
	{
		result.push_back(std::byte{ 0 });
		return result;
	}

	std::size_t totalLength = 1;
	std::size_t start = 0;

	while (start < name.size())
	{
		const auto end = name.find('.', start);

		const auto label = name.substr(
			start,
			end == std::string_view::npos
				? name.size() - start
				: end - start);

		if (label.empty() || label.size() > 63)
		{
			return invalid_packet("Invalid DNS label length");
		}

		totalLength += label.size() + 1;

		if (totalLength > 255)
		{
			return invalid_packet("Domain name is too long");
		}

		result.push_back(static_cast<std::byte>(label.size()));

		for (const unsigned char ch : label)
		{
			result.push_back(static_cast<std::byte>(ch));
		}

		if (end == std::string_view::npos)
		{
			break;
		}

		start = end + 1;

		if (start == name.size())
		{
			break;
		}
	}

	result.push_back(std::byte{ 0 });

	return result;
}

std::expected<std::string, Error> decode_name(const Data packet, std::size_t& offset)
{
	std::string result;
	std::unordered_set<std::size_t> visited;

	std::size_t cursor = offset;
	std::size_t encodedLength = 0;
	bool jumped = false;

	while (true)
	{
		if (cursor >= packet.size())
		{
			return invalid_packet("Name exceeds packet bounds");
		}

		const auto length = std::to_integer<std::uint8_t>(packet[cursor]);

		if (length == 0)
		{
			if (!jumped)
			{
				offset = cursor + 1;
			}

			break;
		}

		if ((length & 0xC0) == 0xC0)
		{
			if (cursor + 1 >= packet.size())
			{
				return invalid_packet("Incomplete compression pointer");
			}

			const auto pointer = (static_cast<std::size_t>(length & 0x3F) << 8) | std::to_integer<std::size_t>(packet[cursor + 1]);

			if (pointer >= packet.size())
			{
				return invalid_packet("Invalid compression pointer");
			}

			if (!visited.insert(pointer).second)
			{
				return invalid_packet("Compression pointer loop");
			}

			if (!jumped)
			{
				offset = cursor + 2;
			}

			cursor = pointer;
			jumped = true;
			continue;
		}

		if ((length & 0xC0) != 0)
		{
			return invalid_packet("Invalid DNS label type");
		}

		++cursor;

		if (length > packet.size() - cursor)
		{
			return invalid_packet("Label exceeds packet bounds");
		}

		encodedLength += length + 1;

		if (encodedLength > 254)
		{
			return invalid_packet("Domain name is too long");
		}

		if (!result.empty())
		{
			result += '.';
		}

		result.append(reinterpret_cast<const char*>(packet.data() + cursor), length);

		cursor += length;

		if (!jumped)
		{
			offset = cursor;
		}
	}

	return result;
}

std::expected<Buffer, Error> create_query(
	const std::string_view domain,
	const RecordType type,
	const std::uint16_t id)
{
	auto encodedName = encode_name(domain);

	if (!encodedName)
	{
		return std::unexpected(encodedName.error());
	}

	Buffer packet;
	packet.reserve(12 + encodedName->size() + 4);

	net_write(packet, id);
	net_write(packet, std::uint16_t{ 0 });
	net_write(packet, std::uint16_t{ 1 });
	net_write(packet, std::uint16_t{ 0 });
	net_write(packet, std::uint16_t{ 0 });
	net_write(packet, std::uint16_t{ 0 });

	packet.insert(
		packet.end(),
		encodedName->begin(),
		encodedName->end());

	net_write(packet, to_value(type));
	net_write(packet, std::uint16_t{ 1 });

	return packet;
}

Result parse_response(const Data packet)
{
	if (packet.size() < 12)
	{
		return invalid_packet("DNS header is incomplete");
	}

	Message message;

	auto id = net_read<std::uint16_t>(packet, 0);
	auto flags = net_read<std::uint16_t>(packet, 2);
	auto questions = net_read<std::uint16_t>(packet, 4);
	auto answers = net_read<std::uint16_t>(packet, 6);
	auto authority = net_read<std::uint16_t>(packet, 8);
	auto additional = net_read<std::uint16_t>(packet, 10);

	if (!id || !flags || !questions || !answers || !authority || !additional)
	{
		return invalid_packet("Invalid DNS header");
	}

	message.id = *id;
	message.flags = *flags;

	if ((*flags & 0x8000) == 0)
	{
		return invalid_packet("Packet is not a DNS response");
	}

	if (*questions > 64 || *answers > 4096 || *authority > 4096 || *additional > 4096)
	{
		return invalid_packet("Excessive DNS record count");
	}

	std::size_t offset = 12;

	for (std::uint16_t i = 0; i < *questions; ++i)
	{

		if (auto name = decode_name(packet, offset); !name)
		{
			return std::unexpected(name.error());
		}

		if (packet.size() - offset < 4)
		{
			return invalid_packet("Incomplete DNS question");
		}

		offset += 4;
	}

	const auto parseSection = [&packet, &offset](const std::uint16_t count, std::vector<Record>& records)
		-> std::expected<void, Error> {
		records.reserve(count);

		for (std::uint16_t i = 0; i < count; ++i)
		{
			auto record = parse_record(packet, offset);

			if (!record)
			{
				return std::unexpected(record.error());
			}

			records.push_back(std::move(*record));
		}

		return {};
	};

	if (auto result = parseSection(*answers, message.answers); !result)
	{
		return std::unexpected(result.error());
	}

	if (auto result = parseSection(*authority, message.authority); !result)
	{
		return std::unexpected(result.error());
	}

	if (auto result = parseSection(*additional, message.additional); !result)
	{
		return std::unexpected(result.error());
	}

	if (offset != packet.size())
	{
		return invalid_packet("Unexpected trailing DNS data");
	}

	return message;
}

std::string_view type_name(const RecordType type) noexcept
{
	switch (type)
	{ // clang-format off
	case RecordType::A:     return "A";
	case RecordType::NS:    return "NS";
	case RecordType::CNAME: return "CNAME";
	case RecordType::SOA:   return "SOA";
	case RecordType::PTR:   return "PTR";
	case RecordType::MX:    return "MX";
	case RecordType::TXT:   return "TXT";
	case RecordType::AAAA:  return "AAAA";
	case RecordType::RRSIG: return "RRSIG";
	case RecordType::ANY:   return "ANY";
	default:                return "UNKNOWN";
	} // clang-format on
}

std::expected<RecordType, Error> parse_type(const std::string_view name)
{
	constexpr std::array types{
		RecordType::A,
		RecordType::NS,
		RecordType::CNAME,
		RecordType::SOA,
		RecordType::PTR,
		RecordType::MX,
		RecordType::TXT,
		RecordType::AAAA,
		RecordType::RRSIG,
		RecordType::ANY
	};

	const auto found = std::ranges::find_if(
		types,
		[name](const RecordType type) {
			const auto value = type_name(type);

			return std::ranges::equal(
				value,
				name,
				{},
				[](const char ch) {
					return static_cast<unsigned char>(std::toupper(
						static_cast<unsigned char>(ch)));
				},
				[](const char ch) {
					return static_cast<unsigned char>(std::toupper(
						static_cast<unsigned char>(ch)));
				});
		});

	if (found == types.end())
	{
		return std::unexpected(Error{
			.code = ErrorCode::InvalidResponse,
			.message = "Unsupported DNS record type: " + std::string(name) });
	}

	return *found;
}

} // namespace dns::codec
