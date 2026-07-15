#include "language.hpp"
#include "morphemics.hpp"
#include "rhymes.hpp"
#include "slounik.hpp"
#include "sounds.hpp"
#include "transcription.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string JoinTranscription(std::span<const ryfmach::bel::Sound> transcription) {
    std::string result;
    for (std::size_t index = 0; index < transcription.size(); ++index) {
        if (index != 0) {
            result += ' ';
        }
        result += ryfmach::bel::SoundSpelling(transcription[index]);
    }
    return result;
}

std::array<std::uint64_t, 4> SoundHashesFor(
    std::string_view word,
    std::size_t stress) {
    return {
        *ryfmach::bel::SoundHash(
            word, stress, ryfmach::bel::RhymeMistakeLevel::kIdeal),
        *ryfmach::bel::SoundHash(
            word, stress, ryfmach::bel::RhymeMistakeLevel::kGood),
        *ryfmach::bel::SoundHash(
            word, stress, ryfmach::bel::RhymeMistakeLevel::kMedium),
        *ryfmach::bel::SoundHash(
            word, stress, ryfmach::bel::RhymeMistakeLevel::kWeak),
    };
}

void InsertWord(
    SQLite::Database& db,
    int id,
    std::string_view word,
    int initial_id,
    int part_of_speech,
    std::size_t accent,
    std::array<std::uint64_t, 4> sound_hashes) {
    SQLite::Statement insert(
        db,
        R"sql(
            INSERT INTO words
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        )sql");

    insert.bind(1, id);
    insert.bind(2, std::string(word));
    insert.bind(3, initial_id);
    insert.bind(4, part_of_speech);
    insert.bind(5, static_cast<std::int64_t>(accent));
    for (std::size_t index = 0; index < sound_hashes.size(); ++index) {
        insert.bind(
            static_cast<int>(index + 6),
            static_cast<std::int64_t>(sound_hashes[index]));
    }
    insert.exec();
}

std::vector<int> RhymeWordIds(std::span<const ryfmach::bel::Rhyme> rhymes) {
    std::vector<int> ids;
    ids.reserve(rhymes.size());
    for (const auto& rhyme : rhymes) {
        ids.push_back(rhyme.word_id);
    }
    return ids;
}

std::filesystem::path CreateSlounikTestDatabase() {
    const auto path =
        std::filesystem::temp_directory_path() / "ryfmach_slounik_test.sqlite";

    SQLite::Database db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("DROP TABLE IF EXISTS words");
    db.exec("DROP TABLE IF EXISTS parts_of_speech");
    db.exec("DROP TABLE IF EXISTS morphemics");
    db.exec("DROP TABLE IF EXISTS morph_prefixes");
    db.exec(R"sql(
        CREATE TABLE parts_of_speech (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL
        )
    )sql");
    db.exec(R"sql(
        CREATE TABLE words (
            id INTEGER PRIMARY KEY,
            word TEXT NOT NULL,
            initial_id INTEGER NOT NULL,
            part_of_speech INTEGER NOT NULL,
            accent_index INTEGER NOT NULL,
            sound_hash0 INTEGER,
            sound_hash1 INTEGER,
            sound_hash2 INTEGER,
            sound_hash3 INTEGER
        )
    )sql");
    db.exec("INSERT INTO parts_of_speech (id, name) VALUES (1, 'назоўнік')");
    db.exec(R"sql(
        CREATE TABLE morphemics (
            id INTEGER PRIMARY KEY,
            word TEXT NOT NULL,
            analysis TEXT NOT NULL
        )
    )sql");
    db.exec(R"sql(
        CREATE TABLE morph_prefixes (
            id INTEGER PRIMARY KEY,
            text TEXT NOT NULL,
            analysis TEXT NOT NULL
        )
    )sql");
    db.exec("INSERT INTO morphemics VALUES (1, 'хата', '(хат)-[а]')");
    db.exec("INSERT INTO morphemics VALUES (2, 'ход', '(ход)-[]')");
    db.exec("INSERT INTO morphemics VALUES (3, 'мыць', '(мы)-<ць>')");
    db.exec("INSERT INTO morph_prefixes VALUES (1, 'пры', '|пры|')");

    const auto khata_hashes = SoundHashesFor("хата", 1);
    InsertWord(db, 1, "хата", 1, 1, 1, khata_hashes);
    InsertWord(db, 2, "хаце", 1, 1, 1, SoundHashesFor("хаце", 1));
    InsertWord(db, 3, "верас", 3, 1, 1, SoundHashesFor("верас", 1));
    InsertWord(db, 4, "дзень", 4, 1, 2, SoundHashesFor("дзень", 2));
    InsertWord(db, 9, "гуляць", 9, 2, 3, SoundHashesFor("гуляць", 3));
    InsertWord(db, 10, "мыцца", 10, 2, 4, SoundHashesFor("мыцца", 4));

    InsertWord(
        db, 5, "рата", 5, 1, 1,
        {khata_hashes[0], 100001, 100002, 100003});
    InsertWord(
        db, 6, "хаты", 6, 1, 1,
        {100010, khata_hashes[1], 100012, 100013});
    InsertWord(
        db, 7, "мата", 7, 1, 1,
        {100020, 100021, khata_hashes[2], 100023});
    InsertWord(
        db, 8, "дата", 8, 1, 1,
        {100030, 100031, 100032, khata_hashes[3]});

    return path;
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

TEST(Sounds, DescribesVowelsAndConsonantsInBelarusian) {
    EXPECT_EQ(
        ryfmach::bel::SoundDescription(
            {.phoneme = ryfmach::bel::Phoneme::kA, .stressed = false}),
        "галосны, ненаціскны");
    EXPECT_EQ(
        ryfmach::bel::SoundDescription(
            {.phoneme = ryfmach::bel::Phoneme::kA, .stressed = true}),
        "галосны, націскны");
    EXPECT_EQ(
        ryfmach::bel::SoundDescription(
            {.phoneme = ryfmach::bel::Phoneme::kB, .stressed = false}),
        "зычны, звонкі парны [п], цвёрды парны [б']");
    EXPECT_EQ(
        ryfmach::bel::SoundDescription(
            {.phoneme = ryfmach::bel::Phoneme::kW, .stressed = false}),
        "зычны, звонкі няпарны, цвёрды няпарны");
}

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

TEST(Slounik, FindsExactWords) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());

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
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());

    const auto words = slounik.FindWords(
        "вёрас", ryfmach::bel::WordLookupOptions{.fix_similar_letters = true});

    ASSERT_EQ(words.size(), 1);
    EXPECT_EQ(words[0].word, "верас");
}

