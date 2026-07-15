#ifndef RYFMACH_BEL_LANG_ENGINE_INCLUDE_MORPHEMICS_HPP_
#define RYFMACH_BEL_LANG_ENGINE_INCLUDE_MORPHEMICS_HPP_

#include "slounik.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ryfmach::bel {

enum class MorphemeType : std::uint8_t {
    kUnknown,
    kPrefix,
    kRoot,
    kSuffix,
    kEnding,
};

struct Morpheme {
    MorphemeType type = MorphemeType::kUnknown;
    std::string text;

    friend bool operator==(const Morpheme&, const Morpheme&) = default;
};

struct MorphemicAnalysis {
    std::vector<Morpheme> analysis;
    bool sure = false;

    friend bool operator==(const MorphemicAnalysis&, const MorphemicAnalysis&) =
        default;
};

std::vector<Morpheme> DecodeMorphemicAnalysis(std::string_view encoded);
std::string EncodeMorphemicAnalysis(std::span<const Morpheme> analysis);

class MorphemicAnalyzer {
public:
    explicit MorphemicAnalyzer(const Slounik& slounik);

    std::vector<MorphemicAnalysis> Analyze(
        std::string_view word,
        bool fix_similar_letters = true) const;

private:
    const Slounik& slounik_;
};

} // namespace ryfmach::bel

#endif // RYFMACH_BEL_LANG_ENGINE_INCLUDE_MORPHEMICS_HPP_
