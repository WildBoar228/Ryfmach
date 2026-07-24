#include "bel_lang_engine/test_utils.hpp"
#include "rhyme_likes.hpp"
#include "rhymes.hpp"
#include "ryfmach_service.hpp"
#include "sounds.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace {
using ryfmach::tests::JoinTranscription;
using ryfmach::tests::SharedSlounikTestDatabasePath;
using ryfmach::tests::WordIds;
using ryfmach::app::RhymeResolutionStatus;

TEST(RyfmachService, ResolvesOneExactPronunciationAndFindsRhymes) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes("хата",
        ryfmach::bel::RhymeSearchFilters{});

    ASSERT_EQ(result.status, RhymeResolutionStatus::kResolved);
    ASSERT_TRUE(result.selected_variant.has_value());
    EXPECT_EQ(result.selected_variant->word, "хата");
    EXPECT_EQ(result.selected_variant->accent, 1);
    ASSERT_EQ(result.selected_variant->dictionary_entries.size(), 1);
    EXPECT_EQ(result.selected_variant->dictionary_entries[0].id, 1);
    EXPECT_EQ(WordIds(result.rhymes), (std::vector<int>{5, 6, 7, 8}));
}

TEST(RyfmachService, GroupsDictionaryEntriesByWordAndAccent) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes(
        "ласка", ryfmach::bel::RhymeSearchFilters{});

    ASSERT_EQ(result.status, RhymeResolutionStatus::kResolved);
    ASSERT_TRUE(result.selected_variant.has_value());
    EXPECT_EQ(result.selected_variant->word, "ласка");
    EXPECT_EQ(result.selected_variant->accent, 1);
    EXPECT_EQ(
        WordIds(result.selected_variant->dictionary_entries),
        (std::vector<int>{11, 12}));
}

TEST(RyfmachService, RequiresChoiceForMultipleExactStressVariants) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes(
        "замкі", ryfmach::bel::RhymeSearchFilters{});

    ASSERT_EQ(result.status, RhymeResolutionStatus::kNeedsChoice);
    ASSERT_EQ(result.variants.size(), 2);
    EXPECT_EQ(result.variants[0].word, "замкі");
    EXPECT_EQ(result.variants[0].accent, 1);
    EXPECT_TRUE(result.variants[0].exact_match);
    EXPECT_EQ(result.variants[1].word, "замкі");
    EXPECT_EQ(result.variants[1].accent, 4);
    EXPECT_TRUE(result.variants[1].exact_match);
    EXPECT_FALSE(result.selected_variant.has_value());
    EXPECT_TRUE(result.rhymes.empty());
}

TEST(RyfmachService, OrdersExactPronunciationsBeforeCorrections) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes(
        "мўка", ryfmach::bel::RhymeSearchFilters{});

    ASSERT_EQ(result.status, RhymeResolutionStatus::kNeedsChoice);
    ASSERT_EQ(result.variants.size(), 4);
    EXPECT_EQ(result.variants[0].word, "мўка");
    EXPECT_TRUE(result.variants[0].exact_match);
    EXPECT_EQ(result.variants[1].word, "мўка");
    EXPECT_TRUE(result.variants[1].exact_match);
    EXPECT_EQ(result.variants[2].word, "мука");
    EXPECT_FALSE(result.variants[2].exact_match);
    EXPECT_EQ(result.variants[3].word, "мука");
    EXPECT_FALSE(result.variants[3].exact_match);
    EXPECT_TRUE(result.rhymes.empty());
}

TEST(RyfmachService, RequiresConfirmationForCorrectedOnlyPronunciation) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes(
        "вёрас", ryfmach::bel::RhymeSearchFilters{});

    ASSERT_EQ(result.status, RhymeResolutionStatus::kNeedsChoice);
    ASSERT_EQ(result.variants.size(), 1);
    EXPECT_EQ(result.variants[0].word, "верас");
    EXPECT_FALSE(result.variants[0].exact_match);
    EXPECT_TRUE(result.rhymes.empty());
}

TEST(RyfmachService, PrefersOneExactPronunciationOverCorrection) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes(
        "сена", ryfmach::bel::RhymeSearchFilters{});

    ASSERT_EQ(result.status, RhymeResolutionStatus::kResolved);
    ASSERT_TRUE(result.selected_variant.has_value());
    EXPECT_EQ(result.selected_variant->word, "сена");
    EXPECT_TRUE(result.selected_variant->exact_match);
    EXPECT_EQ(
        WordIds(result.selected_variant->dictionary_entries),
        std::vector<int>{19});
    EXPECT_TRUE(result.variants.empty());
}

TEST(RyfmachService, ReturnsNotFoundForUnknownWord) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes(
        "абракадабра", ryfmach::bel::RhymeSearchFilters{});

    EXPECT_EQ(result.status, RhymeResolutionStatus::kNotFound);
    EXPECT_TRUE(result.variants.empty());
    EXPECT_FALSE(result.selected_variant.has_value());
    EXPECT_TRUE(result.rhymes.empty());
}

