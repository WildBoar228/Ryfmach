#include "rhymes.hpp"
#include "slounik.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ryfmach::bel {
namespace {

constexpr char kSlounikDbPathVariable[] = "SLOUNIK_DB_PATH";

std::filesystem::path GetSlounikDbPathFromEnvironment() {
    const char* path = std::getenv(kSlounikDbPathVariable);
    if (path == nullptr || std::string_view(path).empty()) {
        throw std::runtime_error("SLOUNIK_DB_PATH is not set");
    }

    return std::filesystem::path(path);
}

bool StartsWith(std::string_view text, std::string_view prefix) noexcept {
    return text.size() >= prefix.size() &&
           text.substr(0, prefix.size()) == prefix;
}

std::size_t Utf8CodePointCount(std::string_view text) noexcept {
    std::size_t count = 0;
    for (const unsigned char ch : text) {
        if ((ch & 0xC0) != 0x80) {
            ++count;
        }
    }
    return count;
}

struct SimilarLetterMatch {
    std::size_t old_byte_count = 0;
    std::string_view replacement;
};

std::optional<SimilarLetterMatch> SimilarLetterReplacement(
    std::string_view word_suffix) noexcept {
    if (StartsWith(word_suffix, "у")) {
        return SimilarLetterMatch{std::string_view("у").size(), "ў"};
    }
    if (StartsWith(word_suffix, "ў")) {
        return SimilarLetterMatch{std::string_view("ў").size(), "у"};
    }
    if (StartsWith(word_suffix, "е")) {
        return SimilarLetterMatch{std::string_view("е").size(), "ё"};
    }
    if (StartsWith(word_suffix, "ё")) {
        return SimilarLetterMatch{std::string_view("ё").size(), "е"};
    }
    return std::nullopt;
}

std::size_t ReplacedLetterByteCount(std::string_view word_suffix) noexcept {
    if (const auto match = SimilarLetterReplacement(word_suffix)) {
        return match->old_byte_count;
    }
    return 1;
}

std::size_t SimilarLetterReplacementCount(std::string_view word) noexcept {
    std::size_t replacement_count = 0;
    for (std::size_t offset = 0; offset < word.size();) {
        if (SimilarLetterReplacement(word.substr(offset)).has_value()) {
            ++replacement_count;
        }
        offset += ReplacedLetterByteCount(word.substr(offset));
    }
    return replacement_count;
}

std::vector<std::string> SimilarLetterVariants(
    std::string_view word,
    std::size_t max_replacements) {
    if (SimilarLetterReplacementCount(word) > max_replacements) {
        return {std::string(word)};
    }

    std::vector<std::string> variants = {""};

    for (std::size_t offset = 0; offset < word.size();) {
        const std::size_t token_size = ReplacedLetterByteCount(word.substr(offset));
        const std::string_view token = word.substr(offset, token_size);
        const auto replacement = SimilarLetterReplacement(word.substr(offset));

        const std::size_t original_size = variants.size();
        for (std::size_t index = 0; index < original_size; ++index) {
            variants[index].append(token);
        }

        if (replacement.has_value()) {
            for (std::size_t index = 0; index < original_size; ++index) {
                std::string variant = variants[index];
                variant.resize(variant.size() - token.size());
                variant.append(replacement->replacement);
                variants.push_back(std::move(variant));
            }
        }

        offset += token.size();
    }

    return variants;
}

std::string BuildPlaceholders(std::size_t count) {
    std::string placeholders;
    for (std::size_t index = 0; index < count; ++index) {
        if (index != 0) {
            placeholders += ", ";
        }
        placeholders += "?";
    }
    return placeholders;
}

struct WordVariantKey {
    std::string word;
    int initial_id = 0;
    std::size_t accent = 0;

