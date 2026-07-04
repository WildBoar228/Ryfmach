#include "language.hpp"
#include "rhymes.hpp"
#include "sounds.hpp"
#include "transcription.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string JoinTranscription(
    const std::vector<ryfmach::bel::Sound>& transcription) {
    std::string result;
    for (std::size_t index = 0; index < transcription.size(); ++index) {
        if (index != 0) {
            result += ' ';
        }
        result += ryfmach::bel::SoundSpelling(transcription[index]);
    }
    return result;
}

TEST(Language, ParsesBelarusianWord) {
    const auto letters = ryfmach::bel::ParseWord("Беларусь");

    ASSERT_TRUE(letters.has_value());
    ASSERT_EQ(letters->size(), 8);
    EXPECT_EQ((*letters)[0], ryfmach::bel::Letter::kBe);
    EXPECT_EQ((*letters)[7], ryfmach::bel::Letter::kSoftSign);
}

TEST(Language, ParsesDashInsideBelarusianWord) {
    const auto letters = ryfmach::bel::ParseWord("што-небудзь");

    ASSERT_TRUE(letters.has_value());
    ASSERT_EQ(letters->size(), 11);
    EXPECT_EQ((*letters)[3], ryfmach::bel::Letter::kDash);
    EXPECT_TRUE(ryfmach::bel::IsDash((*letters)[3]));
}

TEST(Language, RejectsDashAtWordBoundary) {
    EXPECT_FALSE(ryfmach::bel::ParseWord("-небудзь").has_value());
    EXPECT_FALSE(ryfmach::bel::ParseWord("што-").has_value());
    EXPECT_FALSE(ryfmach::bel::ParseWord("што--небудзь").has_value());
}

TEST(Language, ClassifiesLetters) {
    EXPECT_TRUE(ryfmach::bel::IsVowel(ryfmach::bel::Letter::kA));
    EXPECT_TRUE(ryfmach::bel::IsConsonant(ryfmach::bel::Letter::kBe));
    EXPECT_TRUE(ryfmach::bel::IsSofteningVowel(ryfmach::bel::Letter::kYa));
    EXPECT_FALSE(ryfmach::bel::IsAlphabetLetter(
        ryfmach::bel::Letter::kApostrophe));
    EXPECT_FALSE(ryfmach::bel::IsAlphabetLetter(ryfmach::bel::Letter::kDash));
}

TEST(Language, SpellsPhonemesWithCyrillicText) {
    EXPECT_EQ(
        ryfmach::bel::PhonemeSpelling(ryfmach::bel::Phoneme::kBSoft), "б'");
    EXPECT_EQ(ryfmach::bel::PhonemeSpelling(ryfmach::bel::Phoneme::kW), "ў");
}

TEST(Sounds, FindsRingAndThudPairs) {
    EXPECT_TRUE(ryfmach::bel::IsRing(ryfmach::bel::Phoneme::kB));
    EXPECT_TRUE(ryfmach::bel::IsThud(ryfmach::bel::Phoneme::kP));
    EXPECT_EQ(ryfmach::bel::RingPair(ryfmach::bel::Phoneme::kP),
              ryfmach::bel::Phoneme::kB);
    EXPECT_EQ(ryfmach::bel::ThudPair(ryfmach::bel::Phoneme::kBSoft),
              ryfmach::bel::Phoneme::kPSoft);
}

TEST(Sounds, FindsHardAndSoftPairs) {
    EXPECT_TRUE(ryfmach::bel::IsHard(ryfmach::bel::Phoneme::kS));
    EXPECT_TRUE(ryfmach::bel::IsSoft(ryfmach::bel::Phoneme::kSSoft));
    EXPECT_EQ(ryfmach::bel::SoftPair(ryfmach::bel::Phoneme::kS),
              ryfmach::bel::Phoneme::kSSoft);
    EXPECT_EQ(ryfmach::bel::HardPair(ryfmach::bel::Phoneme::kSSoft),
              ryfmach::bel::Phoneme::kS);
}

TEST(Sounds, HandlesMissingPairs) {
    EXPECT_TRUE(ryfmach::bel::IsSonor(ryfmach::bel::Phoneme::kW));
    EXPECT_FALSE(ryfmach::bel::ThudPair(ryfmach::bel::Phoneme::kW).has_value());
    EXPECT_FALSE(ryfmach::bel::SoftPair(ryfmach::bel::Phoneme::kR).has_value());
    EXPECT_FALSE(ryfmach::bel::RingPair(ryfmach::bel::Phoneme::kA).has_value());
}

