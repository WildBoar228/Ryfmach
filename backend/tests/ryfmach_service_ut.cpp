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

TEST(RyfmachService, FindsRhymesForEveryDictionaryWordVariant) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes("хата",
        ryfmach::bel::RhymeSearchFilters{});

    ASSERT_TRUE(result.word_found);
    ASSERT_EQ(result.rhymes_list.size(), 1);
    EXPECT_EQ(result.rhymes_list[0].word_variant.id, 1);
    EXPECT_EQ(
        WordIds(result.rhymes_list[0].rhymes),
        (std::vector<int>{5, 6, 7, 8}));
}

TEST(RyfmachService, FiltersRhymesByPartOfSpeech) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes("хата", {
        .part_of_speech = {false, true, false, false, false, false, false},
    });

    ASSERT_TRUE(result.word_found);
    ASSERT_EQ(result.rhymes_list.size(), 1);
    EXPECT_EQ(WordIds(result.rhymes_list[0].rhymes), std::vector<int>{5});
}

TEST(RyfmachService, FindsRhymesForManualAccentWithoutDictionaryLookup) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes("ката", 1,
        ryfmach::bel::RhymeSearchFilters{});

    ASSERT_TRUE(result.word_found);
    ASSERT_EQ(result.rhymes_list.size(), 1);
    EXPECT_EQ(result.rhymes_list[0].word_variant.id, 0);
    EXPECT_EQ(result.rhymes_list[0].word_variant.word, "ката");
    EXPECT_EQ(result.rhymes_list[0].word_variant.accent, 1);
    EXPECT_FALSE(result.rhymes_list[0].rhymes.empty());
}

TEST(RyfmachService, RejectsInvalidInputBeforePhoneticsAndRhymes) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const ryfmach::app::RyfmachService service(slounik);

    EXPECT_FALSE(
        service.FindRhymes(
            "not Belarusian", ryfmach::bel::RhymeSearchFilters{}
        ).word_found);
    EXPECT_FALSE(
        service.AnalyzePhonetics("not Belarusian").word_found);
    EXPECT_FALSE(service.FindRhymes("хата", 0,
        ryfmach::bel::RhymeSearchFilters{}).word_found);
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
