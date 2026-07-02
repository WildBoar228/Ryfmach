#pragma once

#include "sounds.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace ryfmach::bel {

std::optional<std::string> WorkingPart(
    std::string_view word,
    std::size_t stress,
    int mistake = 0);

std::optional<std::uint64_t> SoundHash(
    std::string_view word,
    std::size_t stress,
    int mistake = 0);

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

double ReplaceCost(Sound left, Sound right) noexcept;
double ReplaceCost(
    Sound left,
    Sound right,
    const SoundCompatibilityTable& table) noexcept;
double ReplaceCost(
    std::optional<Sound> left,
    std::optional<Sound> right) noexcept;
double ReplaceCost(
    std::optional<Sound> left,
    std::optional<Sound> right,
    const SoundCompatibilityTable& table) noexcept;

} // namespace ryfmach::bel