TEST(Sounds, FindsWhistlingAndHissingPairs) {
    EXPECT_TRUE(ryfmach::bel::IsWhistling(ryfmach::bel::Phoneme::kDzSoft));
    EXPECT_TRUE(ryfmach::bel::IsHissing(ryfmach::bel::Phoneme::kDzh));
    EXPECT_EQ(ryfmach::bel::WhistlingPair(ryfmach::bel::Phoneme::kDzh),
              ryfmach::bel::Phoneme::kDz);
    EXPECT_EQ(ryfmach::bel::HissingPair(ryfmach::bel::Phoneme::kTsSoft),
              ryfmach::bel::Phoneme::kCh);
}

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

TEST(Rhymes, BuildsWorkingPartForMistakeLevels) {
    EXPECT_EQ(ryfmach::bel::WorkingPart("хата", 1, 0), "_а_та");
    EXPECT_EQ(ryfmach::bel::WorkingPart("хата", 1, 1), "_а_ты");
    EXPECT_EQ(ryfmach::bel::WorkingPart("хата", 1, 2), "_а_ты");
    EXPECT_EQ(ryfmach::bel::WorkingPart("хата", 1, 3), "_а_фы");
}

TEST(Rhymes, KeepsPreviousSoundForFinalStressInStrictRhymes) {
    EXPECT_EQ(ryfmach::bel::WorkingPart("дзень", 2, 0), "_э_н'");
    EXPECT_EQ(ryfmach::bel::WorkingPart("дзень", 2, 1), "_э_н");
    EXPECT_EQ(ryfmach::bel::WorkingPart("дзень", 2, 2), "_э_н");
}

TEST(Rhymes, MatchesKnownWorkingPartsAndHashes) {
    struct MistakeCase {
        int mistake;
        std::string_view working_part;
        std::uint64_t hash;
    };

    struct WordCase {
        std::string_view word;
        std::size_t stress;
        std::vector<MistakeCase> mistakes;
    };

    const std::vector<WordCase> test_cases = {
        {
            "хата",
            1,
            {
                {0, "_а_та", 445859199653511ULL},
                {1, "_а_ты", 1226555324871529ULL},
                {2, "_а_ты", 1226555324871529ULL},
                {3, "_а_фы", 6004182820957543ULL},
            },
        },
        {
            "неба",
            1,
            {
                {0, "_э_ба", 11088797979124984ULL},
                {1, "_э_пы", 3485216863999811ULL},
                {2, "_э_пы", 3485216863999811ULL},
                {3, "_э_фы", 11105704373828186ULL},
            },
        },
        {
            "дзень",
            2,
            {
                {0, "_э_н'", 10109537621506963ULL},
                {1, "_э_н", 6803580395416928ULL},
                {2, "_э_н", 6803580395416928ULL},
                {3, "_э_л", 5555379031036954ULL},
            },
        },
        {
            "ксёндз",
            2,
            {
                {0, "_о_нц", 1737930885662061ULL},
                {1, "_о_нц", 1737930885662061ULL},
                {2, "_о_лц", 8508750108388868ULL},
                {3, "_о_с", 6525980150844431ULL},
            },
        },
        {
            "шчаўе",
            2,
            {
                {0, "_а_ўйэ", 1160418455848226ULL},
                {1, "_а_ўйы", 8679091878039836ULL},
                {2, "_а_ы", 968517201168586ULL},
                {3, "_а_ы", 968517201168586ULL},
            },
        },
        {
            "лодка",
            1,
            {
                {0, "_о_тка", 577013656646148ULL},
                {1, "_о_ткы", 11205430430294211ULL},
                {2, "_о_ткы", 11205430430294211ULL},
                {3, "_о_фы", 9835650543388685ULL},
            },
        },
        {
            "кніжка",
            2,
            {
                {0, "_і_шка", 671887678995401ULL},
                {1, "_і_шкы", 6167163906441582ULL},
                {2, "_і_шкы", 6167163906441582ULL},
                {3, "_і_фы", 7341684740097667ULL},
            },
        },
        {
            "касьба",
            5,
            {
                {0, "б_а_", 8508740673753778ULL},
                {1, "б_а_", 8508740673753778ULL},
                {2, "_а_", 9525167144504378ULL},
                {3, "_а_", 9525167144504378ULL},
            },
        },
        {
            "лічба",
            1,
            {
                {0, "_і_джба", 8478135653608230ULL},
                {1, "_і_чпы", 939249229248069ULL},
                {2, "_і_шпы", 11402240861085748ULL},
                {3, "_і_фы", 7341684740097667ULL},
            },
        },
        {
            "малацьба",
            7,
            {
                {0, "б_а_", 8508740673753778ULL},
                {1, "б_а_", 8508740673753778ULL},
                {2, "_а_", 9525167144504378ULL},
                {3, "_а_", 9525167144504378ULL},
            },
        },
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.word);

        for (const auto& mistake_case : test_case.mistakes) {
            SCOPED_TRACE(mistake_case.mistake);

            EXPECT_EQ(ryfmach::bel::WorkingPart(
                          test_case.word, test_case.stress, mistake_case.mistake),
                      mistake_case.working_part);
            EXPECT_EQ(ryfmach::bel::SoundHash(
                          test_case.word, test_case.stress, mistake_case.mistake),
                      mistake_case.hash);
        }
    }
}

