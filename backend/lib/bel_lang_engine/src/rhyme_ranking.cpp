#include "rhyme_ranking.hpp"
#include "rhymes.hpp"
#include "transcription.hpp"

namespace ryfmach::bel {

    constexpr int kDefaultMaxShift = 5;

    std::optional<Rhyme> RhymeFromWordRecord(const WordRecord& rec) {
        auto letters = ParseWord(rec.word);
        if (!letters) {
            return std::nullopt;
        }

        Rhyme rhyme;
        rhyme.word_id = rec.id;
        rhyme.letters = std::move(*letters);
        rhyme.initial_id = rec.initial_id;
        rhyme.part_of_speech_id = rec.part_of_speech_id;
        rhyme.stress = rec.accent;
        rhyme.is_initial = rec.is_initial;
        return rhyme;
    }

    std::vector<Rhyme> WordsToRhymesWithScore(
        const WordRecord& target,
        std::vector<WordRecord>&& words
    ) {
        auto target_letters = ParseWord(target.word);
        if (!target_letters) {
            throw std::runtime_error("Invalid rhyme target");
        }
        auto target_transript = GetTranscriptionSounds(*target_letters, target.accent);
        if (!target_transript) {
            throw std::runtime_error("Invalid rhyme target");
        }

        std::vector<Rhyme> rhymes;
        rhymes.reserve(words.size());
        for (auto&& rec : words) {
            auto rhyme = RhymeFromWordRecord(rec);
            if (!rhyme) { continue; }

            auto rhyme_transcript = GetTranscriptionSounds(rhyme->letters, rhyme->stress);
            if (!rhyme_transcript) { continue; }

            rhyme->score = CalcRhymeCost(
                *target_transript,
                *rhyme_transcript,
                kDefaultMaxShift);
            rhymes.push_back(std::move(*rhyme));
        }
        
        return rhymes;
    }

    void SortRhymes(std::vector<Rhyme>& rhymes) {
        std::sort(rhymes.begin(), rhymes.end(),
            [](const Rhyme& lhs, const Rhyme& rhs) {
                return std::tie(lhs.score, lhs.penalty) <
                       std::tie(rhs.score, rhs.penalty);
            });
    }
    
} // namespace ryfmach::bel
