#include "rhymes.hpp"

#include "sounds.hpp"
#include "transcription.hpp"
#include "utils/hash.hpp"

#include <algorithm>
#include <fstream>
#include <ranges>
#include <span>
#include <sstream>
#include <vector>

namespace ryfmach::bel {
namespace {

constexpr std::uint64_t kSoundHashMod = 12345678901234277ULL;
constexpr double kDefaultUnknownReplaceCost = 1000.0;
constexpr std::size_t kPhonemeCount =
    static_cast<std::size_t>(Phoneme::kE) + 1;
constexpr std::size_t kStressedVowelCount = 6;

constexpr std::size_t kTranscriptionBufSize = 20;

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

double SoundReplaceCost(Sound left, Sound right) noexcept {
    return SoundReplaceCost(left, right, DefaultSoundCompatibilityTable());
}

double SoundReplaceCost(
    Sound left,
    Sound right,
    const SoundCompatibilityTable& table) noexcept {
    return table.Cost(left, right);
}

double SoundReplaceCost(
    std::optional<Sound> left,
    std::optional<Sound> right) noexcept {
    return SoundReplaceCost(left, right, DefaultSoundCompatibilityTable());
}

double SoundReplaceCost(
    std::optional<Sound> left,
    std::optional<Sound> right,
    const SoundCompatibilityTable& table) noexcept {
    return table.Cost(left, right);
}

namespace {

template <typename T>
concept Transcription = std::ranges::random_access_range<T> &&
                        std::ranges::sized_range<T> &&
                        std::convertible_to<std::ranges::range_reference_t<T>, const Sound&>;

template <typename Seq1, typename Seq2>
requires Transcription<Seq1> && Transcription<Seq2>
void CalcRhymeScoreDp(
    Seq1&& t1,
    Seq2&& t2,
    utils::MatrixSpan2d<double> dp,
    utils::MatrixSpan2d<uint8_t> anc,
    int max_shift
) {
    const int N = static_cast<int>(t1.size());
    const int M = static_cast<int>(t2.size());
    for (int i = 0; i <= N; ++i) {
        for (int j = 0; j <= M; ++j) {
            dp[i, j] = kDefaultUnknownReplaceCost;
            anc[i, j] = 0;
        }
    }
    dp[0, 0] = 0;

    for (int i = 1; i <= N; ++i) {
        const int lower = std::max(1, i - max_shift);
        const int upper = std::min(M, i + max_shift);
        for (int j = lower; j <= upper; ++j) {
            if (i >= j - max_shift && dp[i - 1, j] < dp[i, j]) {
                dp[i, j] = dp[i - 1, j];
                anc[i, j] = 1;
            }
            if (j >= i - max_shift && dp[i, j - 1] < dp[i, j]) {
                dp[i, j] = dp[i, j - 1];
                anc[i, j] = 2;
            }
            if (dp[i - 1, j - 1] < dp[i, j]) {
                dp[i, j] = dp[i - 1, j - 1];
                anc[i, j] = 3;
            }
            dp[i, j] += SoundReplaceCost(t1[i - 1], t2[j - 1]);
        }
    }
}

double CalcPrefixCost(
    int N, int M,
    utils::MatrixSpan2d<double> dp_span,
    int max_shift
) {
    double prefix_cost = 0;
    for (int i = 1; i <= std::min(N, M); ++i) {
        double minim = dp_span[i, i];
        const int lower = std::max(1, i - max_shift);
        const int upper = std::min(M, i + max_shift);
        for (int j = lower; j <= upper; ++j) {
            if (dp_span[i, j] < minim) {
                minim = dp_span[i, j];
            }
        }
        prefix_cost += minim - i;
    }
    return prefix_cost;
}

} // namespace

std::pair<double, double> CalcRhymeQualityKey(
    std::span<const Sound> t1,
    std::span<const Sound> t2,
    int max_shift
) {
    thread_local double dp_buf[kTranscriptionBufSize][kTranscriptionBufSize];
    thread_local uint8_t anc_buf[kTranscriptionBufSize][kTranscriptionBufSize];
    
    const int N = static_cast<int>(t1.size());
    const int M = static_cast<int>(t2.size());
    max_shift = std::max(max_shift, std::abs(N - M));

    std::vector<double> dp_arr;
    std::vector<uint8_t> anc_arr;

    utils::MatrixSpan2d<double> dp_span;
    utils::MatrixSpan2d<uint8_t> anc_span;

    if (N < kTranscriptionBufSize && M < kTranscriptionBufSize) {
        dp_span = utils::MatrixSpan2d<double>(dp_buf, N + 1, M + 1);
        anc_span = utils::MatrixSpan2d<uint8_t>(anc_buf, N + 1, M + 1);
    } else {
        dp_arr.resize((N + 1) * (M + 1));
        anc_arr.resize((N + 1) * (M + 1));
        dp_span = utils::MatrixSpan2d<double>(dp_arr, N + 1, M + 1, M + 1);
        anc_span = utils::MatrixSpan2d<uint8_t>(anc_arr, N + 1, M + 1, M + 1);
    }

    auto accent1 = GetAccentInTranscription(t1);
    auto accent2 = GetAccentInTranscription(t2);
    std::size_t cut_index1 = accent1.has_value() ? *accent1 : 0;
    std::size_t cut_index2 = accent2.has_value() ? *accent2 : 0;

    auto suffix1 = t1.subspan(cut_index1);
    auto suffix2 = t2.subspan(cut_index2);
    CalcRhymeScoreDp(suffix1, suffix2, dp_span, anc_span, max_shift);
    double suffix_cost = dp_span[suffix1.size(), suffix2.size()];

    auto prefix1 = t1.subspan(0, cut_index1) | std::views::reverse;
    auto prefix2 = t2.subspan(0, cut_index2) | std::views::reverse;
    CalcRhymeScoreDp(prefix1, prefix2, dp_span, anc_span, max_shift);

    double prefix_cost = CalcPrefixCost(
        static_cast<int>(prefix1.size()),
        static_cast<int>(prefix2.size()),
        dp_span, max_shift);
    return {suffix_cost, prefix_cost};
}

} // namespace ryfmach::bel
