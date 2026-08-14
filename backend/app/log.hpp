#ifndef RYFMACH_APP_LOG_HPP_
#define RYFMACH_APP_LOG_HPP_

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include <filesystem>

namespace ryfmach::app {

namespace fs = std::filesystem;

extern spdlog::logger common_log;
extern spdlog::logger rhymes_log;
extern spdlog::logger phonetics_log;
extern spdlog::logger morphemics_log;

void InitializeLoggers(const fs::path&, size_t max_size, size_t max_files);

} // namespace ryfmach::app

#endif // RYFMACH_APP_SERVER_HPP_
