#ifndef RYFMACH_APP_RYFMACH_SERVICE_HPP_
#define RYFMACH_APP_RYFMACH_SERVICE_HPP_

#include "slounik.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace ryfmach::app {

struct RhymeGroup {
    bel::WordRecord word_variant;
    std::vector<bel::WordRecord> rhymes;
};

struct RhymesResult {
    bool word_found = false;
    std::vector<RhymeGroup> rhymes_list;
};

class RyfmachService {
public:
    explicit RyfmachService(const bel::Slounik& slounik);

    RhymesResult FindRhymes(std::string_view word) const;
    RhymesResult FindRhymes(std::string_view word, std::size_t accent) const;

private:
    RhymeGroup FindRhymesForVariant(const bel::WordRecord& word_variant) const;

    const bel::Slounik& slounik_;
};

} // namespace ryfmach::app

#endif // RYFMACH_APP_RYFMACH_SERVICE_HPP_