TEST(Rhymes, UsesDefaultSoundCompatibilityTable) {
    using ryfmach::bel::Phoneme;
    using ryfmach::bel::Sound;

    EXPECT_DOUBLE_EQ(
        ryfmach::bel::SoundReplaceCost(Sound{Phoneme::kB}, Sound{Phoneme::kP}),
        3.5);
    EXPECT_DOUBLE_EQ(
        ryfmach::bel::SoundReplaceCost(
            Sound{Phoneme::kA, true}, Sound{Phoneme::kP}),
        40.0);
    EXPECT_DOUBLE_EQ(
        ryfmach::bel::SoundReplaceCost(std::optional<Sound>{}, std::optional<Sound>{}),
        0.0);
}

TEST(Rhymes, LoadsSoundCompatibilityTableFromFile) {
    using ryfmach::bel::Phoneme;
    using ryfmach::bel::Sound;

    const auto path =
        std::filesystem::temp_directory_path() / "ryfmach_sound_compatibility.tsv";
    {
        std::ofstream output(path);
        output << "# left_sound\tright_sound\treplace_cost\n";
        output << "б\tп\t42\n";
        output << "_а_\tп\t77\n";
    }

    const auto table = ryfmach::bel::LoadSoundCompatibilityTable(path);

    ASSERT_TRUE(table.has_value());
    EXPECT_DOUBLE_EQ(
        ryfmach::bel::SoundReplaceCost(Sound{Phoneme::kB}, Sound{Phoneme::kP}, *table),
        42.0);
    EXPECT_DOUBLE_EQ(
        ryfmach::bel::SoundReplaceCost(Sound{Phoneme::kP}, Sound{Phoneme::kB}, *table),
        42.0);
    EXPECT_DOUBLE_EQ(
        ryfmach::bel::SoundReplaceCost(
            Sound{Phoneme::kA, true}, Sound{Phoneme::kP}, *table),
        77.0);
    EXPECT_DOUBLE_EQ(
        ryfmach::bel::SoundReplaceCost(Sound{Phoneme::kA}, Sound{Phoneme::kO}, *table),
        1000.0);
}

TEST(Rhymes, CalculatesKnownRhymeQualityKeys) {
    struct TestCase {
        std::string_view left_word;
        std::size_t left_stress;
        std::string_view right_word;
        std::size_t right_stress;
        double suffix_cost;
        double prefix_cost;
    };

    const std::vector<TestCase> test_cases = {
        {"неба", 1, "глеба", 2, 0.0, 5.0},
        {"неба", 1, "хлеба", 2, 0.0, 5.0},
        {"неба", 1, "трэба", 2, 0.0, 19.0},
        {"неба", 1, "бясхлеб'і", 5, 1.0, 5.0},
        {"неба", 1, "слепа", 2, 3.5, 5.0},
        {"неба", 1, "свірэпа", 4, 3.5, 19.0},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.left_word);
        SCOPED_TRACE(test_case.right_word);

        const auto left = ryfmach::bel::GetTranscriptionSounds(
            test_case.left_word, test_case.left_stress);
        const auto right = ryfmach::bel::GetTranscriptionSounds(
            test_case.right_word, test_case.right_stress);

        ASSERT_TRUE(left.has_value());
        ASSERT_TRUE(right.has_value());

        const auto [suffix_cost, prefix_cost] =
            ryfmach::bel::CalcRhymeQualityKey(*left, *right, 2);

        EXPECT_DOUBLE_EQ(suffix_cost, test_case.suffix_cost);
        EXPECT_DOUBLE_EQ(prefix_cost, test_case.prefix_cost);
    }
}

} // namespace
