#ifndef RYFMACH_BEL_LANG_ENGINE_INCLUDE_TRANSCRIPTION_HPP_
#define RYFMACH_BEL_LANG_ENGINE_INCLUDE_TRANSCRIPTION_HPP_

#include "sounds.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ryfmach::bel {

enum class PhoneticPhenomenon : std::uint8_t {
    kIotation,
    kConsonantSoftening,
    kAffricates,
    kThudAssimilation,
    kRingAssimilation,
    kSoftAssimilation,
    kWhistlingAssimilation,
    kHissingAssimilation,
    kDentalAssimilation,
};

struct PhoneticPhenomenonOccurrence {
    std::size_t sound_index;
    std::vector<Sound> transcription;
    PhoneticPhenomenon phenomenon;
};

struct FullTranscription {
    std::vector<std::vector<std::size_t>> letter_to_sounds;
    std::vector<Sound> transcription;
    std::vector<PhoneticPhenomenonOccurrence> phenomena;
};

std::optional<std::vector<Sound>> GetTranscriptionSounds(
    std::string_view word,
    std::size_t accent);

std::optional<std::vector<Sound>> GetTranscriptionSounds(
    std::span<const Letter> word,
    std::size_t accent);

std::optional<FullTranscription> GetTranscriptionFull(
    std::string_view word,
    std::size_t accent);

std::optional<FullTranscription> GetTranscriptionFull(
    std::span<const Letter> word,
    std::size_t accent);

std::optional<std::size_t> GetAccentInTranscription(
    std::span<const Sound> transcription) noexcept;

} // namespace ryfmach::bel

#endif // RYFMACH_BEL_LANG_ENGINE_INCLUDE_TRANSCRIPTION_HPP_
