#include "transcription.hpp"

namespace ryfmach::bel {
namespace {

std::optional<Phoneme> PlainVowelSound(Letter letter) noexcept {
    switch (letter) {
        case Letter::kA:
            return Phoneme::kA;
        case Letter::kO:
            return Phoneme::kO;
        case Letter::kU:
            return Phoneme::kU;
        case Letter::kYery:
            return Phoneme::kY;
        case Letter::kE:
            return Phoneme::kE;
        case Letter::kI:
            return Phoneme::kI;
        default:
            return std::nullopt;
    }
}

std::optional<Phoneme> IotatedVowelSound(Letter letter) noexcept {
    switch (letter) {
        case Letter::kYa:
            return Phoneme::kA;
        case Letter::kYo:
            return Phoneme::kO;
        case Letter::kYu:
            return Phoneme::kU;
        case Letter::kYe:
            return Phoneme::kE;
        default:
            return std::nullopt;
    }
}

std::optional<Phoneme> ConsonantSound(Letter letter) noexcept {
    switch (letter) {
        case Letter::kBe:
            return Phoneme::kB;
        case Letter::kVe:
            return Phoneme::kV;
        case Letter::kHe:
            return Phoneme::kH;
        case Letter::kDe:
            return Phoneme::kD;
        case Letter::kZhe:
            return Phoneme::kZh;
        case Letter::kZe:
            return Phoneme::kZ;
        case Letter::kShortI:
            return Phoneme::kJ;
        case Letter::kKa:
            return Phoneme::kK;
        case Letter::kEl:
            return Phoneme::kL;
        case Letter::kEm:
            return Phoneme::kM;
        case Letter::kEn:
            return Phoneme::kN;
        case Letter::kPe:
            return Phoneme::kP;
        case Letter::kEr:
            return Phoneme::kR;
        case Letter::kEs:
            return Phoneme::kS;
        case Letter::kTe:
            return Phoneme::kT;
        case Letter::kShortU:
            return Phoneme::kW;
        case Letter::kEf:
            return Phoneme::kF;
        case Letter::kKha:
            return Phoneme::kKh;
        case Letter::kTse:
            return Phoneme::kTs;
        case Letter::kChe:
            return Phoneme::kCh;
        case Letter::kSha:
            return Phoneme::kSh;
        default:
            return std::nullopt;
    }
}

bool NeedsIotation(std::span<const Letter> word, std::size_t index) noexcept {
    if (index == 0) {
        return true;
    }

    const Letter previous = word[index - 1];
    return IsVowel(previous) || previous == Letter::kShortU ||
           previous == Letter::kSoftSign || IsApostrophe(previous) ||
           IsDash(previous);
}

bool SoftensNextConsonant(Letter letter) noexcept {
    return IsSofteningVowel(letter) || letter == Letter::kSoftSign;
}

void AppendInitialSound(
    std::vector<Sound>& transcription,
    Phoneme phoneme,
    bool stressed
) {
    transcription.push_back(Sound{
        .phoneme = phoneme,
        .stressed = stressed,
    });
}

void FoldAffricates(std::vector<Sound>& transcription) {
    for (std::size_t index = 0; index + 1 < transcription.size(); ++index) {
        if (transcription[index].phoneme != Phoneme::kD) {
            continue;
        }

        const Phoneme next = transcription[index + 1].phoneme;
        if (next == Phoneme::kZh) {
            transcription[index].phoneme = Phoneme::kDzh;
        } else if (next == Phoneme::kZ) {
            transcription[index].phoneme = Phoneme::kDz;
        } else if (next == Phoneme::kZSoft) {
            transcription[index].phoneme = Phoneme::kDzSoft;
        } else {
            continue;
        }

        transcription.erase(
            transcription.begin() + static_cast<std::ptrdiff_t>(index + 1));
    }
}

bool AssimilateRingOrThud(std::vector<Sound>& transcription, std::size_t index) {
    const Phoneme current = transcription[index].phoneme;
    const bool has_next = index + 1 < transcription.size();

    const auto thud_pair = ThudPair(current);
    if (IsRing(current) && thud_pair &&
        (!has_next || IsThud(transcription[index + 1].phoneme))) {
        transcription[index].phoneme = *thud_pair;
        return true;
    }

    const auto ring_pair = RingPair(current);
    if (IsThud(current) && ring_pair && has_next &&
        IsRing(transcription[index + 1].phoneme) &&
        !IsSonor(transcription[index + 1].phoneme)) {
        transcription[index].phoneme = *ring_pair;
        return true;
    }

    return false;
}

bool AssimilateSoftness(std::vector<Sound>& transcription, std::size_t index) {
    if (index + 1 >= transcription.size()) {
        return false;
    }

    const Phoneme current = transcription[index].phoneme;
    const Phoneme next = transcription[index + 1].phoneme;
    const bool z_or_s_softening =
        (current == Phoneme::kZ || current == Phoneme::kS) && IsSoft(next) &&
        next != Phoneme::kHSoft && next != Phoneme::kKSoft &&
        next != Phoneme::kKhSoft;
    const bool dental_softening =
        (current == Phoneme::kD || current == Phoneme::kT ||
         current == Phoneme::kDz || current == Phoneme::kTs) &&
        (next == Phoneme::kTsSoft || next == Phoneme::kDzSoft ||
         next == Phoneme::kVSoft);

    if (!z_or_s_softening && !dental_softening) {
        return false;
    }

    const auto soft_pair = SoftPair(current);
    if (!soft_pair) {
        return false;
    }

    transcription[index].phoneme = *soft_pair;
    return true;
}

bool AssimilateWhistlingOrHissing(
    std::vector<Sound>& transcription,
    std::size_t index
) {
    if (index + 1 >= transcription.size()) {
        return false;
    }

    const Phoneme current = transcription[index].phoneme;
    const Phoneme next = transcription[index + 1].phoneme;

    if (IsHissing(current) && IsWhistling(next)) {
        const auto whistling_pair = WhistlingPair(current);
        if (whistling_pair) {
            transcription[index].phoneme = *whistling_pair;
            return true;
        }
    } else if (IsWhistling(current) && IsHissing(next)) {
        const auto hissing_pair = HissingPair(current);
        if (hissing_pair) {
            transcription[index].phoneme = *hissing_pair;
            return true;
        }
    }

    return false;
}

bool AssimilateDental(std::vector<Sound>& transcription, std::size_t index) {
    if (index + 1 >= transcription.size()) {
        return false;
    }

    const Phoneme current = transcription[index].phoneme;
    const Phoneme next = transcription[index + 1].phoneme;
    if ((current == Phoneme::kD || current == Phoneme::kT) &&
        (next == Phoneme::kTs || next == Phoneme::kCh)) {
        transcription[index].phoneme = next;
        return true;
    }

    return false;
}

void Assimilate(std::vector<Sound>& transcription) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t index = 0; index < transcription.size(); ++index) {
            changed = AssimilateRingOrThud(transcription, index) || changed;
            changed = AssimilateSoftness(transcription, index) || changed;
            changed = AssimilateWhistlingOrHissing(transcription, index) || changed;
            changed = AssimilateDental(transcription, index) || changed;
        }
    }
}

} // namespace

