#ifndef RYFMACH_BEL_LANG_ENGINE_INCLUDE_SLOUNIK_HPP_
#define RYFMACH_BEL_LANG_ENGINE_INCLUDE_SLOUNIK_HPP_

#include "rhyme_ranking.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ryfmach::bel {

struct Rhyme;

struct WordRecord {
    int id = 0;
    std::string word;
    int initial_id = 0;
    int part_of_speech_id = 0;
    std::string part_of_speech;
    std::size_t accent = 0;
    bool is_initial = false;
    std::optional<std::string> initial_word;
    std::optional<std::size_t> initial_accent;
};

struct WordLookupOptions {
    bool fix_similar_letters = false;
    bool include_compound_tail = true;
    std::size_t max_similar_letter_replacements = 5;
};

enum class SearchMistakeLevel {
    kAdaptive,
    kIdeal,
    kGood,
    kMedium,
    kWeak
};

struct RhymeSearchFilters {
    // TODO
};

class Slounik {
public:
    Slounik();
    explicit Slounik(const std::filesystem::path& db_path);
    ~Slounik();

    Slounik(const Slounik&) = delete;
    Slounik& operator=(const Slounik&) = delete;
    Slounik(Slounik&&) noexcept;
    Slounik& operator=(Slounik&&) noexcept;

    std::vector<WordRecord> FindWords(
        std::string_view word,
        WordLookupOptions options = {}) const;
    std::optional<WordRecord> GetWordById(int id) const;
    std::vector<WordRecord> GetWordForms(int initial_id) const;

    std::vector<Rhyme> FindRhymes(
        const WordRecord& input_word,
        SearchMistakeLevel,
        RhymeSearchFilters,
        std::size_t max_cnt
    ) const;

private:
    class Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace ryfmach::bel

#endif // RYFMACH_BEL_LANG_ENGINE_INCLUDE_SLOUNIK_HPP_
