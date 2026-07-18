#include "test_utils.hpp"

namespace {
TEST(Rhymes, BuildsWorkingPartForMistakeLevels) {
    EXPECT_EQ(ryfmach::bel::CalcWorkingPart("хата", 1, ryfmach::bel::RhymeMistakeLevel::kIdeal), "_а_та");
    EXPECT_EQ(ryfmach::bel::CalcWorkingPart("хата", 1, ryfmach::bel::RhymeMistakeLevel::kGood), "_а_ты");
    EXPECT_EQ(ryfmach::bel::CalcWorkingPart("хата", 1, ryfmach::bel::RhymeMistakeLevel::kMedium), "_а_ты");
    EXPECT_EQ(ryfmach::bel::CalcWorkingPart("хата", 1, ryfmach::bel::RhymeMistakeLevel::kWeak), "_а_фы");
}

TEST(Rhymes, KeepsPreviousSoundForFinalStressInStrictRhymes) {
    EXPECT_EQ(ryfmach::bel::CalcWorkingPart("дзень", 2, ryfmach::bel::RhymeMistakeLevel::kIdeal), "_э_н'");
    EXPECT_EQ(ryfmach::bel::CalcWorkingPart("дзень", 2, ryfmach::bel::RhymeMistakeLevel::kGood), "_э_н");
    EXPECT_EQ(ryfmach::bel::CalcWorkingPart("дзень", 2, ryfmach::bel::RhymeMistakeLevel::kMedium), "_э_н");
}

TEST(Rhymes, MatchesKnownWorkingPartsAndHashes) {
    struct MistakeCase {
        ryfmach::bel::RhymeMistakeLevel mistake;
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
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "_а_та", 445859199653511ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "_а_ты", 1226555324871529ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_а_ты", 1226555324871529ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_а_фы", 6004182820957543ULL},
            },
        },
        {
            "неба",
            1,
            {
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "_э_ба", 11088797979124984ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "_э_пы", 3485216863999811ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_э_пы", 3485216863999811ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_э_фы", 11105704373828186ULL},
            },
        },
        {
            "дзень",
            2,
            {
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "_э_н'", 10109537621506963ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "_э_н", 6803580395416928ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_э_н", 6803580395416928ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_э_л", 5555379031036954ULL},
            },
        },
        {
            "ксёндз",
            2,
            {
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "_о_нц", 1737930885662061ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "_о_нц", 1737930885662061ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_о_лц", 8508750108388868ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_о_с", 6525980150844431ULL},
            },
        },
        {
            "шчаўе",
            2,
            {
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "_а_ўйэ", 1160418455848226ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "_а_ўйы", 8679091878039836ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_а_ы", 968517201168586ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_а_ы", 968517201168586ULL},
            },
        },
        {
            "лодка",
            1,
            {
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "_о_тка", 577013656646148ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "_о_ткы", 11205430430294211ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_о_ткы", 11205430430294211ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_о_фы", 9835650543388685ULL},
            },
        },
        {
            "кніжка",
            2,
            {
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "_і_шка", 671887678995401ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "_і_шкы", 6167163906441582ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_і_шкы", 6167163906441582ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_і_фы", 7341684740097667ULL},
            },
        },
        {
            "касьба",
            5,
            {
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "б_а_", 8508740673753778ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "б_а_", 8508740673753778ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_а_", 9525167144504378ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_а_", 9525167144504378ULL},
            },
        },
        {
            "лічба",
            1,
            {
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "_і_джба", 8478135653608230ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "_і_чпы", 939249229248069ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_і_шпы", 11402240861085748ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_і_фы", 7341684740097667ULL},
            },
        },
        {
            "малацьба",
            7,
            {
                {ryfmach::bel::RhymeMistakeLevel::kIdeal, "б_а_", 8508740673753778ULL},
                {ryfmach::bel::RhymeMistakeLevel::kGood, "б_а_", 8508740673753778ULL},
                {ryfmach::bel::RhymeMistakeLevel::kMedium, "_а_", 9525167144504378ULL},
                {ryfmach::bel::RhymeMistakeLevel::kWeak, "_а_", 9525167144504378ULL},
            },
        },
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.word);

        for (const auto& mistake_case : test_case.mistakes) {
            SCOPED_TRACE(static_cast<int>(mistake_case.mistake));

            EXPECT_EQ(ryfmach::bel::CalcWorkingPart(
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
            ryfmach::bel::CalcRhymeCost(*left, *right, 2);

        EXPECT_DOUBLE_EQ(suffix_cost, test_case.suffix_cost);
        EXPECT_DOUBLE_EQ(prefix_cost, test_case.prefix_cost);
    }
}


} // namespace
