#include <common/net/Utils.hpp>

#include <common/net/c/conversions.h>

namespace net::conv
{

std::uint16_t to_network(const std::uint16_t value)
{
	return u16_to_network(value);
}

std::uint16_t to_host(const std::uint16_t value)
{
	return u16_to_host(value);
}

std::uint32_t to_network(const std::uint32_t value)
{
	return u32_to_network(value);
}

std::uint32_t to_host(const std::uint32_t value)
{
	return u32_to_host(value);
}

} // namespace net::conv