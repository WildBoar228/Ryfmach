#pragma once

#include "language.hpp"

#include <optional>
#include <string_view>

namespace ryfmach::bel {

struct Sound {
    Phoneme phoneme;
    bool stressed = false;
};

std::string_view SoundSpelling(Sound sound) noexcept;

bool IsConsonantPhoneme(Phoneme sound) noexcept;
bool IsVowelSound(Phoneme sound) noexcept;
bool IsVowelSound(Sound sound) noexcept;
bool IsStressedVowel(Sound sound) noexcept;

bool IsRing(Phoneme sound) noexcept;
bool IsThud(Phoneme sound) noexcept;
bool IsHard(Phoneme sound) noexcept;
bool IsSoft(Phoneme sound) noexcept;
bool IsSonor(Phoneme sound) noexcept;
bool IsWhistling(Phoneme sound) noexcept;
bool IsHissing(Phoneme sound) noexcept;

std::optional<Phoneme> RingPair(Phoneme sound) noexcept;
std::optional<Phoneme> ThudPair(Phoneme sound) noexcept;
std::optional<Phoneme> HardPair(Phoneme sound) noexcept;
std::optional<Phoneme> SoftPair(Phoneme sound) noexcept;
std::optional<Phoneme> WhistlingPair(Phoneme sound) noexcept;
std::optional<Phoneme> HissingPair(Phoneme sound) noexcept;

} // namespace ryfmach::bel
