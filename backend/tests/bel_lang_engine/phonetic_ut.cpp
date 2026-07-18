#include "test_utils.hpp"

namespace {
using ryfmach::tests::JoinTranscription;
TEST(Transcription, MarksStressedVowelsWithoutSeparatePhonemes) {
    const auto transcription = ryfmach::bel::GetTranscriptionSounds("мама", 1);

    ASSERT_TRUE(transcription.has_value());
    ASSERT_EQ(transcription->size(), 4);
    EXPECT_EQ((*transcription)[1].phoneme, ryfmach::bel::Phoneme::kA);
    EXPECT_TRUE((*transcription)[1].stressed);
    EXPECT_EQ(ryfmach::bel::SoundSpelling((*transcription)[1]), "_а_");
    EXPECT_EQ(ryfmach::bel::GetAccentInTranscription(*transcription), 1);
}

TEST(Transcription, HandlesIotatedVowels) {
    const auto transcription = ryfmach::bel::GetTranscriptionSounds("яма", 0);

    ASSERT_TRUE(transcription.has_value());
    ASSERT_EQ(transcription->size(), 4);
    EXPECT_EQ((*transcription)[0].phoneme, ryfmach::bel::Phoneme::kJ);
    EXPECT_EQ((*transcription)[1].phoneme, ryfmach::bel::Phoneme::kA);
    EXPECT_TRUE((*transcription)[1].stressed);
}

TEST(Transcription, DevoicesFinalConsonant) {
    const auto transcription = ryfmach::bel::GetTranscriptionSounds("сад", 1);

    ASSERT_TRUE(transcription.has_value());
    ASSERT_EQ(transcription->size(), 3);
    EXPECT_EQ((*transcription)[2].phoneme, ryfmach::bel::Phoneme::kT);
}

TEST(Transcription, FoldsAffricates) {
    const auto transcription = ryfmach::bel::GetTranscriptionSounds("дзень", 2);

    ASSERT_TRUE(transcription.has_value());
    ASSERT_EQ(transcription->size(), 3);
    EXPECT_EQ((*transcription)[0].phoneme, ryfmach::bel::Phoneme::kDzSoft);
    EXPECT_EQ((*transcription)[1].phoneme, ryfmach::bel::Phoneme::kE);
    EXPECT_TRUE((*transcription)[1].stressed);
    EXPECT_EQ((*transcription)[2].phoneme, ryfmach::bel::Phoneme::kNSoft);
}

TEST(Transcription, BuildsFullTranscriptionWithMappingsAndPhenomena) {
    const auto full = ryfmach::bel::GetTranscriptionFull("вераб'і", 6);

    ASSERT_TRUE(full.has_value());
    const std::vector<std::vector<std::size_t>> expected_mapping = {
        {0}, {1}, {2}, {3}, {4}, {}, {5, 6},
    };
    EXPECT_EQ(full->letter_to_sounds, expected_mapping);
    EXPECT_EQ(JoinTranscription(full->transcription), "в' э р а б й _і_");

    ASSERT_EQ(full->phenomena.size(), 2);
    EXPECT_EQ(full->phenomena[0].sound_index, 0);
    EXPECT_EQ(
        full->phenomena[0].phenomenon,
        ryfmach::bel::PhoneticPhenomenon::kConsonantSoftening);
    EXPECT_EQ(
        JoinTranscription(full->phenomena[0].transcription),
        "в э р а б _і_");
    EXPECT_EQ(full->phenomena[1].sound_index, 5);
    EXPECT_EQ(
        full->phenomena[1].phenomenon,
        ryfmach::bel::PhoneticPhenomenon::kIotation);
    EXPECT_EQ(
        JoinTranscription(full->phenomena[1].transcription),
        "в' э р а б _і_");
}

TEST(Transcription, RecordsFullAffricationAndAssimilation) {
    const auto full = ryfmach::bel::GetTranscriptionFull("ксёндз", 2);

    ASSERT_TRUE(full.has_value());
    const std::vector<std::vector<std::size_t>> expected_mapping = {
        {0}, {1}, {2}, {3}, {4}, {4},
    };
    EXPECT_EQ(full->letter_to_sounds, expected_mapping);
    EXPECT_EQ(JoinTranscription(full->transcription), "к с' _о_ н ц");

    ASSERT_EQ(full->phenomena.size(), 3);
    EXPECT_EQ(
        full->phenomena[0].phenomenon,
        ryfmach::bel::PhoneticPhenomenon::kConsonantSoftening);
    EXPECT_EQ(
        full->phenomena[1].phenomenon,
        ryfmach::bel::PhoneticPhenomenon::kAffricates);
    EXPECT_EQ(
        full->phenomena[2].phenomenon,
        ryfmach::bel::PhoneticPhenomenon::kThudAssimilation);
    EXPECT_EQ(full->phenomena[1].sound_index, 4);
    EXPECT_EQ(full->phenomena[2].sound_index, 4);
    EXPECT_EQ(
        JoinTranscription(full->phenomena[2].transcription),
        "к с' _о_ н дз");
}

TEST(Transcription, FullTranscriptionDoesNotIotateInitialI) {
    const auto full = ryfmach::bel::GetTranscriptionFull("іва", 0);

    ASSERT_TRUE(full.has_value());
    EXPECT_TRUE(full->phenomena.empty());
    EXPECT_EQ(JoinTranscription(full->transcription), "_і_ в а");
}

TEST(Transcription, RejectsAccentOnNonVowel) {
    EXPECT_FALSE(ryfmach::bel::GetTranscriptionSounds("мама", 0).has_value());
}

TEST(Transcription, MatchesKnownWords) {
    struct TestCase {
        std::string_view word;
        std::size_t stress;
        std::string_view expected;
    };

    const std::vector<TestCase> test_cases = {
        {"хата", 1, "х _а_ т а"},
        {"неба", 1, "н' _э_ б а"},
        {"дзень", 2, "дз' _э_ н'"},
        {"ксёндз", 2, "к с' _о_ н ц"},
        {"шчаўе", 2, "ш ч _а_ ў й э"},
        {"лодка", 1, "л _о_ т к а"},
        {"кніжка", 2, "к н' _і_ ш к а"},
        {"касьба", 5, "к а з' б _а_"},
        {"лічба", 1, "л' _і_ дж б а"},
        {"малацьба", 7, "м а л а дз' б _а_"},
        {"цемя", 1, "ц' _э_ м' а"},
        {"здзек", 3, "з' дз' _э_ к"},
        {"збіраць", 4, "з' б' і р _а_ ц'"},
        {"дзверы", 3, "дз' в' _э_ р ы"},
        {"чацвёрты", 4, "ч а ц' в' _о_ р т ы"},
        {"скінуць", 2, "с к' _і_ н у ц'"},
        {"дошцы", 1, "д _о_ с ц ы"},
        {"смяешся", 3, "с' м' а й _э_ с' с' а"},
        {"зжаць", 2, "ж ж _а_ ц'"},
        {"сшытак", 2, "ш ш _ы_ т а к"},
        {"расчасаць", 6, "р а ш ч а с _а_ ц'"},
        {"нарэшце", 3, "н а р _э_ с' ц' э"},
        {"нарэжце", 3, "н а р _э_ с' ц' э"},
        {"матчын", 1, "м _а_ ч ч ы н"},
        {"кладцы", 2, "к л _а_ ц ц ы"},
        {"разводдзе", 4, "р а з в _о_ дз' дз' э"},
        {"ерась", 0, "й _э_ р а с'"},
        {"верас", 1, "в' _э_ р а с"},
        {"роспач", 1, "р _о_ с п а ч"},
        {"пробашч", 2, "п р _о_ б а ш ч"},
        {"бусел", 1, "б _у_ с' э л"},
        {"вузел", 1, "в _у_ з' э л"},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.word);

        const auto transcription = ryfmach::bel::GetTranscriptionSounds(
            test_case.word, test_case.stress);

        ASSERT_TRUE(transcription.has_value());
        EXPECT_EQ(JoinTranscription(*transcription), test_case.expected);
    }
}


} // namespace
