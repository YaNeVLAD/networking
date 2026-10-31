#include "../include/IterativeResolver.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <ranges>
#include <unordered_set>

namespace dns
{
namespace
{

constexpr std::array ROOT_SERVERS{
	"198.41.0.4",
	"199.9.14.201",
	"192.33.4.12",
	"199.7.91.13",
	"192.203.230.10",
	"192.5.5.241",
	"192.112.36.4",
	"198.97.190.53",
	"192.36.148.17",
	"192.58.128.30",
	"193.0.14.129",
	"199.7.83.42",
	"202.12.27.33"
};

constexpr unsigned MAX_STEPS = 32;

std::string normalize(const std::string_view name)
{
	std::string result(name);

	if (!result.empty() && result.back() == '.')
	{
		result.pop_back();
	}

	std::ranges::transform(
		result,
		result.begin(),
		[](const unsigned char ch) {
			return static_cast<char>(std::tolower(ch));
		});

	return result;
}

bool eq_name(
	const std::string_view lhs,
	const std::string_view rhs)
{
	return normalize(lhs) == normalize(rhs);
}

struct ResolutionGuard
{
	std::vector<std::string>& names;

	explicit ResolutionGuard(
		std::vector<std::string>& names,
		std::string name)
		: names(names)
	{
		names.push_back(std::move(name));
	}

	~ResolutionGuard()
	{
		names.pop_back();
	}
};

std::unexpected<Error> resolution_error(
	const ErrorCode code,
	std::string message)
{
	return std::unexpected(Error{ code, std::move(message) });
}

} // namespace

IterativeResolver::IterativeResolver(const Client client)
	: m_client(client)
{
}

ResolveResult IterativeResolver::resolve(
	const std::string_view domain,
	const RecordType type) const
{
	std::vector<std::string> resolving;

	return resolve_impl(
		std::string(domain),
		type,
		resolving);
}

ResolveResult IterativeResolver::resolve_nameserver(
	const std::string_view name,
	std::vector<std::string>& resolving) const
{
	return resolve_impl(
		std::string(name),
		RecordType::A,
		resolving);
}

ResolveResult IterativeResolver::resolve_impl(
	std::string domain,
	const RecordType type,
	std::vector<std::string>& resolving) const
{
	const auto normalized = normalize(domain);
	if (normalized.empty())
	{
		return resolution_error(ErrorCode::InvalidResponse, "Empty domain name");
	}
	if (std::ranges::contains(resolving, normalized))
	{
		return resolution_error(ErrorCode::ResolutionLimit, "Circular DNS resolution dependency: " + domain);
	}

	ResolutionGuard guard(resolving, normalized);

	std::vector<std::string> servers;
	for (const auto server : ROOT_SERVERS)
	{
		servers.emplace_back(server);
	}

	std::unordered_set<std::string> visited;
	std::string currentTarget = std::move(domain);

	for (unsigned step = 0; step < MAX_STEPS; ++step)
	{
		if (servers.empty())
		{
			break;
		}

		std::vector<std::string> nextServers;
		bool followCname = false;

		for (const auto& server : servers)
		{
			if (const auto visitKey = normalize(currentTarget) + "|" + server;
				!visited.insert(visitKey).second)
			{
				continue;
			}

			auto response = m_client.query(
				server,
				currentTarget,
				type);

			if (!response)
			{
				continue;
			}

			const auto rcode = response->response_code();

			if (rcode == 3)
			{
				return resolution_error(ErrorCode::NameNotFound, "NXDOMAIN: " + currentTarget);
			}

			if (rcode != 0)
			{
				continue;
			}

			std::vector<std::string> results;
			std::string cnameTarget;
			for (const auto& answer : response->answers)
			{
				if (answer.type == type)
				{
					results.push_back(answer.value);
				}
				else if (answer.type == RecordType::CNAME)
				{
					cnameTarget = answer.value;
				}
			}

			if (!results.empty())
			{
				return results;
			}

			if (!cnameTarget.empty())
			{
				currentTarget = std::move(cnameTarget);
				followCname = true;
				break;
			}

			std::vector<std::string> nsNames;
			for (const auto& record : response->authority)
			{
				if (record.type == RecordType::NS && !record.value.empty())
				{
					nsNames.push_back(record.value);
				}
			}

			if (nsNames.empty())
			{
				continue;
			}

			for (const auto& nsName : nsNames)
			{
				for (const auto& record : response->additional)
				{
					if (record.type == RecordType::A
						&& record.record_class == 1
						&& eq_name(record.name, nsName)
						&& !record.value.empty())
					{
						nextServers.push_back(record.value);
					}
				}
			}

			if (!nextServers.empty())
			{
				break;
			}

			for (const auto& nsName : nsNames)
			{
				auto addresses = resolve_nameserver(nsName, resolving);
				if (!addresses)
				{
					continue;
				}

				nextServers.insert(nextServers.end(), addresses->begin(), addresses->end());
			}

			if (!nextServers.empty())
			{
				break;
			}
		}

		if (followCname)
		{
			servers.clear();

			for (const auto server : ROOT_SERVERS)
			{
				servers.emplace_back(server);
			}

			visited.clear();

			continue;
		}

		if (!nextServers.empty())
		{
			std::ranges::sort(nextServers);
			nextServers.erase(
				std::ranges::unique(nextServers).begin(),
				nextServers.end());

			servers = std::move(nextServers);
			continue;
		}

		servers.erase(servers.begin());
	}

	return resolution_error(ErrorCode::ResolutionLimit, "DNS resolution failed or exceeded iteration limit: " + currentTarget);
}

} // namespace dns
