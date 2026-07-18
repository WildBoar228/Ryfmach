#ifndef RYFMACH_BEL_LANG_ENGINE_INCLUDE_RHYME_LIKES_HPP_
#define RYFMACH_BEL_LANG_ENGINE_INCLUDE_RHYME_LIKES_HPP_

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <string_view>

namespace ryfmach::bel {

class RhymeLikes {
public:
    RhymeLikes();
    explicit RhymeLikes(const std::filesystem::path& db_path);

    int UpdateScore(
        std::string_view request_word,
        int request_stress,
        std::string_view rhyme_word,
        int rhyme_stress,
        int delta);

private:
    std::filesystem::path db_path_;
    std::mutex mutex_;
};

} // namespace ryfmach::bel

#endif // RYFMACH_BEL_LANG_ENGINE_INCLUDE_RHYME_LIKES_HPP_
