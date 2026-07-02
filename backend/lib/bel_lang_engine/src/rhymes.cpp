#include "rhymes.hpp"

#include "sounds.hpp"
#include "transcription.hpp"
#include "utils/hash.hpp"

#include <fstream>
#include <sstream>
#include <vector>

namespace ryfmach::bel {
namespace {

constexpr std::uint64_t kSoundHashMod = 12345678901234277ULL;
constexpr double kDefaultUnknownReplaceCost = 1000.0;
constexpr std::size_t kPhonemeCount =
    static_cast<std::size_t>(Phoneme::kE) + 1;
constexpr std::size_t kStressedVowelCount = 6;

// Keys: empty, base phonemes in Phoneme enum order, then stressed vowels.
static_assert(
    SoundCompatibilityTable::kSoundKeyCount ==
    1 + kPhonemeCount + kStressedVowelCount);

std::optional<std::size_t> StressedVowelIndex(Phoneme phoneme) noexcept {
    switch (phoneme) {
        case Phoneme::kA:
            return 0;
        case Phoneme::kO:
            return 1;
        case Phoneme::kU:
            return 2;
        case Phoneme::kI:
            return 3;
        case Phoneme::kY:
            return 4;
        case Phoneme::kE:
            return 5;
        default:
            return std::nullopt;
    }
}

std::optional<std::size_t> SoundKey(std::optional<Sound> sound) noexcept {
    if (!sound) {
        return 0;
    }

    if (sound->stressed) {
        if (const auto vowel_index = StressedVowelIndex(sound->phoneme)) {
            return 1 + kPhonemeCount + *vowel_index;
        }
    }

    const auto phoneme_index = static_cast<std::size_t>(sound->phoneme);
    if (phoneme_index >= kPhonemeCount) {
        return std::nullopt;
    }
    return 1 + phoneme_index;
}

bool ParseSoundKey(std::string_view key, std::optional<Sound>& sound) noexcept {
    if (key == "<empty>") {
        sound = std::nullopt;
        return true;
    }

    for (std::size_t index = 0; index < kPhonemeCount; ++index) {
        const auto phoneme = static_cast<Phoneme>(index);
        const Sound unstressed{phoneme};
        if (SoundSpelling(unstressed) == key) {
            sound = unstressed;
            return true;
        }

        if (StressedVowelIndex(phoneme)) {
            const Sound stressed{phoneme, true};
            if (SoundSpelling(stressed) == key) {
                sound = stressed;
                return true;
            }
        }
    }

    return false;
}

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

SoundCompatibilityTable::SoundCompatibilityTable() {
    costs_.fill(kDefaultUnknownReplaceCost);
    SetCost(std::optional<Sound>{}, std::optional<Sound>{}, 0.0);
}

double SoundCompatibilityTable::Cost(Sound left, Sound right) const noexcept {
    return Cost(std::optional<Sound>{left}, std::optional<Sound>{right});
}

double SoundCompatibilityTable::Cost(
    std::optional<Sound> left,
    std::optional<Sound> right) const noexcept {
    const auto left_key = SoundKey(left);
    const auto right_key = SoundKey(right);
    if (!left_key || !right_key) {
        return kDefaultUnknownReplaceCost;
    }

    return costs_[*left_key * kSoundKeyCount + *right_key];
}

void SoundCompatibilityTable::SetCost(Sound left, Sound right, double cost) noexcept {
    SetCost(std::optional<Sound>{left}, std::optional<Sound>{right}, cost);
}

void SoundCompatibilityTable::SetCost(
    std::optional<Sound> left,
    std::optional<Sound> right,
    double cost) noexcept {
    const auto left_key = SoundKey(left);
    const auto right_key = SoundKey(right);
    if (!left_key || !right_key) {
        return;
    }

    costs_[*left_key * kSoundKeyCount + *right_key] = cost;
    costs_[*right_key * kSoundKeyCount + *left_key] = cost;
}

std::optional<SoundCompatibilityTable> LoadSoundCompatibilityTable(
    const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        return std::nullopt;
    }

    SoundCompatibilityTable table;

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream line_stream(line);
        std::string left_text;
        std::string right_text;
        std::string cost_text;
        if (!std::getline(line_stream, left_text, '\t') ||
            !std::getline(line_stream, right_text, '\t') ||
            !std::getline(line_stream, cost_text)) {
            return std::nullopt;
        }

        std::optional<Sound> left;
        std::optional<Sound> right;
        if (!ParseSoundKey(left_text, left) || !ParseSoundKey(right_text, right)) {
            return std::nullopt;
        }

        try {
            table.SetCost(left, right, std::stod(cost_text));
        } catch (...) {
            return std::nullopt;
        }
    }

    return table;
}

const SoundCompatibilityTable& DefaultSoundCompatibilityTable() noexcept {
    static const SoundCompatibilityTable table = [] {
#ifdef RYFMACH_SOUND_COMPATIBILITY_PATH
        if (const auto loaded =
                LoadSoundCompatibilityTable(RYFMACH_SOUND_COMPATIBILITY_PATH)) {
            return *loaded;
        }
#endif
        return SoundCompatibilityTable();
    }();

    return table;
}

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

double ReplaceCost(Sound left, Sound right) noexcept {
    return ReplaceCost(left, right, DefaultSoundCompatibilityTable());
}

double ReplaceCost(
    Sound left,
    Sound right,
    const SoundCompatibilityTable& table) noexcept {
    return table.Cost(left, right);
}

double ReplaceCost(
    std::optional<Sound> left,
    std::optional<Sound> right) noexcept {
    return ReplaceCost(left, right, DefaultSoundCompatibilityTable());
}

double ReplaceCost(
    std::optional<Sound> left,
    std::optional<Sound> right,
    const SoundCompatibilityTable& table) noexcept {
    return table.Cost(left, right);
}

} // namespace ryfmach::bel
