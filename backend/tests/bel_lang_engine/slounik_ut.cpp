#include "test_utils.hpp"

namespace {
using ryfmach::tests::RhymeWordIds;
using ryfmach::tests::SharedSlounikTestDatabasePath;
TEST(Slounik, FindsExactWords) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());

    const auto words = slounik.FindWords("хаце");

    ASSERT_EQ(words.size(), 1);
    EXPECT_EQ(words[0].id, 2);
    EXPECT_EQ(words[0].word, "хаце");
    EXPECT_EQ(words[0].part_of_speech_id, 1);
    EXPECT_EQ(words[0].part_of_speech, "назоўнік");
    EXPECT_EQ(words[0].accent, 1);
    EXPECT_FALSE(words[0].is_initial);
    EXPECT_EQ(words[0].initial_id, 1);
    ASSERT_TRUE(words[0].initial_word.has_value());
    EXPECT_EQ(*words[0].initial_word, "хата");
    ASSERT_TRUE(words[0].initial_accent.has_value());
    EXPECT_EQ(*words[0].initial_accent, 1);
}

TEST(Slounik, FindsSimilarLetterVariants) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());

    const auto words = slounik.FindWords(
        "вёрас", ryfmach::bel::WordLookupOptions{.fix_similar_letters = true});

    ASSERT_EQ(words.size(), 1);
    EXPECT_EQ(words[0].word, "верас");
}

TEST(Slounik, LimitsSimilarLetterVariants) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());

    const auto words = slounik.FindWords(
        "вёрас",
        ryfmach::bel::WordLookupOptions{
            .fix_similar_letters = true,
            .max_similar_letter_replacements = 0,
        });

    EXPECT_TRUE(words.empty());
}

TEST(Slounik, FindsCompoundTailWords) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());

    const auto words = slounik.FindWords("што-дзень");

    ASSERT_EQ(words.size(), 1);
    EXPECT_EQ(words[0].word, "што-дзень");
    EXPECT_EQ(words[0].accent, 6);
}

TEST(Slounik, GetsWordById) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());

    const auto word = slounik.GetWordById(1);

    ASSERT_TRUE(word.has_value());
    EXPECT_EQ(word->id, 1);
    EXPECT_EQ(word->word, "хата");
    EXPECT_TRUE(word->is_initial);
    EXPECT_EQ(word->initial_id, 1);
    EXPECT_FALSE(word->initial_word.has_value());
    EXPECT_FALSE(word->initial_accent.has_value());
}

TEST(Slounik, ReturnsNulloptForMissingWordId) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());

    EXPECT_FALSE(slounik.GetWordById(404).has_value());
}

TEST(Slounik, GetsWordForms) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());

    const auto words = slounik.GetWordForms(1);

    ASSERT_EQ(words.size(), 2);
    EXPECT_EQ(words[0].word, "хата");
    EXPECT_EQ(words[1].word, "хаце");
    EXPECT_EQ(words[1].initial_word, "хата");
}

TEST(Slounik, FindsStoredMorphemicAnalysesAndPrefixes) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());

    EXPECT_EQ(
        slounik.FindMorphemicAnalyses("хата"),
        (std::vector<std::string>{"(хат)-[а]"}));
    EXPECT_EQ(
        slounik.GetMorphemicPrefixes(),
        (std::vector<ryfmach::bel::MorphemicPrefixRecord>{
            {.text = "пры", .analysis = "|пры|"},
        }));
}

TEST(Slounik, FindsIdealRhymesBySoundHash) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::RhymeSearchFilters{
            ryfmach::bel::SearchMistakeLevel::kIdeal},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), std::vector<int>{5});
}

TEST(Slounik, FiltersRhymesByPartOfSpeech) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const ryfmach::bel::RhymeSearchFilters filters{
        .mistake = ryfmach::bel::SearchMistakeLevel::kIdeal,
        .part_of_speech = {false, true, false, false, false, false, false},
    };
    const auto rhymes = slounik.FindRhymes(
        input_word->word, input_word->accent, filters, 20);

    EXPECT_EQ(RhymeWordIds(rhymes), std::vector<int>{5});
}

TEST(Slounik, FindsGoodRhymesBySoundHash) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::RhymeSearchFilters{
            ryfmach::bel::SearchMistakeLevel::kGood},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), std::vector<int>{6});
}

TEST(Slounik, FindsMediumRhymesBySoundHash) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::RhymeSearchFilters{
            ryfmach::bel::SearchMistakeLevel::kMedium},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), std::vector<int>{7});
}

TEST(Slounik, FindsWeakRhymesBySoundHash) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::RhymeSearchFilters{
            ryfmach::bel::SearchMistakeLevel::kWeak},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), std::vector<int>{8});
}

TEST(Slounik, FindsAdaptiveRhymesAcrossMistakeLevels) {
    const ryfmach::bel::Slounik slounik(SharedSlounikTestDatabasePath());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::RhymeSearchFilters{
            ryfmach::bel::SearchMistakeLevel::kAdaptive},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), (std::vector<int>{5, 6, 7, 8}));
}


} // namespace
