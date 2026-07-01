#include "language.hpp"

#include <array>

namespace ryfmach::bel {
namespace {

constexpr std::array<Letter, 32> kAlphabet = {
    Letter::kA,
    Letter::kBe,
    Letter::kVe,
    Letter::kHe,
    Letter::kDe,
    Letter::kYe,
    Letter::kYo,
    Letter::kZhe,
    Letter::kZe,
    Letter::kI,
    Letter::kShortI,
    Letter::kKa,
    Letter::kEl,
    Letter::kEm,
    Letter::kEn,
    Letter::kO,
    Letter::kPe,
    Letter::kEr,
    Letter::kEs,
    Letter::kTe,
    Letter::kU,
    Letter::kShortU,
    Letter::kEf,
    Letter::kKha,
    Letter::kTse,
    Letter::kChe,
    Letter::kSha,
    Letter::kYery,
    Letter::kSoftSign,
    Letter::kE,
    Letter::kYu,
    Letter::kYa,
};

struct LetterToken {
    std::string_view text;
    Letter letter;
};

constexpr std::array<LetterToken, 73> kLetterTokens = {{
    {"а", Letter::kA},
    {"А", Letter::kA},
    {"б", Letter::kBe},
    {"Б", Letter::kBe},
    {"в", Letter::kVe},
    {"В", Letter::kVe},
    {"г", Letter::kHe},
    {"Г", Letter::kHe},
    {"д", Letter::kDe},
    {"Д", Letter::kDe},
    {"е", Letter::kYe},
    {"Е", Letter::kYe},
    {"ё", Letter::kYo},
    {"Ё", Letter::kYo},
    {"ж", Letter::kZhe},
    {"Ж", Letter::kZhe},
    {"з", Letter::kZe},
    {"З", Letter::kZe},
    {"і", Letter::kI},
    {"І", Letter::kI},
    {"й", Letter::kShortI},
    {"Й", Letter::kShortI},
    {"к", Letter::kKa},
    {"К", Letter::kKa},
    {"л", Letter::kEl},
    {"Л", Letter::kEl},
    {"м", Letter::kEm},
    {"М", Letter::kEm},
    {"н", Letter::kEn},
    {"Н", Letter::kEn},
    {"о", Letter::kO},
    {"О", Letter::kO},
    {"п", Letter::kPe},
    {"П", Letter::kPe},
    {"р", Letter::kEr},
    {"Р", Letter::kEr},
    {"с", Letter::kEs},
    {"С", Letter::kEs},
    {"т", Letter::kTe},
    {"Т", Letter::kTe},
    {"у", Letter::kU},
    {"У", Letter::kU},
    {"ў", Letter::kShortU},
    {"Ў", Letter::kShortU},
    {"ф", Letter::kEf},
    {"Ф", Letter::kEf},
    {"х", Letter::kKha},
    {"Х", Letter::kKha},
    {"ц", Letter::kTse},
    {"Ц", Letter::kTse},
    {"ч", Letter::kChe},
    {"Ч", Letter::kChe},
    {"ш", Letter::kSha},
    {"Ш", Letter::kSha},
    {"ы", Letter::kYery},
    {"Ы", Letter::kYery},
    {"ь", Letter::kSoftSign},
    {"Ь", Letter::kSoftSign},
    {"э", Letter::kE},
    {"Э", Letter::kE},
    {"ю", Letter::kYu},
    {"Ю", Letter::kYu},
    {"я", Letter::kYa},
    {"Я", Letter::kYa},
    {"'", Letter::kApostrophe},
    {"’", Letter::kApostrophe},
    {"ʼ", Letter::kApostrophe},
    {"`", Letter::kApostrophe},
    {"‘", Letter::kApostrophe},
    {"-", Letter::kDash},
    {"‐", Letter::kDash},
    {"‑", Letter::kDash},
    {"–", Letter::kDash},
}};

} // namespace

std::span<const Letter> Alphabet() noexcept {
    return kAlphabet;
}

std::string_view Spelling(Letter letter) noexcept {
    switch (letter) {
        case Letter::kA:
            return "а";
        case Letter::kBe:
            return "б";
        case Letter::kVe:
            return "в";
        case Letter::kHe:
            return "г";
        case Letter::kDe:
            return "д";
        case Letter::kYe:
            return "е";
        case Letter::kYo:
            return "ё";
        case Letter::kZhe:
            return "ж";
        case Letter::kZe:
            return "з";
        case Letter::kI:
            return "і";
        case Letter::kShortI:
            return "й";
        case Letter::kKa:
            return "к";
        case Letter::kEl:
            return "л";
        case Letter::kEm:
            return "м";
        case Letter::kEn:
            return "н";
        case Letter::kO:
            return "о";
        case Letter::kPe:
            return "п";
        case Letter::kEr:
            return "р";
        case Letter::kEs:
            return "с";
        case Letter::kTe:
            return "т";
        case Letter::kU:
            return "у";
        case Letter::kShortU:
            return "ў";
        case Letter::kEf:
            return "ф";
        case Letter::kKha:
            return "х";
        case Letter::kTse:
            return "ц";
        case Letter::kChe:
            return "ч";
        case Letter::kSha:
            return "ш";
        case Letter::kYery:
            return "ы";
        case Letter::kSoftSign:
            return "ь";
        case Letter::kE:
            return "э";
        case Letter::kYu:
            return "ю";
        case Letter::kYa:
            return "я";
        case Letter::kApostrophe:
            return "'";
        case Letter::kDash:
            return "-";
    }

    return "";
}

std::string_view PhonemeSpelling(Phoneme phoneme) noexcept {
    switch (phoneme) {
        case Phoneme::kA:
            return "а";
        case Phoneme::kB:
            return "б";
        case Phoneme::kBSoft:
            return "б'";
        case Phoneme::kV:
            return "в";
        case Phoneme::kVSoft:
            return "в'";
        case Phoneme::kH:
            return "г";
        case Phoneme::kHSoft:
            return "г'";
        case Phoneme::kG:
            return "г*";
        case Phoneme::kGSoft:
            return "г*'";
        case Phoneme::kD:
            return "д";
        case Phoneme::kDz:
            return "дз";
        case Phoneme::kDzSoft:
            return "дз'";
        case Phoneme::kDzh:
            return "дж";
        case Phoneme::kZh:
            return "ж";
        case Phoneme::kZ:
            return "з";
        case Phoneme::kZSoft:
            return "з'";
        case Phoneme::kI:
            return "і";
        case Phoneme::kJ:
            return "й";
        case Phoneme::kK:
            return "к";
        case Phoneme::kKSoft:
            return "к'";
        case Phoneme::kL:
            return "л";
        case Phoneme::kLSoft:
            return "л'";
        case Phoneme::kM:
            return "м";
        case Phoneme::kMSoft:
            return "м'";
        case Phoneme::kN:
            return "н";
        case Phoneme::kNSoft:
            return "н'";
        case Phoneme::kO:
            return "о";
        case Phoneme::kP:
            return "п";
        case Phoneme::kPSoft:
            return "п'";
        case Phoneme::kR:
            return "р";
        case Phoneme::kS:
            return "с";
        case Phoneme::kSSoft:
            return "с'";
        case Phoneme::kT:
            return "т";
        case Phoneme::kU:
            return "у";
        case Phoneme::kW:
            return "ў";
        case Phoneme::kF:
            return "ф";
        case Phoneme::kFSoft:
            return "ф'";
        case Phoneme::kKh:
            return "х";
        case Phoneme::kKhSoft:
            return "х'";
        case Phoneme::kTs:
            return "ц";
        case Phoneme::kTsSoft:
            return "ц'";
        case Phoneme::kCh:
            return "ч";
        case Phoneme::kSh:
            return "ш";
        case Phoneme::kY:
            return "ы";
        case Phoneme::kE:
            return "э";
    }

    return "";
}

std::optional<ParsedLetter> ParseLetterPrefix(std::string_view text) noexcept {
    for (const auto& token : kLetterTokens) {
        if (text.starts_with(token.text)) {
            return ParsedLetter{
                .letter = token.letter,
                .byte_count = token.text.size(),
            };
        }
    }

    return std::nullopt;
}

std::optional<Letter> ParseLetter(std::string_view text) noexcept {
    const auto parsed = ParseLetterPrefix(text);
    if (!parsed || parsed->byte_count != text.size()) {
        return std::nullopt;
    }

    return parsed->letter;
}

std::optional<std::vector<Letter>> ParseWord(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }

