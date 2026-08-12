#ifndef RYFMACH_TESTS_BEL_LANG_ENGINE_TEST_UTILS_HPP_
#define RYFMACH_TESTS_BEL_LANG_ENGINE_TEST_UTILS_HPP_

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

namespace ryfmach::tests {

inline std::string JoinTranscription(
    std::span<const ryfmach::bel::Sound> transcription) {
    std::string result;
    for (std::size_t index = 0; index < transcription.size(); ++index) {
        if (index != 0) {
            result += ' ';
        }
        result += ryfmach::bel::SoundSpelling(transcription[index]);
    }
    return result;
}

inline std::array<std::uint64_t, 4> SoundHashesFor(
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

inline void InsertWord(
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

inline std::vector<int> RhymeWordIds(
    std::span<const ryfmach::bel::Rhyme> rhymes) {
    std::vector<int> ids;
    ids.reserve(rhymes.size());
    for (const auto& rhyme : rhymes) {
        ids.push_back(rhyme.word_id);
    }
    return ids;
}

inline std::filesystem::path SharedSlounikTestDatabasePath() {
    constexpr int kSchemaVersion = 4;
    const auto path =
        std::filesystem::temp_directory_path() /
        "ryfmach_shared_slounik_test_v4.sqlite";

    SQLite::Database db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.setBusyTimeout(10000);
    if (db.execAndGet("PRAGMA user_version").getInt() == kSchemaVersion) {
        return path;
    }

    db.exec("BEGIN EXCLUSIVE");
    if (db.execAndGet("PRAGMA user_version").getInt() == kSchemaVersion) {
        db.exec("COMMIT");
        return path;
    }

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
    InsertWord(db, 11, "ласка", 11, 1, 1,
        {200001, 200002, 200003, 200004});
    InsertWord(db, 12, "ласка", 12, 2, 1,
        {200011, 200012, 200013, 200014});
    InsertWord(db, 13, "замкі", 13, 1, 1,
        {200021, 200022, 200023, 200024});
    InsertWord(db, 14, "замкі", 14, 1, 4,
        {200031, 200032, 200033, 200034});
    InsertWord(db, 15, "мука", 15, 1, 1,
        {200041, 200042, 200043, 200044});
    InsertWord(db, 16, "мука", 16, 1, 3,
        {200051, 200052, 200053, 200054});
    InsertWord(db, 17, "мўка", 17, 1, 1,
        {200061, 200062, 200063, 200064});
    InsertWord(db, 18, "мўка", 18, 1, 3,
        {200071, 200072, 200073, 200074});
    InsertWord(db, 19, "сена", 19, 1, 1,
        {200081, 200082, 200083, 200084});
    InsertWord(db, 20, "сёна", 20, 1, 1,
        {200091, 200092, 200093, 200094});

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

    db.exec("PRAGMA user_version = 4");
    db.exec("COMMIT");

    return path;
}

inline std::vector<int> WordIds(
    std::span<const ryfmach::bel::WordRecord> words) {
    std::vector<int> ids;
    ids.reserve(words.size());
    for (const auto& word : words) {
        ids.push_back(word.id);
    }
    return ids;
}

} // namespace ryfmach::tests

#endif // RYFMACH_TESTS_BEL_LANG_ENGINE_TEST_UTILS_HPP_