    friend bool operator==(
        const WordVariantKey& left,
        const WordVariantKey& right) noexcept {
        return left.word == right.word && left.initial_id == right.initial_id &&
               left.accent == right.accent;
    }
};

struct WordVariantKeyHash {
    std::size_t operator()(const WordVariantKey& key) const noexcept {
        std::size_t hash = std::hash<std::string>{}(key.word);
        hash ^= std::hash<int>{}(key.initial_id) + 0x9e3779b9 + (hash << 6) +
                (hash >> 2);
        hash ^= std::hash<std::size_t>{}(key.accent) + 0x9e3779b9 + (hash << 6) +
                (hash >> 2);
        return hash;
    }
};

using WordVariantSet = std::unordered_set<WordVariantKey, WordVariantKeyHash>;

WordVariantKey KeyFromWord(const WordRecord& word) {
    return WordVariantKey{
        .word = word.word,
        .initial_id = word.initial_id,
        .accent = word.accent,
    };
}

void AppendUniqueWord(
    std::vector<WordRecord>& words,
    WordVariantSet& used_words,
    WordRecord word) {
    if (used_words.insert(KeyFromWord(word)).second) {
        words.push_back(std::move(word));
    }
}

} // namespace

class Slounik::Impl {
public:
    explicit Impl(const std::filesystem::path& db_path)
        : db_(db_path, SQLite::OPEN_READONLY) {}

    std::vector<WordRecord> FindWords(
        std::string_view word,
        WordLookupOptions options) const {
        std::vector<WordRecord> words;
        WordVariantSet used_words;
        used_words.reserve(256);

        if (options.include_compound_tail) {
            const auto dash_position = word.rfind('-');
            if (dash_position != std::string_view::npos &&
                dash_position + 1 < word.size()) {
                WordLookupOptions tail_options = options;
                tail_options.include_compound_tail = false;

                const auto prefix = word.substr(0, dash_position);
                const auto tail = word.substr(dash_position + 1);
                auto tail_words = FindWords(tail, tail_options);
                const std::size_t accent_shift =
                    Utf8CodePointCount(word.substr(0, dash_position + 1));

                for (auto& tail_word : tail_words) {
                    tail_word.word =
                        std::string(prefix) + "-" + std::move(tail_word.word);
                    tail_word.accent += accent_shift;
                    AppendUniqueWord(words, used_words, std::move(tail_word));
                }
            }
        }

        for (auto& found_word : FindWordsInTable(word, options)) {
            AppendUniqueWord(words, used_words, std::move(found_word));
        }

        return words;
    }

    std::optional<WordRecord> GetWordById(int id) const {
        std::lock_guard lock(mutex_);

        SQLite::Statement query(
            db_,
            WordRecordSelectSql() + R"sql(
                WHERE w.id = ?
            )sql");
        query.bind(1, id);

        if (!query.executeStep()) {
            return std::nullopt;
        }

        return WordFromRow(query);
    }

    std::vector<WordRecord> GetWordForms(int initial_id) const {
        std::lock_guard lock(mutex_);

        SQLite::Statement query(
            db_,
            WordRecordSelectSql() + R"sql(
                WHERE w.initial_id = ?
                ORDER BY w.word, w.initial_id, w.accent_index
            )sql");
        query.bind(1, initial_id);

        std::vector<WordRecord> words;
        WordVariantSet used_words;
        used_words.reserve(64);
        while (query.executeStep()) {
            AppendUniqueWord(words, used_words, WordFromRow(query));
        }

        return words;
    }

    std::vector<std::string> FindMorphemicAnalyses(
        std::string_view word,
        bool fix_similar_letters
    ) const {
        const auto variants = fix_similar_letters
                                  ? SimilarLetterVariants(word, 5)
                                  : std::vector<std::string>{std::string(word)};

        std::lock_guard lock(mutex_);
        SQLite::Statement query(
            db_,
            "SELECT * FROM morphemics WHERE word IN (" +
                BuildPlaceholders(variants.size()) + ")");
        for (std::size_t index = 0; index < variants.size(); ++index) {
            query.bind(static_cast<int>(index + 1), variants[index]);
        }

        std::vector<std::string> analyses;
        while (query.executeStep()) {
            analyses.push_back(query.getColumn(2).getString());
        }
        return analyses;
    }