std::optional<std::vector<Sound>> GetTranscriptionSounds(
    std::string_view word,
    std::size_t stress
) {
    const auto parsed_word = ParseWord(word);
    if (!parsed_word) {
        return std::nullopt;
    }

    return GetTranscriptionSounds(*parsed_word, stress);
}

std::optional<std::vector<Sound>> GetTranscriptionSounds(
    std::span<const Letter> word,
    std::size_t stress
) {
    if (stress >= word.size() || !IsVowel(word[stress])) {
        return std::nullopt;
    }

    std::vector<Sound> transcription;
    for (std::size_t index = 0; index < word.size(); ++index) {
        const bool stressed = index == stress;

        if (const auto vowel = PlainVowelSound(word[index])) {
            AppendInitialSound(transcription, *vowel, stressed);
            continue;
        }

        if (const auto vowel = IotatedVowelSound(word[index])) {
            if (NeedsIotation(word, index)) {
                AppendInitialSound(transcription, Phoneme::kJ, false);
            }
            AppendInitialSound(transcription, *vowel, stressed);
            continue;
        }

        if (const auto consonant = ConsonantSound(word[index])) {
            Phoneme sound = *consonant;
            if (index + 1 < word.size() &&
                SoftensNextConsonant(word[index + 1])) {
                if (const auto soft_pair = SoftPair(sound)) {
                    sound = *soft_pair;
                }
            }
            AppendInitialSound(transcription, sound, false);
        }
    }

    FoldAffricates(transcription);
    Assimilate(transcription);
    return transcription;
}

std::optional<std::size_t> GetAccentInTranscription(
    std::span<const Sound> transcription
) noexcept {
    for (std::size_t index = 0; index < transcription.size(); ++index) {
        if (IsStressedVowel(transcription[index])) {
            return index;
        }
    }

    return std::nullopt;
}

} // namespace ryfmach::bel
