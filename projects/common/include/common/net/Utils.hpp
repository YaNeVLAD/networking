#pragma once

#include <cstdint>

namespace net::conv
{
std::uint16_t to_network(std::uint16_t value);
std::uint16_t to_host(std::uint16_t value);

std::uint32_t to_network(std::uint32_t value);
std::uint32_t to_host(std::uint32_t value);

} // namespace net::conv

namespace net::literals
{

constexpr std::uint16_t operator""_u16(const unsigned long long v) { return static_cast<std::uint16_t>(v); }
constexpr std::uint32_t operator""_u32(const unsigned long long v) { return static_cast<std::uint32_t>(v); }

} // namespace net::literals