TEST(Slounik, LimitsSimilarLetterVariants) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());

    const auto words = slounik.FindWords(
        "вёрас",
        ryfmach::bel::WordLookupOptions{
            .fix_similar_letters = true,
            .max_similar_letter_replacements = 0,
        });

    EXPECT_TRUE(words.empty());
}

TEST(Slounik, FindsCompoundTailWords) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());

    const auto words = slounik.FindWords("што-дзень");

    ASSERT_EQ(words.size(), 1);
    EXPECT_EQ(words[0].word, "што-дзень");
    EXPECT_EQ(words[0].accent, 6);
}

TEST(Slounik, GetsWordById) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());

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
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());

    EXPECT_FALSE(slounik.GetWordById(404).has_value());
}

TEST(Slounik, GetsWordForms) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());

    const auto words = slounik.GetWordForms(1);

    ASSERT_EQ(words.size(), 2);
    EXPECT_EQ(words[0].word, "хата");
    EXPECT_EQ(words[1].word, "хаце");
    EXPECT_EQ(words[1].initial_word, "хата");
}

TEST(Slounik, FindsStoredMorphemicAnalysesAndPrefixes) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());

    EXPECT_EQ(
        slounik.FindMorphemicAnalyses("хата"),
        (std::vector<std::string>{"(хат)-[а]"}));
    EXPECT_EQ(
        slounik.GetMorphemicPrefixes(),
        (std::vector<ryfmach::bel::MorphemicPrefixRecord>{
            {.text = "пры", .analysis = "|пры|"},
        }));
}

TEST(Morphemics, UsesStoredAnalysesAndPrefixPredictions) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
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
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
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

TEST(Slounik, FindsIdealRhymesBySoundHash) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::SearchMistakeLevel::kIdeal,
        ryfmach::bel::RhymeSearchFilters{},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), std::vector<int>{5});
}

TEST(Slounik, FindsGoodRhymesBySoundHash) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::SearchMistakeLevel::kGood,
        ryfmach::bel::RhymeSearchFilters{},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), std::vector<int>{6});
}

TEST(Slounik, FindsMediumRhymesBySoundHash) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::SearchMistakeLevel::kMedium,
        ryfmach::bel::RhymeSearchFilters{},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), std::vector<int>{7});
}

TEST(Slounik, FindsWeakRhymesBySoundHash) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::SearchMistakeLevel::kWeak,
        ryfmach::bel::RhymeSearchFilters{},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), std::vector<int>{8});
}

TEST(Slounik, FindsAdaptiveRhymesAcrossMistakeLevels) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
    const auto input_word = slounik.GetWordById(1);
    ASSERT_TRUE(input_word.has_value());

    const auto rhymes = slounik.FindRhymes(
        input_word->word,
        input_word->accent,
        ryfmach::bel::SearchMistakeLevel::kAdaptive,
        ryfmach::bel::RhymeSearchFilters{},
        20);

    EXPECT_EQ(RhymeWordIds(rhymes), (std::vector<int>{5, 6, 7, 8}));
}

} // namespace
