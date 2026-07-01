#include "sounds.hpp"

#include <array>

namespace ryfmach::bel {
namespace {

struct ConsonantData {
    Phoneme sound;
    std::size_t group;
    bool thud;
    bool soft;
};

constexpr std::array<ConsonantData, 39> kConsonants = {{
    {Phoneme::kB, 0, false, false},
    {Phoneme::kBSoft, 0, false, true},
    {Phoneme::kP, 0, true, false},
    {Phoneme::kPSoft, 0, true, true},
    {Phoneme::kV, 1, false, false},
    {Phoneme::kVSoft, 1, false, true},
    {Phoneme::kH, 2, false, false},
    {Phoneme::kHSoft, 2, false, true},
    {Phoneme::kKh, 2, true, false},
    {Phoneme::kKhSoft, 2, true, true},
    {Phoneme::kG, 3, false, false},
    {Phoneme::kGSoft, 3, false, true},
    {Phoneme::kK, 3, true, false},
    {Phoneme::kKSoft, 3, true, true},
    {Phoneme::kD, 4, false, false},
    {Phoneme::kDzSoft, 4, false, true},
    {Phoneme::kT, 4, true, false},
    {Phoneme::kTsSoft, 4, true, true},
    {Phoneme::kDz, 5, false, false},
    {Phoneme::kTs, 5, true, false},
    {Phoneme::kZh, 6, false, false},
    {Phoneme::kSh, 6, true, false},
    {Phoneme::kZ, 7, false, false},
    {Phoneme::kZSoft, 7, false, true},
    {Phoneme::kS, 7, true, false},
    {Phoneme::kSSoft, 7, true, true},
    {Phoneme::kJ, 8, false, true},
    {Phoneme::kL, 9, false, false},
    {Phoneme::kLSoft, 9, false, true},
    {Phoneme::kM, 10, false, false},
    {Phoneme::kMSoft, 10, false, true},
    {Phoneme::kN, 11, false, false},
    {Phoneme::kNSoft, 11, false, true},
    {Phoneme::kR, 12, false, false},
    {Phoneme::kF, 13, true, false},
    {Phoneme::kFSoft, 13, true, true},
    {Phoneme::kDzh, 14, false, false},
    {Phoneme::kCh, 14, true, false},
    {Phoneme::kW, 15, false, false},
}};

constexpr std::array<std::size_t, 7> kSonorGroups = {1, 8, 9, 10, 11, 12, 15};

std::optional<ConsonantData> GetConsonantData(Phoneme sound) noexcept {
    for (const auto& data : kConsonants) {
        if (data.sound == sound) {
            return data;
        }
    }

    return std::nullopt;
}

std::optional<Phoneme> PairByData(std::size_t group, bool thud, bool soft) noexcept {
    for (const auto& data : kConsonants) {
        if (data.group == group && data.thud == thud && data.soft == soft) {
            return data.sound;
        }
    }

    return std::nullopt;
}

bool IsSonorGroup(std::size_t group) noexcept {
    for (const std::size_t sonor_group : kSonorGroups) {
        if (sonor_group == group) {
            return true;
        }
    }

    return false;
}

} // namespace

bool IsConsonantSound(Phoneme sound) noexcept {
    return GetConsonantData(sound).has_value();
}

bool IsVowelSound(Phoneme sound) noexcept {
    switch (sound) {
        case Phoneme::kA:
        case Phoneme::kO:
        case Phoneme::kU:
        case Phoneme::kI:
        case Phoneme::kY:
        case Phoneme::kE:
            return true;
        default:
            return false;
    }
}

bool IsRing(Phoneme sound) noexcept {
    const auto data = GetConsonantData(sound);
    return data.has_value() && !data->thud;
}

bool IsThud(Phoneme sound) noexcept {
    const auto data = GetConsonantData(sound);
    return data.has_value() && data->thud;
}

bool IsHard(Phoneme sound) noexcept {
    const auto data = GetConsonantData(sound);
    return data.has_value() && !data->soft;
}

bool IsSoft(Phoneme sound) noexcept {
    const auto data = GetConsonantData(sound);
    return data.has_value() && data->soft;
}

bool IsSonor(Phoneme sound) noexcept {
    const auto data = GetConsonantData(sound);
    return data.has_value() && IsSonorGroup(data->group);
}

bool IsWhistling(Phoneme sound) noexcept {
    switch (sound) {
        case Phoneme::kZ:
        case Phoneme::kZSoft:
        case Phoneme::kS:
        case Phoneme::kSSoft:
        case Phoneme::kDz:
        case Phoneme::kDzSoft:
        case Phoneme::kTs:
        case Phoneme::kTsSoft:
            return true;
        default:
            return false;
    }
}

bool IsHissing(Phoneme sound) noexcept {
    switch (sound) {
        case Phoneme::kZh:
        case Phoneme::kSh:
        case Phoneme::kDzh:
        case Phoneme::kCh:
            return true;
        default:
            return false;
    }
}

std::optional<Phoneme> RingPair(Phoneme sound) noexcept {
    const auto data = GetConsonantData(sound);
    if (!data) {
        return std::nullopt;
    }

    return PairByData(data->group, false, data->soft);
}

std::optional<Phoneme> ThudPair(Phoneme sound) noexcept {
    const auto data = GetConsonantData(sound);
    if (!data) {
        return std::nullopt;
    }

    return PairByData(data->group, true, data->soft);
}

std::optional<Phoneme> HardPair(Phoneme sound) noexcept {
    const auto data = GetConsonantData(sound);
    if (!data) {
        return std::nullopt;
    }

    return PairByData(data->group, data->thud, false);
}

std::optional<Phoneme> SoftPair(Phoneme sound) noexcept {
    const auto data = GetConsonantData(sound);
    if (!data) {
        return std::nullopt;
    }

    return PairByData(data->group, data->thud, true);
}

std::optional<Phoneme> WhistlingPair(Phoneme sound) noexcept {
    switch (sound) {
        case Phoneme::kZh:
            return Phoneme::kZ;
        case Phoneme::kSh:
            return Phoneme::kS;
        case Phoneme::kDzh:
            return Phoneme::kDz;
        case Phoneme::kCh:
            return Phoneme::kTs;
        default:
            return std::nullopt;
    }
}

std::optional<Phoneme> HissingPair(Phoneme sound) noexcept {
    switch (sound) {
        case Phoneme::kZ:
        case Phoneme::kZSoft:
            return Phoneme::kZh;
        case Phoneme::kS:
        case Phoneme::kSSoft:
            return Phoneme::kSh;
        case Phoneme::kDz:
        case Phoneme::kDzSoft:
            return Phoneme::kDzh;
        case Phoneme::kTs:
        case Phoneme::kTsSoft:
            return Phoneme::kCh;
        default:
            return std::nullopt;
    }
}

} // namespace ryfmach::bel
