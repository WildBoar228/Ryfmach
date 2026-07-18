#include "rhymes.hpp"
#include "ryfmach_service.hpp"
#include "sounds.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
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

std::filesystem::path CreateSlounikTestDatabase() {
    const auto path = std::filesystem::temp_directory_path() /
                      "ryfmach_service_test.sqlite";

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
    db.exec("INSERT INTO parts_of_speech (id, name) VALUES (2, 'дзеяслоў')");
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

    const auto khata_hashes = SoundHashesFor("хата", 1);
    InsertWord(db, 1, "хата", 1, 1, 1, khata_hashes);
    InsertWord(db, 2, "хаце", 1, 1, 1, SoundHashesFor("хаце", 1));
    InsertWord(db, 3, "верас", 3, 1, 1, SoundHashesFor("верас", 1));
    InsertWord(db, 4, "дзень", 4, 1, 2, SoundHashesFor("дзень", 2));

    InsertWord(
        db, 5, "рата", 5, 2, 1,
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

std::vector<int> WordIds(std::span<const ryfmach::bel::WordRecord> words) {
    std::vector<int> ids;
    ids.reserve(words.size());
    for (const auto& word : words) {
        ids.push_back(word.id);
    }
    return ids;
}

TEST(RyfmachService, FindsRhymesForEveryDictionaryWordVariant) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
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
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
    const ryfmach::app::RyfmachService service(slounik);

    const auto result = service.FindRhymes("хата", {
        .part_of_speech = {false, true, false, false, false, false, false},
    });

    ASSERT_TRUE(result.word_found);
    ASSERT_EQ(result.rhymes_list.size(), 1);
    EXPECT_EQ(WordIds(result.rhymes_list[0].rhymes), std::vector<int>{5});
}

TEST(RyfmachService, FindsRhymesForManualAccentWithoutDictionaryLookup) {
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
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
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
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
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
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
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
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
    const ryfmach::bel::Slounik slounik(CreateSlounikTestDatabase());
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

} // namespace
