
#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Types.hpp"

namespace dns::codec
{

using Buffer = std::vector<std::byte>;
using Data = std::span<const std::byte>;

[[nodiscard]] std::expected<Buffer, Error> encode_name(std::string_view name);

[[nodiscard]] std::expected<std::string, Error> decode_name(Data packet, std::size_t& offset);

[[nodiscard]] std::expected<Buffer, Error> create_query(std::string_view domain, RecordType type, std::uint16_t id);

[[nodiscard]] Result parse_response(Data packet);

[[nodiscard]] std::string_view type_name(RecordType type) noexcept;

[[nodiscard]] std::expected<RecordType, Error> parse_type(std::string_view name);

} // namespace dns::codec