    std::vector<Letter> letters;
    bool previous_is_dash = false;

    while (!text.empty()) {
        const auto parsed = ParseLetterPrefix(text);
        if (!parsed) {
            return std::nullopt;
        }

        const bool current_is_dash = IsDash(parsed->letter);
        if (current_is_dash && (letters.empty() || previous_is_dash)) {
            return std::nullopt;
        }

        letters.push_back(parsed->letter);
        previous_is_dash = current_is_dash;
        text.remove_prefix(parsed->byte_count);
    }

    if (previous_is_dash) {
        return std::nullopt;
    }

    return letters;
}

bool IsAlphabetLetter(Letter letter) noexcept {
    return letter != Letter::kApostrophe && letter != Letter::kDash;
}

bool IsVowel(Letter letter) noexcept {
    switch (letter) {
        case Letter::kA:
        case Letter::kYe:
        case Letter::kYo:
        case Letter::kI:
        case Letter::kO:
        case Letter::kU:
        case Letter::kYery:
        case Letter::kE:
        case Letter::kYu:
        case Letter::kYa:
            return true;
        default:
            return false;
    }
}

bool IsConsonant(Letter letter) noexcept {
    return IsAlphabetLetter(letter) && !IsVowel(letter) &&
        letter != Letter::kSoftSign;
}

bool IsSofteningVowel(Letter letter) noexcept {
    switch (letter) {
        case Letter::kYe:
        case Letter::kYo:
        case Letter::kI:
        case Letter::kYu:
        case Letter::kYa:
            return true;
        default:
            return false;
    }
}

bool IsApostrophe(Letter letter) noexcept {
    return letter == Letter::kApostrophe;
}

bool IsDash(Letter letter) noexcept {
    return letter == Letter::kDash;
}

std::optional<std::size_t> AlphabetOrder(Letter letter) noexcept {
    for (std::size_t index = 0; index < kAlphabet.size(); ++index) {
        if (kAlphabet[index] == letter) {
            return index;
        }
    }

    return std::nullopt;
}

} // namespace ryfmach::bel