    std::vector<MorphemicPrefixRecord> GetMorphemicPrefixes() const {
        std::lock_guard lock(mutex_);
        if (morphemic_prefixes_) {
            return *morphemic_prefixes_;
        }

        SQLite::Statement query(db_, "SELECT * FROM morph_prefixes");

        std::vector<MorphemicPrefixRecord> prefixes;
        while (query.executeStep()) {
            prefixes.push_back(MorphemicPrefixRecord{
                .text = query.getColumn(1).getString(),
                .analysis = query.getColumn(2).getString(),
            });
        }
        std::stable_sort(
            prefixes.begin(),
            prefixes.end(),
            [](const MorphemicPrefixRecord& left,
               const MorphemicPrefixRecord& right) {
                return left.text.size() > right.text.size();
            });
        morphemic_prefixes_ = std::move(prefixes);
        return *morphemic_prefixes_;
    }

    std::vector<Rhyme> FindRhymesAdaptive(
        const WordRecord& input_word,
        const RhymeSearchFilters& filters,
        std::size_t min_adaptive_cnt,
        std::size_t max_cnt
    ) const {
        std::vector<Rhyme> rhymes;
        std::unordered_map<uint64_t, RhymeCostType> best_costs;
        size_t final_cnt = 0;

        for (auto iter_mistake : {RhymeMistakeLevel::kIdeal,
                                  RhymeMistakeLevel::kGood,
                                  RhymeMistakeLevel::kMedium,
                                  RhymeMistakeLevel::kWeak}) {
            std::vector<WordRecord> new_candidates = FilteredSearchBySoundHash(
                input_word,
                iter_mistake,
                filters, max_cnt);
            
            std::vector<Rhyme> new_rhymes = WordsToRhymesWithScore(
                input_word,
                std::move(new_candidates));

            final_cnt += SaveRhymeWordFamilyLeaders(new_rhymes, best_costs);
            
            rhymes.insert(
                rhymes.end(),
                std::make_move_iterator(new_rhymes.begin()),
                std::make_move_iterator(new_rhymes.end()));

            if (final_cnt >= min_adaptive_cnt) {
                break;
            }
        }

        final_cnt = CleanRhymeWordFamilies(rhymes, best_costs);
        rhymes.resize(final_cnt);

        return rhymes;
    }

