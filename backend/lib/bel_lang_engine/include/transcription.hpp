#pragma once

#include "sounds.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ryfmach::bel {

std::optional<std::vector<Sound>> GetTranscriptionSounds(
    std::string_view word,
    std::size_t accent);

std::optional<std::vector<Sound>> GetTranscriptionSounds(
    std::span<const Letter> word,
    std::size_t accent);

std::optional<std::size_t> GetAccentInTranscription(
    std::span<const Sound> transcription) noexcept;

} // namespace ryfmach::bel
