#ifndef RYFMACH_APP_RYFMACH_SERVICE_HPP_
#define RYFMACH_APP_RYFMACH_SERVICE_HPP_

#include "morphemics.hpp"
#include "slounik.hpp"
#include "transcription.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ryfmach::app {

struct RhymeGroup {
    bel::WordRecord word_variant;
    std::vector<bel::WordRecord> rhymes;
};

struct RhymesResult {
    bool word_found = false;
    std::vector<RhymeGroup> rhymes_list;
};

struct LetterSoundMap {
    std::vector<std::size_t> letters;
    std::vector<std::size_t> sounds;
};

struct PhoneticAnalysis {
    bel::WordRecord word_variant;
    std::vector<LetterSoundMap> letter_map;
    std::vector<bel::Sound> transcription;
    std::vector<bel::PhoneticPhenomenonOccurrence> phenomena;
    std::vector<std::string> sound_analysis;
};

struct PhoneticsResult {
    bool word_found = false;
    std::vector<PhoneticAnalysis> word_variants;
};

struct MorphemicsResult {
    bool word_found = false;
    std::vector<bel::MorphemicAnalysis> variants;
};

class RyfmachService {
public:
    explicit RyfmachService(const bel::Slounik& slounik);

    RhymesResult FindRhymes(
        std::string_view word,
        const bel::RhymeSearchFilters&) const;
    RhymesResult FindRhymes(
        std::string_view word,
        std::size_t accent,
        const bel::RhymeSearchFilters&) const;
    PhoneticsResult AnalyzePhonetics(std::string_view word) const;
    PhoneticsResult AnalyzePhonetics(
        std::string_view word,
        std::size_t accent) const;
    MorphemicsResult AnalyzeMorphemics(std::string_view word) const;

private:
    RhymeGroup FindRhymesForVariant(
        const bel::WordRecord& word_variant,
        const bel::RhymeSearchFilters&) const;
    std::optional<PhoneticAnalysis> AnalyzePhoneticsForVariant(
        const bel::WordRecord& word_variant) const;

    const bel::Slounik& slounik_;
};

} // namespace ryfmach::app

#endif // RYFMACH_APP_RYFMACH_SERVICE_HPP_