    std::vector<Rhyme> FindRhymes(
        const WordRecord& input_word,
        RhymeMistakeLevel mistake,
        const RhymeSearchFilters& filters,
        std::size_t max_cnt
    ) const {
        std::vector<WordRecord> candidates = FilteredSearchBySoundHash(
            input_word, mistake, filters, max_cnt);
        std::vector<Rhyme> rhymes = WordsToRhymesWithScore(
            input_word,
            std::move(candidates));

        std::unordered_map<uint64_t, RhymeCostType> best_costs;
        (void)SaveRhymeWordFamilyLeaders(rhymes, best_costs);
        std::size_t new_size = CleanRhymeWordFamilies(rhymes, best_costs);

        SortRhymes(rhymes);

        return rhymes;
    }

private:
    std::string WordRecordSelectSql() const {
        return R"sql(
            SELECT
                w.id,
                w.word,
                w.initial_id,
                w.part_of_speech,
                w.accent_index,
                COALESCE(pos.name, ''),
                initial.word,
                initial.accent_index
            FROM words AS w
            LEFT JOIN parts_of_speech AS pos
                ON pos.id = w.part_of_speech
            LEFT JOIN words AS initial
                ON initial.id = w.initial_id
        )sql";
    }

    std::vector<WordRecord> FindWordsInTable(
        std::string_view word,
        WordLookupOptions options) const {
        const auto variants = options.fix_similar_letters
                                  ? SimilarLetterVariants(
                                        word,
                                        options.max_similar_letter_replacements)
                                  : std::vector<std::string>{std::string(word)};

        std::lock_guard lock(mutex_);

        SQLite::Statement query(
            db_,
            WordRecordSelectSql() +
                R"sql(
                WHERE w.word IN ()sql" +
                BuildPlaceholders(variants.size()) +
                R"sql()
                ORDER BY w.word, w.initial_id, w.accent_index
            )sql");

        for (std::size_t index = 0; index < variants.size(); ++index) {
            query.bind(static_cast<int>(index + 1), variants[index]);
        }

        std::vector<WordRecord> words;
        WordVariantSet used_words;
        used_words.reserve(variants.size());
        while (query.executeStep()) {
            AppendUniqueWord(words, used_words, WordFromRow(query));
        }

        return words;
    }

    WordRecord WordFromRow(const SQLite::Statement& row) const {
        WordRecord word;
        word.id = row.getColumn(0).getInt();
        word.word = row.getColumn(1).getString();
        word.initial_id = row.getColumn(2).getInt();
        word.part_of_speech_id = row.getColumn(3).getInt();
        word.accent = static_cast<std::size_t>(row.getColumn(4).getInt());
        word.part_of_speech = row.getColumn(5).getString();
        word.is_initial = word.id == word.initial_id;

        if (!word.is_initial && !row.getColumn(6).isNull()) {
            word.initial_word = row.getColumn(6).getString();
        }
        if (!word.is_initial && !row.getColumn(7).isNull()) {
            word.initial_accent =
                static_cast<std::size_t>(row.getColumn(7).getInt());
        }

        return word;
    }

    std::size_t SaveRhymeWordFamilyLeaders(
        std::span<const Rhyme> rhymes,
        std::unordered_map<std::uint64_t, RhymeCostType>& best_family_words
    ) const {
        std::size_t k = 0;
        for (std::size_t i = 0; i < rhymes.size(); ++i) {
            const Rhyme& rhyme = rhymes[i];
            auto iter = best_family_words.find(rhyme.initial_id);
            if (iter == best_family_words.end()) {
                best_family_words.emplace(rhyme.initial_id, rhyme.cost);
                ++k;
            } else {
                if (iter->second > rhyme.cost) {
                    iter->second = rhyme.cost;
                }
            }
        }
        return k;
    }

    std::size_t CleanRhymeWordFamilies(
        std::span<Rhyme> rhymes,
        std::unordered_map<std::uint64_t, RhymeCostType>& best_family_words
    ) const {
        std::size_t k = 0;
        for (std::size_t i = 0; i < rhymes.size(); ++i) {
            Rhyme& rhyme = rhymes[i];
            auto it = best_family_words.find(rhyme.initial_id);
            if (it != best_family_words.end() && rhyme.cost == it->second) {
                if (k != i) {
                    rhymes[k] = std::move(rhyme);
                }
                best_family_words.erase(it);
                ++k;
            }
        }
        return k;
    }

    std::vector<WordRecord> FilteredSearchBySoundHash(
        const WordRecord& input_word,
        RhymeMistakeLevel mistake,
        const RhymeSearchFilters& filters,
        std::size_t max_cnt
    ) const {
        std::lock_guard lock(mutex_);

        SQLite::Statement query(
            db_,
            WordRecordSelectSql() +
            "WHERE w.word != ? AND " +
            BuildSoundHashFilter(mistake) + 
            BuildRhymeFilter(filters) +
            R"sql(
                ORDER BY w.word, w.initial_id, w.accent_index
                LIMIT ?;
            )sql");

        int placehold_index = 1;
        query.bind(placehold_index++, input_word.word);
        for (auto iter_mistake : {RhymeMistakeLevel::kIdeal,
                                  RhymeMistakeLevel::kGood,
                                  RhymeMistakeLevel::kMedium,
                                  RhymeMistakeLevel::kWeak}) {
            auto sound_hash = SoundHash(input_word.word, input_word.accent, iter_mistake);
            if (!sound_hash) { throw std::runtime_error("Can't calculate sound hash"); }
            query.bind(placehold_index++, static_cast<int64_t>(*sound_hash));
            if (iter_mistake == mistake) { break; }
        }
        query.bind(placehold_index++, static_cast<int64_t>(max_cnt));

        std::vector<WordRecord> words;
        while (query.executeStep()) {
            WordRecord rec = WordFromRow(query);
            words.push_back(std::move(rec));
        }
        return words;
    }

    std::string BuildSoundHashFilter(RhymeMistakeLevel mistake) const {
        switch (mistake) {
            case RhymeMistakeLevel::kIdeal:
                return "w.sound_hash0 == ?";
            case RhymeMistakeLevel::kGood:
                return "w.sound_hash0 != ? AND w.sound_hash1 == ?";
            case RhymeMistakeLevel::kMedium:
                return R"(w.sound_hash0 != ? AND w.sound_hash1 != ? AND
                        w.sound_hash2 == ?)";
            case RhymeMistakeLevel::kWeak:
                return R"(w.sound_hash0 != ? AND w.sound_hash1 != ? AND
                        w.sound_hash2 != ? AND w.sound_hash3 == ?)";
        }
        return "";
    }

    std::string BuildRhymeFilter(const RhymeSearchFilters& filters) const {
        std::string result;
        if (filters.only_initial) {
            result += " AND w.id == w.initial_id ";
        }

        const bool has_selected_part_of_speech = std::ranges::any_of(
            filters.part_of_speech, [](bool selected) { return selected; });
        const bool has_unselected_part_of_speech = std::ranges::any_of(
            filters.part_of_speech, [](bool selected) { return !selected; });
        if (!has_selected_part_of_speech || !has_unselected_part_of_speech) {
            return result;
        }

        result += " AND w.part_of_speech IN (";
        bool first = true;
        for (std::size_t index = 0; index < filters.part_of_speech.size();
             ++index) {
            if (!filters.part_of_speech[index]) {
                continue;
            }
            if (!first) {
                result += ", ";
            }
            result += std::to_string(index + 1);
            first = false;
        }
        result += ") ";
        return result;
    }

    SQLite::Database db_;
    mutable std::mutex mutex_;
    mutable std::optional<std::vector<MorphemicPrefixRecord>> morphemic_prefixes_;
};

