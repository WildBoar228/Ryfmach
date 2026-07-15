#include "ryfmach_service.hpp"

#include <string>
#include <utility>

namespace ryfmach::app {
namespace {

constexpr std::size_t kMaxRhymesPerVariant = 300;
constexpr std::size_t kMaxInputWordLength = 40;

void ReplaceAll(
    std::string& text,
    std::string_view from,
    std::string_view to
) {
    for (std::size_t position = text.find(from); position != std::string::npos;
         position = text.find(from, position + to.size())) {
        text.replace(position, from.size(), to);
    }
}

std::string NormalizeInputWord(std::string_view word) {
    std::string normalized(word);
    ReplaceAll(normalized, "и", "і");
    ReplaceAll(normalized, "щ", "ў");
    ReplaceAll(normalized, "ъ", "'");
    return normalized;
}

std::optional<std::vector<bel::Letter>> ParseInputWord(
    std::string_view word
) {
    const auto letters = bel::ParseWord(word);
    if (!letters || letters->size() > kMaxInputWordLength) {
        return std::nullopt;
    }

    return letters;
}

} // namespace

RyfmachService::RyfmachService(const bel::Slounik& slounik)
    : slounik_(slounik) {}

RhymesResult RyfmachService::FindRhymes(std::string_view word) const {
    const std::string normalized_word = NormalizeInputWord(word);
    if (!ParseInputWord(normalized_word)) {
        return {};
    }

    const auto word_variants = slounik_.FindWords(
        normalized_word,
        bel::WordLookupOptions{.fix_similar_letters = true});

    RhymesResult result;
    result.word_found = !word_variants.empty();
    result.rhymes_list.reserve(word_variants.size());

    for (const auto& word_variant : word_variants) {
        result.rhymes_list.push_back(FindRhymesForVariant(word_variant));
    }

    return result;
}

RhymesResult RyfmachService::FindRhymes(
    std::string_view word,
    std::size_t accent) const {
    const std::string normalized_word = NormalizeInputWord(word);
    const auto letters = ParseInputWord(normalized_word);
    if (!letters || accent >= letters->size() || !bel::IsVowel((*letters)[accent])) {
        return {};
    }

    bel::WordRecord word_variant{
        .word = normalized_word,
        .accent = accent,
    };

    RhymesResult result;
    result.word_found = true;
    result.rhymes_list.push_back(FindRhymesForVariant(word_variant));
    return result;
}

PhoneticsResult RyfmachService::AnalyzePhonetics(std::string_view word) const {
    const std::string normalized_word = NormalizeInputWord(word);
    if (!ParseInputWord(normalized_word)) {
        return {};
    }

    const auto word_variants = slounik_.FindWords(
        normalized_word,
        bel::WordLookupOptions{.fix_similar_letters = true});

    PhoneticsResult result;
    result.word_variants.reserve(word_variants.size());
    for (const auto& word_variant : word_variants) {
        if (auto analysis = AnalyzePhoneticsForVariant(word_variant)) {
            result.word_variants.push_back(std::move(*analysis));
        }
    }
    result.word_found = !result.word_variants.empty();
    return result;
}

PhoneticsResult RyfmachService::AnalyzePhonetics(
    std::string_view word,
    std::size_t accent
) const {
    bel::WordRecord word_variant{
        .word = NormalizeInputWord(word),
        .accent = accent,
    };

    PhoneticsResult result;
    if (!ParseInputWord(word_variant.word)) {
        return result;
    }
    if (auto analysis = AnalyzePhoneticsForVariant(word_variant)) {
        result.word_found = true;
        result.word_variants.push_back(std::move(*analysis));
    }
    return result;
}

MorphemicsResult RyfmachService::AnalyzeMorphemics(std::string_view word) const {
    const std::string normalized_word = NormalizeInputWord(word);
    if (!ParseInputWord(normalized_word)) {
        return {};
    }

    bel::MorphemicAnalyzer analyzer(slounik_);
    MorphemicsResult result;
    result.variants = analyzer.Analyze(normalized_word);
    result.word_found = !result.variants.empty();
    return result;
}

RhymeGroup RyfmachService::FindRhymesForVariant(
    const bel::WordRecord& word_variant) const {
    RhymeGroup rhyme_group;
    rhyme_group.word_variant = word_variant;

    const auto rhymes = slounik_.FindRhymes(
        word_variant.word,
        word_variant.accent,
        bel::SearchMistakeLevel::kAdaptive,
        bel::RhymeSearchFilters{},
        kMaxRhymesPerVariant);
    rhyme_group.rhymes.reserve(rhymes.size());

    for (const auto& rhyme : rhymes) {
        if (const auto rhyme_word = slounik_.GetWordById(rhyme.word_id)) {
            rhyme_group.rhymes.push_back(std::move(*rhyme_word));
        }
    }

    return rhyme_group;
}

std::optional<PhoneticAnalysis> RyfmachService::AnalyzePhoneticsForVariant(
    const bel::WordRecord& word_variant
) const {
    const auto full_transcription = bel::GetTranscriptionFull(
        word_variant.word, word_variant.accent);
    if (!full_transcription) {
        return std::nullopt;
    }

    PhoneticAnalysis analysis;
    analysis.word_variant = word_variant;
    analysis.transcription = full_transcription->transcription;
    analysis.phenomena = full_transcription->phenomena;
    analysis.letter_map.reserve(full_transcription->letter_to_sounds.size());

    for (std::size_t index = 0;
         index < full_transcription->letter_to_sounds.size(); ++index) {
        const auto& sounds = full_transcription->letter_to_sounds[index];
        if (index > 0 &&
            full_transcription->letter_to_sounds[index - 1] == sounds) {
            analysis.letter_map.back().letters.push_back(index);
        } else {
            analysis.letter_map.push_back(LetterSoundMap{
                .letters = {index},
                .sounds = sounds,
            });
        }
    }

    analysis.sound_analysis.reserve(analysis.transcription.size());
    for (const bel::Sound sound : analysis.transcription) {
        analysis.sound_analysis.push_back(bel::SoundDescription(sound));
    }

    return analysis;
}

} // namespace ryfmach::app
