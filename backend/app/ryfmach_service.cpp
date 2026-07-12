#include "ryfmach_service.hpp"

#include <utility>

namespace ryfmach::app {
namespace {

constexpr std::size_t kMaxRhymesPerVariant = 300;

} // namespace

RyfmachService::RyfmachService(const bel::Slounik& slounik)
    : slounik_(slounik) {}

RhymesResult RyfmachService::FindRhymes(std::string_view word) const {
    const auto word_variants = slounik_.FindWords(
        word,
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
    bel::WordRecord word_variant{
        .word = std::string(word),
        .accent = accent,
    };

    RhymesResult result;
    result.word_found = true;
    result.rhymes_list.push_back(FindRhymesForVariant(word_variant));
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

} // namespace ryfmach::app
