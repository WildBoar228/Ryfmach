#ifndef RYFMACH_BEL_LANG_ENGINE_INCLUDE_RHYMES_HPP_
#define RYFMACH_BEL_LANG_ENGINE_INCLUDE_RHYMES_HPP_

#include "sounds.hpp"
#include "utils/matrix_span.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ryfmach::bel {

enum class RhymeMistakeLevel {
    kIdeal,
    kGood,
    kMedium,
    kWeak
};

std::optional<std::string> CalcWorkingPart(
    std::string_view word,
    std::size_t stress,
    RhymeMistakeLevel mistake = RhymeMistakeLevel::kIdeal);

std::optional<std::uint64_t> SoundHash(
    std::string_view word,
    std::size_t stress,
    RhymeMistakeLevel mistake = RhymeMistakeLevel::kIdeal);

class SoundCompatibilityTable {
public:
    static constexpr std::size_t kSoundKeyCount = 52;

    SoundCompatibilityTable();

    double Cost(Sound left, Sound right) const noexcept;
    double Cost(
        std::optional<Sound> left,
        std::optional<Sound> right) const noexcept;

    void SetCost(Sound left, Sound right, double cost) noexcept;
    void SetCost(
        std::optional<Sound> left,
        std::optional<Sound> right,
        double cost) noexcept;

private:
    std::array<double, kSoundKeyCount * kSoundKeyCount> costs_;
};

std::optional<SoundCompatibilityTable> LoadSoundCompatibilityTable(
    const std::filesystem::path& path);

const SoundCompatibilityTable& DefaultSoundCompatibilityTable() noexcept;

double SoundReplaceCost(Sound left, Sound right) noexcept;

double SoundReplaceCost(
    Sound left,
    Sound right,
    const SoundCompatibilityTable& table) noexcept;

double SoundReplaceCost(
    std::optional<Sound> left,
    std::optional<Sound> right) noexcept;

double SoundReplaceCost(
    std::optional<Sound> left,
    std::optional<Sound> right,
    const SoundCompatibilityTable& table) noexcept;

using RhymeCostType = std::pair<double, double>;

RhymeCostType CalcRhymeCost(
    std::span<const Sound> t1,
    std::span<const Sound> t2,
    int max_shift);

} // namespace ryfmach::bel

#endif // RYFMACH_BEL_LANG_ENGINE_INCLUDE_RHYMES_HPP_
