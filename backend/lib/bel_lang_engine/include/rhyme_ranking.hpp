#ifndef RYFMACH_BEL_LANG_ENGINE_INCLUDE_RHYME_RANKING_HPP_
#define RYFMACH_BEL_LANG_ENGINE_INCLUDE_RHYME_RANKING_HPP_

#include "language.hpp"
#include "rhymes.hpp"
#include "slounik.hpp"

#include <optional>
#include <vector>

namespace ryfmach::bel {

struct WordRecord;

struct Rhyme {
    int word_id = 0;
    std::vector<Letter> letters;
    int initial_id = 0;
    int part_of_speech_id = 0;
    std::size_t stress = 0;
    bool is_initial = false;

    RhymeCostType cost{};
    double penalty = 0;
};

std::optional<Rhyme> RhymeFromWordRecord(const WordRecord&);

std::vector<Rhyme> WordsToRhymesWithScore(
    const WordRecord& target,
    std::vector<WordRecord>&& words);

void SortRhymes(std::vector<Rhyme>&);

} // namespace ryfmach::bel

#endif // RYFMACH_BEL_LANG_ENGINE_INCLUDE_RHYME_RANKING_HPP_