TEST(RyfmachService, FiltersRhymesByPartOfSpeech) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes("хата", {
        .part_of_speech = {false, true, false, false, false, false, false},
    });

    ASSERT_EQ(result.status, RhymeResolutionStatus::kResolved);
    EXPECT_EQ(WordIds(result.rhymes), std::vector<int>{5});
}

TEST(RyfmachService, FindsRhymesForManualAccentWithoutDictionaryLookup) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes("ката", 1,
        ryfmach::bel::RhymeSearchFilters{});

    ASSERT_EQ(result.status, RhymeResolutionStatus::kResolved);
    ASSERT_TRUE(result.selected_variant.has_value());
    EXPECT_EQ(result.selected_variant->word, "ката");
    EXPECT_EQ(result.selected_variant->accent, 1);
    EXPECT_TRUE(result.selected_variant->exact_match);
    EXPECT_TRUE(result.selected_variant->dictionary_entries.empty());
    EXPECT_FALSE(result.rhymes.empty());
}

TEST(RyfmachService, RejectsInvalidInputBeforePhoneticsAndRhymes) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    EXPECT_EQ(
        service.FindRhymes(
            "not Belarusian", ryfmach::bel::RhymeSearchFilters{}
        ).status,
        RhymeResolutionStatus::kNotFound);
    EXPECT_FALSE(
        service.AnalyzePhonetics("not Belarusian").word_found);
    EXPECT_EQ(
        service.FindRhymes(
            "хата", 0, ryfmach::bel::RhymeSearchFilters{}).status,
        RhymeResolutionStatus::kNotFound);
}

TEST(RyfmachService, AnalyzesDictionaryWordPhonetics) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.AnalyzePhonetics("хата");

    ASSERT_TRUE(result.word_found);
    ASSERT_EQ(result.word_variants.size(), 1);
    const auto& analysis = result.word_variants[0];
    EXPECT_EQ(analysis.word_variant.id, 1);
    EXPECT_EQ(JoinTranscription(analysis.transcription), "х _а_ т а");
    EXPECT_EQ(
        analysis.sound_analysis,
        (std::vector<std::string>{
            "зычны, глухі парны [г], цвёрды парны [х']",
            "галосны, націскны",
            "зычны, глухі парны [д], цвёрды парны [ц']",
            "галосны, ненаціскны",
        }));
}

TEST(RyfmachService, AnalyzesManualAccentPhonetics) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.AnalyzePhonetics("ксёндз", 2);

    ASSERT_TRUE(result.word_found);
    ASSERT_EQ(result.word_variants.size(), 1);
    const auto& analysis = result.word_variants[0];
    EXPECT_EQ(analysis.word_variant.id, 0);
    EXPECT_EQ(JoinTranscription(analysis.transcription), "к с' _о_ н ц");
    EXPECT_EQ(
        analysis.letter_map[4].letters,
        (std::vector<std::size_t>{4, 5}));
    ASSERT_EQ(analysis.phenomena.size(), 3);
    EXPECT_EQ(
        analysis.phenomena[2].phenomenon,
        ryfmach::bel::PhoneticPhenomenon::kThudAssimilation);
}

TEST(RyfmachService, AnalyzesMorphemics) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.AnalyzeMorphemics("хата");

    ASSERT_TRUE(result.word_found);
    ASSERT_EQ(result.variants.size(), 1);
    EXPECT_TRUE(result.variants[0].sure);
    EXPECT_EQ(
        result.variants[0].analysis,
        (std::vector<ryfmach::bel::Morpheme>{
            {.type = ryfmach::bel::MorphemeType::kRoot, .text = "хат"},
            {.type = ryfmach::bel::MorphemeType::kEnding, .text = "а"},
        }));
}

TEST(RyfmachService, UpdatesOneRhymeLikeScoreForEitherPairOrder) {
    const auto path = std::filesystem::temp_directory_path() /
                      "ryfmach_rhyme_likes_test.sqlite";
    std::filesystem::remove(path);
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    ryfmach::app::RyfmachService service(
        slounik, std::make_unique<ryfmach::bel::RhymeLikes>(path));

    EXPECT_EQ(service.UpdateRhymeLikeScore("alpha", 1, "beta", 2, 1), 1);
    EXPECT_EQ(service.UpdateRhymeLikeScore("beta", 2, "alpha", 1, -1), 0);
    EXPECT_EQ(service.UpdateRhymeLikeScore("alpha", 1, "beta", 2, 1), 1);

    SQLite::Database db(path, SQLite::OPEN_READONLY);
    SQLite::Statement count(db, "SELECT COUNT(*) FROM rhyme_likes");
    ASSERT_TRUE(count.executeStep());
    EXPECT_EQ(count.getColumn(0).getInt(), 1);
}

} // namespace
