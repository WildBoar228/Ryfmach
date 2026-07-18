#include "test_utils.hpp"

namespace {
using ryfmach::tests::SharedSlounikTestDatabasePath;
TEST(Morphemics, DecodesAndEncodesStoredAnalysis) {
    const auto analysis = ryfmach::bel::DecodeMorphemicAnalysis(
        "|пры|-(ход)-<н>-[]");

    EXPECT_EQ(
        analysis,
        (std::vector<ryfmach::bel::Morpheme>{
            {.type = ryfmach::bel::MorphemeType::kPrefix, .text = "пры"},
            {.type = ryfmach::bel::MorphemeType::kRoot, .text = "ход"},
            {.type = ryfmach::bel::MorphemeType::kSuffix, .text = "н"},
            {.type = ryfmach::bel::MorphemeType::kEnding, .text = ""},
        }));
    EXPECT_EQ(
        ryfmach::bel::EncodeMorphemicAnalysis(analysis),
        "|пры|-(ход)-<н>-[]");
}

TEST(Morphemics, UsesStoredAnalysesAndPrefixPredictions) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::bel::MorphemicAnalyzer analyzer(slounik);

    EXPECT_EQ(
        analyzer.Analyze("хата"),
        (std::vector<ryfmach::bel::MorphemicAnalysis>{
            {
                .analysis = {
                    {.type = ryfmach::bel::MorphemeType::kRoot, .text = "хат"},
                    {.type = ryfmach::bel::MorphemeType::kEnding, .text = "а"},
                },
                .sure = true,
            },
        }));
    EXPECT_EQ(
        analyzer.Analyze("прыход"),
        (std::vector<ryfmach::bel::MorphemicAnalysis>{
            {
                .analysis = {
                    {.type = ryfmach::bel::MorphemeType::kPrefix, .text = "пры"},
                    {.type = ryfmach::bel::MorphemeType::kRoot, .text = "ход"},
                    {.type = ryfmach::bel::MorphemeType::kEnding, .text = ""},
                },
                .sure = true,
            },
        }));
}

TEST(Morphemics, BuildsSureAndFallbackPredictions) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::bel::MorphemicAnalyzer analyzer(slounik);

    EXPECT_EQ(
        analyzer.Analyze("мыцца"),
        (std::vector<ryfmach::bel::MorphemicAnalysis>{
            {
                .analysis = {
                    {.type = ryfmach::bel::MorphemeType::kRoot, .text = "мы"},
                    {.type = ryfmach::bel::MorphemeType::kSuffix, .text = "ц"},
                    {.type = ryfmach::bel::MorphemeType::kSuffix, .text = "ца"},
                },
                .sure = true,
            },
        }));
    EXPECT_EQ(
        analyzer.Analyze("гуляць"),
        (std::vector<ryfmach::bel::MorphemicAnalysis>{
            {
                .analysis = {
                    {.type = ryfmach::bel::MorphemeType::kUnknown, .text = "гуля"},
                    {.type = ryfmach::bel::MorphemeType::kSuffix, .text = "ць"},
                },
                .sure = false,
            },
        }));
}


} // namespace