Slounik::Slounik() : Slounik(GetSlounikDbPathFromEnvironment()) {}

Slounik::Slounik(const std::filesystem::path& db_path)
    : impl_(std::make_unique<Impl>(db_path)) {}

Slounik::~Slounik() = default;

Slounik::Slounik(Slounik&&) noexcept = default;

Slounik& Slounik::operator=(Slounik&&) noexcept = default;

std::vector<WordRecord> Slounik::FindWords(
    std::string_view word,
    WordLookupOptions options) const {
    return impl_->FindWords(word, options);
}

std::optional<WordRecord> Slounik::GetWordById(int id) const {
    return impl_->GetWordById(id);
}

std::vector<WordRecord> Slounik::GetWordForms(int initial_id) const {
    return impl_->GetWordForms(initial_id);
}

std::vector<std::string> Slounik::FindMorphemicAnalyses(
    std::string_view word,
    bool fix_similar_letters
) const {
    return impl_->FindMorphemicAnalyses(word, fix_similar_letters);
}

std::vector<MorphemicPrefixRecord> Slounik::GetMorphemicPrefixes() const {
    return impl_->GetMorphemicPrefixes();
}

std::vector<Rhyme> Slounik::FindRhymes(
    const WordRecord& input_word,
    const RhymeSearchFilters& filters,
    std::size_t max_cnt
) const {
    RhymeMistakeLevel rhyme_mistake;
    switch (filters.mistake) {
        case SearchMistakeLevel::kAdaptive:
            return impl_->FindRhymesAdaptive(
                input_word,
                filters,
                std::numeric_limits<std::size_t>::max(),
                max_cnt);

        case SearchMistakeLevel::kIdeal:
            rhyme_mistake = RhymeMistakeLevel::kIdeal; break;

        case SearchMistakeLevel::kGood:
            rhyme_mistake = RhymeMistakeLevel::kGood; break;

        case SearchMistakeLevel::kMedium:
            rhyme_mistake = RhymeMistakeLevel::kMedium; break;

        case SearchMistakeLevel::kWeak:
            rhyme_mistake = RhymeMistakeLevel::kWeak; break;
    }

    return impl_->FindRhymes(
        input_word,
        rhyme_mistake,
        filters,
        max_cnt);
}

std::vector<Rhyme> Slounik::FindRhymes(
    std::string_view word,
    std::size_t accent,
    const RhymeSearchFilters& filters,
    std::size_t max_cnt
) const {
    const WordRecord input_word{
        .word = std::string(word),
        .accent = accent,
    };

    return FindRhymes(input_word, filters, max_cnt);
}

} // namespace ryfmach::bel
