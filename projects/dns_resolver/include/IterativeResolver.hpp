#pragma once

#include <string_view>
#include <vector>

#include "Client.hpp"

namespace dns
{

class IterativeResolver
{
public:
	explicit IterativeResolver(Client client = Client{});

	[[nodiscard]] ResolveResult resolve(
		std::string_view domain,
		RecordType type = RecordType::A) const;

private:
	[[nodiscard]] ResolveResult resolve_impl(
		std::string domain,
		RecordType type,
		std::vector<std::string>& resolving) const;

	[[nodiscard]] ResolveResult resolve_nameserver(
		std::string_view name,
		std::vector<std::string>& resolving) const;

	Client m_client;
};

} // namespace dns
