#include "log.hpp"

namespace ryfmach::app {

spdlog::logger common_log("common");
spdlog::logger rhymes_log("rhymes");
spdlog::logger phonetics_log("phonetics");
spdlog::logger morphemics_log("morphemics");

void InitializeLoggers(const fs::path& log_path, size_t max_size, size_t max_files) {
    common_log.sinks().push_back(
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        
    if (log_path != "") {
        common_log.sinks().push_back(
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_path / "common.log", max_size, max_files));

        rhymes_log.sinks().push_back(
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    log_path / "rhymes.log", max_size, max_files));

        phonetics_log.sinks().push_back(
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    log_path / "phonetics.log", max_size, max_files));

        morphemics_log.sinks().push_back(
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    log_path / "morphemics.log", max_size, max_files));
    }

    common_log.set_level(spdlog::level::info);
    rhymes_log.set_level(spdlog::level::debug);
    phonetics_log.set_level(spdlog::level::debug);
    morphemics_log.set_level(spdlog::level::debug);

    common_log.flush_on(spdlog::level::info);
    rhymes_log.flush_on(spdlog::level::debug);
    phonetics_log.flush_on(spdlog::level::debug);
    morphemics_log.flush_on(spdlog::level::debug);

    common_log.set_pattern("[%Y-%m-%d %H:%M:%S] [%n] [th %t] [%l] %v");
    rhymes_log.set_pattern("[%Y-%m-%d %H:%M:%S] [%n] [th %t] [%l] %v");
    phonetics_log.set_pattern("[%Y-%m-%d %H:%M:%S] [%n] [th %t] [%l] %v");
    morphemics_log.set_pattern("[%Y-%m-%d %H:%M:%S] [%n] [th %t] [%l] %v");

    if (log_path == "") {
        common_log.warn("API logging directory is not set");
    }
}

} // namespace ryfmach::app
