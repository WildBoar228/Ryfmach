#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace ryfmach::utils {

using Sha1Digest = std::array<std::uint8_t, 20>;

Sha1Digest Sha1(std::string_view input);
std::uint64_t DigestModulo(const Sha1Digest& digest, std::uint64_t modulo) noexcept;

} // namespace ryfmach::utils
