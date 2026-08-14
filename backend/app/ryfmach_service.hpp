#ifndef RYFMACH_APP_RYFMACH_SERVICE_HPP_
#define RYFMACH_APP_RYFMACH_SERVICE_HPP_

#include "morphemics.hpp"
#include "rhyme_likes.hpp"
#include "slounik.hpp"
#include "transcription.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ryfmach::app {

enum class RhymeResolutionStatus {
    kResolved,
    kNeedsChoice,
    kNotFound,
};

struct RhymeWordVariant {
    bel::WordRecord dictionary_entry;
    bool exact_match = false;
};

struct RhymesResult {
    RhymeResolutionStatus status = RhymeResolutionStatus::kNotFound;
    std::vector<RhymeWordVariant> variants;
    std::optional<RhymeWordVariant> selected_variant;
    std::vector<bel::WordRecord> rhymes;
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
    explicit RyfmachService(
        const bel::Slounik& slounik,
        std::unique_ptr<bel::RhymeLikes> rhyme_likes = nullptr);

    RhymesResult FindRhymes(
        std::string_view word,
        const bel::RhymeSearchFilters&) const;
    RhymesResult FindRhymes(
        std::string_view word,
        std::size_t accent,
        const bel::RhymeSearchFilters&) const;
    RhymesResult FindRhymes(
        std::string_view word,
        std::size_t accent,
        int dictionary_id,
        const bel::RhymeSearchFilters&) const;
    PhoneticsResult AnalyzePhonetics(std::string_view word) const;
    PhoneticsResult AnalyzePhonetics(
        std::string_view word,
        std::size_t accent) const;
    MorphemicsResult AnalyzeMorphemics(std::string_view word) const;
    int UpdateRhymeLikeScore(
        std::string_view request_word,
        int request_stress,
        std::string_view rhyme_word,
        int rhyme_stress,
        int delta);

private:
    std::vector<bel::WordRecord> FindRhymesForVariant(
        const bel::WordRecord& word_variant,
        const bel::RhymeSearchFilters&) const;
    std::optional<PhoneticAnalysis> AnalyzePhoneticsForVariant(
        const bel::WordRecord& word_variant) const;

    const bel::Slounik& slounik_;
    std::unique_ptr<bel::RhymeLikes> rhyme_likes_;
};

} // namespace ryfmach::app

#endif // RYFMACH_APP_RYFMACH_SERVICE_HPP_
