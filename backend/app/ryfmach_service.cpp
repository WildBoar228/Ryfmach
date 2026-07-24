#include "ryfmach_service.hpp"

#include <algorithm>
#include <span>
#include <stdexcept>
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

std::vector<RhymeWordVariant> BuildRhymeWordVariants(
    std::string_view normalized_input,
    std::span<const bel::WordRecord> word_variants
) {
    std::vector<RhymeWordVariant> variants;
    variants.reserve(word_variants.size());

    for (const auto& word_variant : word_variants) {
        variants.push_back(RhymeWordVariant{
            .dictionary_entry = word_variant,
            .exact_match = NormalizeInputWord(word_variant.word) ==
                           normalized_input,
        });
    }

    std::stable_partition(
        variants.begin(), variants.end(),
        [](const auto& variant) {
            return variant.exact_match;
        });
    return variants;
}

} // namespace

RyfmachService::RyfmachService(
    const bel::Slounik& slounik,
    std::unique_ptr<bel::RhymeLikes> rhyme_likes)
    : slounik_(slounik), rhyme_likes_(std::move(rhyme_likes)) {}

RhymesResult RyfmachService::FindRhymes(
    std::string_view word,
    const bel::RhymeSearchFilters& filters
) const {
    const std::string normalized_word = NormalizeInputWord(word);
    if (!ParseInputWord(normalized_word)) {
        return {};
    }

    const auto word_variants = slounik_.FindWords(
        normalized_word,
        bel::WordLookupOptions{.fix_similar_letters = true});

    auto variants = BuildRhymeWordVariants(
        normalized_word, word_variants);
    const auto exact_variant_count = std::count_if(
        variants.begin(), variants.end(),
        [](const auto& variant) {
            return variant.exact_match;
        });

    RhymesResult result;
    if (variants.empty()) {
        return result;
    }
    if (exact_variant_count != 1) {
        result.status = RhymeResolutionStatus::kNeedsChoice;
        result.variants = std::move(variants);
        return result;
    }

    result.status = RhymeResolutionStatus::kResolved;
    result.selected_variant = std::move(variants.front());
    result.rhymes = FindRhymesForVariant(
        result.selected_variant->dictionary_entry, filters);
    return result;
}

RhymesResult RyfmachService::FindRhymes(
    std::string_view word,
    std::size_t accent,
    const bel::RhymeSearchFilters& filters
) const {
    const std::string normalized_word = NormalizeInputWord(word);
    const auto letters = ParseInputWord(normalized_word);
    if (!letters || accent >= letters->size() || !bel::IsVowel((*letters)[accent])) {
        return {};
    }

    RhymeWordVariant word_variant{
        .dictionary_entry = bel::WordRecord{
            .word = normalized_word,
            .accent = accent,
        },
        .exact_match = true,
    };

    RhymesResult result;
    result.status = RhymeResolutionStatus::kResolved;
    result.selected_variant = std::move(word_variant);
    result.rhymes = FindRhymesForVariant(
        result.selected_variant->dictionary_entry, filters);
    return result;
}

RhymesResult RyfmachService::FindRhymes(
    std::string_view word,
    std::size_t accent,
    int dictionary_id,
    const bel::RhymeSearchFilters& filters
) const {
    const std::string normalized_word = NormalizeInputWord(word);
    const auto letters = ParseInputWord(normalized_word);
    if (!letters || accent >= letters->size() ||
        !bel::IsVowel((*letters)[accent]) || dictionary_id <= 0) {
        return {};
    }

    const auto word_variants = slounik_.FindWords(normalized_word);
    const auto selected = std::find_if(
        word_variants.begin(), word_variants.end(),
        [dictionary_id, accent, &normalized_word](const auto& candidate) {
            return candidate.id == dictionary_id &&
                   candidate.word == normalized_word &&
                   candidate.accent == accent;
        });
    if (selected == word_variants.end()) {
        return {};
    }

    RhymesResult result;
    result.status = RhymeResolutionStatus::kResolved;
    result.selected_variant = RhymeWordVariant{
        .dictionary_entry = *selected,
        .exact_match = true,
    };
    result.rhymes = FindRhymesForVariant(*selected, filters);
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

int RyfmachService::UpdateRhymeLikeScore(
    std::string_view request_word,
    int request_stress,
    std::string_view rhyme_word,
    int rhyme_stress,
    int delta) {
    if (!rhyme_likes_) {
        throw std::logic_error("rhyme likes are not configured");
    }
    return rhyme_likes_->UpdateScore(
        request_word, request_stress, rhyme_word, rhyme_stress, delta);
}

std::vector<bel::WordRecord> RyfmachService::FindRhymesForVariant(
    const bel::WordRecord& word_variant,
    const bel::RhymeSearchFilters& filters
) const {
    const auto rhymes = slounik_.FindRhymes(
        word_variant,
        filters,
        kMaxRhymesPerVariant);

    std::vector<bel::WordRecord> rhyme_words;
    rhyme_words.reserve(rhymes.size());

    for (const auto& rhyme : rhymes) {
        if (const auto rhyme_word = slounik_.GetWordById(rhyme.word_id)) {
            rhyme_words.push_back(std::move(*rhyme_word));
        }
    }

    return rhyme_words;
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
