#include "../include/Client.hpp"

#include "common/net/Utils.hpp"
#include "common/udp/Socket.hpp"

#include <mutex>
#include <random>
#include <string>

#include <common/tcp/Socket.hpp>

using namespace dns;

namespace
{

std::unexpected<Error> network_error(std::string&& message)
{
	return std::unexpected(
		Error{
			.code = ErrorCode::NetworkError,
			.message = message,
		});
}

std::uint16_t generate_id()
{
	static std::mutex m;
	static std::mt19937 g(std::random_device{}());

	std::lock_guard lock(m);

	return std::uniform_int_distribution<std::uint16_t>{}(g);
}

std::expected<void, Error> send(
	const tcp::Socket& socket,
	const codec::Data data)
{
	try
	{
		socket.send(data);
	}
	catch (const std::exception& e)
	{
		return network_error(e.what());
	}

	return {};
}

std::expected<codec::Buffer, Error> receive_exact_size(
	const tcp::Socket& socket,
	const std::size_t size)
{
	codec::Buffer result(size);
	try
	{
		std::size_t received = 0;
		while (received < size)
		{
			const std::string chunk = socket.receive(size - received);

			if (chunk.empty())
			{
				return network_error("Connection closed before receiving complete DNS message");
			}

			std::memcpy(
				result.data() + received,
				chunk.data(),
				chunk.size());

			received += chunk.size();
		}
	}
	catch (const std::exception& e)
	{
		return network_error(e.what());
	}

	return result;
}

Result validate_response(
	const codec::Data packet,
	const std::uint16_t expected_id)
{
	auto response = codec::parse_response(packet);

	if (!response)
	{
		return std::unexpected(response.error());
	}

	if (response->id != expected_id)
	{
		return std::unexpected(Error{
			.code = ErrorCode::InvalidResponse,
			.message = "DNS transaction ID mismatch" });
	}

	return response;
}

} // namespace

Client::Client(const std::chrono::milliseconds timeout)
	: m_timeout(timeout)
{
}

Result Client::query(
	const std::string_view server,
	const std::string_view domain,
	const RecordType type) const
{
	const auto id = generate_id();

	auto request = codec::create_query(domain, type, id);

	if (!request)
	{
		return std::unexpected(request.error());
	}

	auto response = exchange_udp(server, *request, id);

	if (!response)
	{
		return response;
	}

	if (!response->truncated())
	{
		return response;
	}

	return exchange_tcp(server, *request, id);
}

Result Client::exchange_udp(
	const std::string_view server,
	const codec::Buffer& request,
	const std::uint16_t id) const
{
	try
	{
		const udp::Socket socket;

		socket.send_to(
			server,
			53,
			codec::Data(request));

		if (!socket.wait_for(static_cast<long>(m_timeout.count())))
		{
			return std::unexpected(Error{
				.code = ErrorCode::Timeout,
				.message = "UDP DNS request timed out: " + std::string(server),
			});
		}

		const auto datagram = socket.receive_from(4096);

		return validate_response(
			codec::Data{
				reinterpret_cast<const std::byte*>(datagram.data.data()),
				datagram.data.size(),
			},
			id);
	}
	catch (const std::exception& e)
	{
		return network_error(e.what());
	}
}

Result Client::exchange_tcp(
	const std::string_view server,
	const codec::Buffer& request,
	const std::uint16_t id)
{
	try
	{
		const tcp::Socket socket;
		socket.connect(server, 53);

		if (request.size() > 65535)
		{
			return std::unexpected(Error{
				.code = ErrorCode::InvalidPacket,
				.message = "DNS TCP request exceeds maximum size",
			});
		}

		codec::Buffer framed;
		framed.reserve(request.size() + 2);

		const auto length = net::conv::to_network(
			static_cast<std::uint16_t>(request.size()));

		const auto* lengthBytes = reinterpret_cast<const std::byte*>(&length);

		framed.insert(framed.end(), lengthBytes, lengthBytes + 2);
		framed.insert(framed.end(), request.begin(), request.end());

		if (auto sent = send(socket, framed); !sent)
		{
			return std::unexpected(sent.error());
		}

		auto lengthBuffer = receive_exact_size(socket, 2);

		if (!lengthBuffer)
		{
			return std::unexpected(lengthBuffer.error());
		}

		std::uint16_t responseLength{};

		std::memcpy(
			&responseLength,
			lengthBuffer->data(),
			sizeof(responseLength));

		responseLength = net::conv::to_host(responseLength);

		if (responseLength < 12)
		{
			return std::unexpected(Error{
				.code = ErrorCode::InvalidPacket,
				.message = "Invalid TCP DNS response length",
			});
		}

		auto response = receive_exact_size(socket, responseLength);

		if (!response)
		{
			return std::unexpected(response.error());
		}

		return validate_response(*response, id);
	}
	catch (const std::exception& e)
	{
		return network_error(e.what());
	}
}