#include "rhymes.hpp"

#include "sounds.hpp"
#include "transcription.hpp"
#include "utils/hash.hpp"

#include <vector>

namespace ryfmach::bel {
namespace {

constexpr std::uint64_t kSoundHashMod = 12345678901234277ULL;

bool SameForIdealRhyme(Sound left, Sound right) noexcept {
    if (left.phoneme == right.phoneme && left.stressed == right.stressed) {
        return true;
    }

    return (left.phoneme == Phoneme::kDz || left.phoneme == Phoneme::kDzSoft ||
            left.phoneme == Phoneme::kDzh) &&
           right.phoneme == Phoneme::kD;
}

std::optional<std::size_t> FindStress(const std::vector<Sound>& sounds) noexcept {
    return GetAccentInTranscription(sounds);
}

void RemoveSoundDuplicates(std::vector<Sound>& sounds) {
    for (std::size_t index = 0; index < sounds.size();) {
        if (index > 0 && !IsVowelSound(sounds[index]) &&
            SameForIdealRhyme(sounds[index], sounds[index - 1])) {
            sounds.erase(sounds.begin() + static_cast<std::ptrdiff_t>(index - 1));
            continue;
        }

        if (sounds[index].phoneme == Phoneme::kY && sounds[index].stressed) {
            sounds[index].phoneme = Phoneme::kI;
        }

        ++index;
    }
}

std::vector<Sound> CutFromStress(std::vector<Sound> sounds, int mistake) {
    const auto stress = FindStress(sounds);
    if (!stress) {
        return sounds;
    }

    const std::size_t start =
        *stress == sounds.size() - 1 && sounds.size() > 1 && mistake < 2
            ? *stress - 1
            : *stress;

    return std::vector<Sound>(sounds.begin() + static_cast<std::ptrdiff_t>(start),
                              sounds.end());
}

void ApplyGoodRhyme(std::vector<Sound>& sounds) {
    for (std::size_t index = 0; index < sounds.size(); ++index) {
        if (IsVowelSound(sounds[index]) && !sounds[index].stressed) {
            sounds[index].phoneme = Phoneme::kY;
        }

        if (!IsConsonantPhoneme(sounds[index].phoneme)) {
            continue;
        }

        if (index > 0 && index + 1 < sounds.size() && IsRing(sounds[index].phoneme)) {
            if (const auto thud_pair = ThudPair(sounds[index].phoneme)) {
                sounds[index].phoneme = *thud_pair;
            }
        }

        if ((index + 1 == sounds.size() || !IsVowelSound(sounds[index + 1])) &&
            IsSoft(sounds[index].phoneme)) {
            if (const auto hard_pair = HardPair(sounds[index].phoneme)) {
                sounds[index].phoneme = *hard_pair;
            }
        }
    }
}

void ApplyMediumRhyme(std::vector<Sound>& sounds) {
    for (std::size_t index = 0; index < sounds.size();) {
        if (sounds[index].phoneme == Phoneme::kJ ||
            sounds[index].phoneme == Phoneme::kW) {
            sounds.erase(sounds.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }

        if (index + 1 < sounds.size()) {
            if (IsSonor(sounds[index].phoneme)) {
                sounds[index].phoneme = Phoneme::kL;
            } else if (IsHissing(sounds[index].phoneme)) {
                sounds[index].phoneme = Phoneme::kSh;
            } else if (IsWhistling(sounds[index].phoneme)) {
                sounds[index].phoneme = Phoneme::kS;
            }
        }

        ++index;
    }

    for (std::size_t index = 0; index < sounds.size();) {
        if (index > 0 && SameForIdealRhyme(sounds[index], sounds[index - 1])) {
            sounds.erase(sounds.begin() + static_cast<std::ptrdiff_t>(index - 1));
            continue;
        }

        ++index;
    }
}

void ApplyWeakRhyme(std::vector<Sound>& sounds) {
    for (std::size_t index = 0; index < sounds.size(); ++index) {
        if (!IsConsonantPhoneme(sounds[index].phoneme)) {
            continue;
        }

        if (index + 1 == sounds.size() || IsVowelSound(sounds[index + 1])) {
            if (IsSonor(sounds[index].phoneme)) {
                sounds[index].phoneme = Phoneme::kL;
            } else if (IsHissing(sounds[index].phoneme)) {
                sounds[index].phoneme = Phoneme::kSh;
            } else if (IsWhistling(sounds[index].phoneme)) {
                sounds[index].phoneme =
                    index + 1 == sounds.size() ? Phoneme::kS : Phoneme::kSh;
            } else if (IsRing(sounds[index].phoneme)) {
                sounds[index].phoneme = Phoneme::kV;
            } else if (IsThud(sounds[index].phoneme)) {
                sounds[index].phoneme = Phoneme::kF;
            }
        } else {
            sounds.erase(sounds.begin() + static_cast<std::ptrdiff_t>(index));
            --index;
        }
    }

    for (std::size_t index = 0; index < sounds.size();) {
        if (index > 0 && sounds[index].phoneme == sounds[index - 1].phoneme &&
            sounds[index].stressed == sounds[index - 1].stressed) {
            sounds.erase(sounds.begin() + static_cast<std::ptrdiff_t>(index - 1));
            continue;
        }

        ++index;
    }
}

std::string JoinSounds(const std::vector<Sound>& sounds) {
    std::string result;
    for (const Sound sound : sounds) {
        result += SoundSpelling(sound);
    }
    return result;
}

} // namespace

std::optional<std::string> WorkingPart(
    std::string_view word,
    std::size_t stress,
    int mistake) {
    if (mistake < 0) {
        mistake = 0;
    }

    auto sounds = GetTranscriptionSounds(word, stress);
    if (!sounds) {
        return std::nullopt;
    }

    RemoveSoundDuplicates(*sounds);
    auto working = CutFromStress(*sounds, mistake);
    if (working.empty()) {
        return std::nullopt;
    }

    if (mistake >= 1) {
        ApplyGoodRhyme(working);
    }
    if (mistake >= 2) {
        ApplyMediumRhyme(working);
    }
    if (mistake >= 3) {
        ApplyWeakRhyme(working);
    }

    return JoinSounds(working);
}

std::optional<std::uint64_t> SoundHash(
    std::string_view word,
    std::size_t stress,
    int mistake) {
    const auto working_part = WorkingPart(word, stress, mistake);
    if (!working_part) {
        return std::nullopt;
    }

    return utils::DigestModulo(utils::Sha1(*working_part), kSoundHashMod);
}

} // namespace ryfmach::bel
