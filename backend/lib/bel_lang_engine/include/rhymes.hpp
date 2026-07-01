#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ryfmach::bel {

std::optional<std::string> WorkingPart(
    std::string_view word,
    std::size_t stress,
    int mistake = 0);

std::optional<std::uint64_t> SoundHash(
    std::string_view word,
    std::size_t stress,
    int mistake = 0);

} // namespace ryfmach::bel
