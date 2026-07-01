#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ryfmach::bel {

enum class Letter : std::uint8_t {
    kA,
    kBe,
    kVe,
    kHe,
    kDe,
    kYe,
    kYo,
    kZhe,
    kZe,
    kI,
    kShortI,
    kKa,
    kEl,
    kEm,
    kEn,
    kO,
    kPe,
    kEr,
    kEs,
    kTe,
    kU,
    kShortU,
    kEf,
    kKha,
    kTse,
    kChe,
    kSha,
    kYery,
    kSoftSign,
    kE,
    kYu,
    kYa,

    kApostrophe,
    kDash,
};

enum class Phoneme : std::uint8_t {
    kA,
    kB,
    kBSoft,
    kV,
    kVSoft,
    kH,
    kHSoft,
    kD,
    kDSoft,
    kZh,
    kZ,
    kZSoft,
    kI,
    kJ,
    kK,
    kKSoft,
    kL,
    kLSoft,
    kM,
    kMSoft,
    kN,
    kNSoft,
    kO,
    kP,
    kPSoft,
    kR,
    kS,
    kSSoft,
    kT,
    kTSoft,
    kU,
    kW,
    kF,
    kFSoft,
    kKh,
    kKhSoft,
    kTs,
    kTsSoft,
    kCh,
    kSh,
    kY,
    kE,
};

struct ParsedLetter {
    Letter letter;
    std::size_t byte_count;
};

std::span<const Letter> Alphabet() noexcept;

std::string_view Spelling(Letter letter) noexcept;
std::string_view PhonemeSpelling(Phoneme phoneme) noexcept;

std::optional<ParsedLetter> ParseLetterPrefix(std::string_view text) noexcept;
std::optional<Letter> ParseLetter(std::string_view text) noexcept;
std::optional<std::vector<Letter>> ParseWord(std::string_view text);

bool IsAlphabetLetter(Letter letter) noexcept;
bool IsVowel(Letter letter) noexcept;
bool IsConsonant(Letter letter) noexcept;
bool IsSofteningVowel(Letter letter) noexcept;
bool IsApostrophe(Letter letter) noexcept;
bool IsDash(Letter letter) noexcept;

std::optional<std::size_t> AlphabetOrder(Letter letter) noexcept;

} // namespace ryfmach::bel
