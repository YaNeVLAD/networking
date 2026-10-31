
#include <iostream>
#include <string_view>

#include "include/IterativeResolver.hpp"

int main(const int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cerr
			<< "Usage: dns_resolver.exe <domain> <record_type>\n"
			<< "Example: dns_resolver.exe example.com A\n";

		return 1;
	}

	const std::string_view domain = argv[1];
	const std::string_view typeName = argv[2];

	auto type = dns::codec::parse_type(typeName);

	if (!type)
	{
		std::cerr << type.error().message << '\n';
		return 1;
	}

	const dns::IterativeResolver resolver;

	auto result = resolver.resolve(domain, *type);
	if (!result)
	{
		std::cerr << result.error().message << '\n';
		return 1;
	}

	if (result->empty())
	{
		std::cout << "No records found\n";
		return 1;
	}

	for (const auto& value : *result)
	{
		std::cout << value << '\n';
	}

	return 0;
}
