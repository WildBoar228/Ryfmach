#include "rhyme_likes.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace ryfmach::bel {
namespace {

constexpr char kRhymeLikesDbPathVariable[] = "RHYME_LIKES_DB_PATH";

std::filesystem::path GetRhymeLikesDbPathFromEnvironment() {
    const char* path = std::getenv(kRhymeLikesDbPathVariable);
    if (path == nullptr || std::string_view(path).empty()) {
        throw std::runtime_error("RHYME_LIKES_DB_PATH is not set");
    }
    return std::filesystem::path(path);
}

void InitializeDatabase(SQLite::Database& db) {
    db.exec(R"sql(
        CREATE TABLE IF NOT EXISTS rhyme_likes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            request_word TEXT NOT NULL,
            request_stress INTEGER NOT NULL,
            rhyme_word TEXT NOT NULL,
            rhyme_stress INTEGER NOT NULL,
            score INTEGER NOT NULL DEFAULT 0,
            updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(request_word, request_stress, rhyme_word, rhyme_stress)
        )
    )sql");
}

} // namespace

RhymeLikes::RhymeLikes()
    : RhymeLikes(GetRhymeLikesDbPathFromEnvironment()) {}

RhymeLikes::RhymeLikes(const std::filesystem::path& db_path)
    : db_path_(db_path) {
    if (const auto parent = db_path_.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    SQLite::Database db(db_path_, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    InitializeDatabase(db);
}

int RhymeLikes::UpdateScore(
    std::string_view request_word,
    int request_stress,
    std::string_view rhyme_word,
    int rhyme_stress,
    int delta) {
    auto pair = std::make_tuple(
        std::string(request_word), request_stress,
        std::string(rhyme_word), rhyme_stress);
    auto reverse_pair = std::make_tuple(
        std::string(rhyme_word), rhyme_stress,
        std::string(request_word), request_stress);
    if (reverse_pair < pair) {
        std::swap(pair, reverse_pair);
    }

    const auto& [first_word, first_stress, second_word, second_stress] = pair;
    std::lock_guard lock(mutex_);
    SQLite::Database db(db_path_, SQLite::OPEN_READWRITE);
    SQLite::Transaction transaction(db);

    SQLite::Statement insert(db, R"sql(
        INSERT OR IGNORE INTO rhyme_likes (
            request_word, request_stress, rhyme_word, rhyme_stress, score
        ) VALUES (?, ?, ?, ?, 0)
    )sql");
    insert.bind(1, first_word);
    insert.bind(2, first_stress);
    insert.bind(3, second_word);
    insert.bind(4, second_stress);
    insert.exec();

    SQLite::Statement update(db, R"sql(
        UPDATE rhyme_likes
        SET score = score + ?, updated_at = CURRENT_TIMESTAMP
        WHERE request_word = ? AND request_stress = ?
          AND rhyme_word = ? AND rhyme_stress = ?
    )sql");
    update.bind(1, delta);
    update.bind(2, first_word);
    update.bind(3, first_stress);
    update.bind(4, second_word);
    update.bind(5, second_stress);
    update.exec();

    SQLite::Statement select(db, R"sql(
        SELECT score FROM rhyme_likes
        WHERE request_word = ? AND request_stress = ?
          AND rhyme_word = ? AND rhyme_stress = ?
    )sql");
    select.bind(1, first_word);
    select.bind(2, first_stress);
    select.bind(3, second_word);
    select.bind(4, second_stress);
    if (!select.executeStep()) {
        throw std::runtime_error("failed to read updated rhyme score");
    }

    const int score = select.getColumn(0).getInt();
    transaction.commit();
    return score;
}

} // namespace ryfmach::bel
