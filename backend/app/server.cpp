#include "log.hpp"
#include "server.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <utility>

namespace ryfmach::app {
namespace {

using json = nlohmann::json;

std::string GetApiLogPath() {
    const char kDefaultLogPath[] = "";
    const char* path = std::getenv("RYFMACH_API_LOG_DIR");
    return (path ? path : kDefaultLogPath);
}

ryfmach::bel::RhymeSearchFilters FiltersFromJson(const json& j) {
    using namespace ryfmach::bel;
    RhymeSearchFilters filters;

    int mistake_id = j.value("search_mistake", -1);
    switch (mistake_id) {
        case -1: filters.mistake = SearchMistakeLevel::kAdaptive; break;
        case 0:  filters.mistake = SearchMistakeLevel::kIdeal; break;
        case 1:  filters.mistake = SearchMistakeLevel::kGood; break;
        case 2:  filters.mistake = SearchMistakeLevel::kMedium; break;
        case 3:  filters.mistake = SearchMistakeLevel::kWeak; break;
    }

    filters.only_initial = j.value("only_initial", false);
    if (j.contains("filtered_posp")) {
        const json& part_of_speech = j.at("filtered_posp");
        if (!part_of_speech.is_array() ||
            part_of_speech.size() != filters.part_of_speech.size()) {
            throw json::type_error::create(
                302,
                "filtered_posp must be an array of 7 booleans",
                &part_of_speech);
        }
        for (std::size_t index = 0; index < filters.part_of_speech.size();
             ++index) {
            filters.part_of_speech[index] = part_of_speech.at(index).get<bool>();
        }
    }

    return filters;
}

json WordRecordToJson(const bel::WordRecord& word) {
    if (word.id == 0) {
        return {
            {"word", word.word},
            {"accent", word.accent},
        };
    }

    json result = {
        {"id", word.id},
        {"word", word.word},
        {"initial_id", word.initial_id},
        {"part_of_speech_id", word.part_of_speech_id},
        {"part_of_speech", word.part_of_speech},
        {"accent", word.accent},
        {"is_initial", word.is_initial},
    };

    if (word.initial_word) {
        result["initial_word"] = *word.initial_word;
    }
    if (word.initial_accent) {
        result["initial_accent"] = *word.initial_accent;
    }

    return result;
}

json RhymeWordVariantToJson(const RhymeWordVariant& variant) {
    json result = {
        {"word", variant.dictionary_entry.word},
        {"accent", variant.dictionary_entry.accent},
        {"exact_match", variant.exact_match},
        {"dictionary_entry", WordRecordToJson(variant.dictionary_entry)},
    };
    if (variant.dictionary_entry.id != 0) {
        result["dictionary_id"] = variant.dictionary_entry.id;
    }
    return result;
}

json RhymesResultToJson(const RhymesResult& result) {
    if (result.status == RhymeResolutionStatus::kNotFound) {
        return {{"status", "not_found"}};
    }

    if (result.status == RhymeResolutionStatus::kNeedsChoice) {
        json variants = json::array();
        for (const auto& variant : result.variants) {
            variants.push_back(RhymeWordVariantToJson(variant));
        }
        return {
            {"status", "needs_choice"},
            {"variants", std::move(variants)},
        };
    }

    json rhymes = json::array();
    for (const auto& rhyme : result.rhymes) {
        rhymes.push_back(WordRecordToJson(rhyme));
    }

    return {
        {"status", "resolved"},
        {"selected_variant", result.selected_variant
            ? RhymeWordVariantToJson(*result.selected_variant)
            : json(nullptr)},
        {"rhymes_data", std::move(rhymes)},
    };
}

json SoundsToJson(std::span<const bel::Sound> sounds) {
    json result = json::array();
    for (const bel::Sound sound : sounds) {
        result.push_back(bel::SoundSpelling(sound));
    }
    return result;
}

json PhoneticAnalysisToJson(const PhoneticAnalysis& analysis) {
    json letter_map = json::array();
    for (const auto& mapping : analysis.letter_map) {
        json mapped_letters = json::array();
        mapped_letters.push_back(mapping.letters);
        mapped_letters.push_back(mapping.sounds);
        letter_map.push_back(std::move(mapped_letters));
    }

    json phenomena = json::array();
    for (const auto& occurrence : analysis.phenomena) {
        json phenomenon = json::array();
        phenomenon.push_back(occurrence.sound_index);
        phenomenon.push_back(SoundsToJson(occurrence.transcription));
        phenomenon.push_back(static_cast<int>(occurrence.phenomenon));
        phenomena.push_back(std::move(phenomenon));
    }

    return {
        {"word_variant", WordRecordToJson(analysis.word_variant)},
        {"letter_map", std::move(letter_map)},
        {"transcription", SoundsToJson(analysis.transcription)},
        {"phenomena", std::move(phenomena)},
        {"sound_analysis", analysis.sound_analysis},
    };
}

json PhoneticsResultToJson(const PhoneticsResult& result) {
    json word_variants = json::array();
    for (const auto& analysis : result.word_variants) {
        word_variants.push_back(PhoneticAnalysisToJson(analysis));
    }

    return {
        {"word_variants", std::move(word_variants)},
        {"word_found", result.word_found},
    };
}

json MorphemicsResultToJson(const MorphemicsResult& result) {
    json variants = json::array();
    for (const auto& variant : result.variants) {
        json analysis = json::array();
        for (const auto& morpheme : variant.analysis) {
            analysis.push_back({
                {"type", static_cast<int>(morpheme.type)},
                {"text", morpheme.text},
            });
        }
        variants.push_back({
            {"analysis", std::move(analysis)},
            {"sure", variant.sure},
        });
    }

    return {
        {"variants", std::move(variants)},
        {"word_found", result.word_found},
    };
}

void WriteJson(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void LogTopRhymes(const RhymesResult& result, size_t top_k) {
    for (const auto& word : result.rhymes_list) {
        std::string top_rhymes;
        for (int i = 0; i < top_k && i < (int)word.rhymes.size(); ++i) {
            top_rhymes += word.rhymes[i].word + "  ";
        }
        rhymes_log.debug("Rhymes to word \"{}\" ({}, {}):  {} rhymes:  {}",
            word.word_variant.word,
            word.word_variant.accent,
            word.word_variant.part_of_speech,
            word.rhymes.size(),
            top_rhymes
        );
    }
}

void HandleRhymesRequest(
    const httplib::Request& req,
    httplib::Response& res,
    const RyfmachService& service) {
    try {
        const json request = json::parse(req.body);
        const std::string word = request.value("word", std::string{});
        if (word.empty()) {
            WriteJson(res, 200, RhymesResultToJson({}));
            return;
        }
        ryfmach::bel::RhymeSearchFilters filters = FiltersFromJson(request);

        RhymesResult result;

        if (request.contains("dictionary_id")) {
            const std::size_t accent = request.at("accent").get<std::size_t>();
            const int dictionary_id = request.at("dictionary_id").get<int>();
            common_log.info("Rhymes to \"{}\", {}, id={}", word, accent, dictionary_id);
            result = service.FindRhymes(word, accent, dictionary_id, filters);
        } else if (request.contains("accent")) {
            const std::size_t accent = request.at("accent").get<std::size_t>();
            common_log.info("Rhymes to \"{}\", {} ", word, accent);
            result = service.FindRhymes(word, accent, filters);
        } else {
            common_log.info("Rhymes to \"{}\"", word);
            result = service.FindRhymes(word, filters);
        }

        LogTopRhymes(result, 5);
        WriteJson(res, 200, RhymesResultToJson(result));
    } catch (const json::exception&) {
        common_log.error("Failed to find rhymes: wrong format");
        WriteJson(
            res,
            400,
            {{"error", "Request body must be JSON with a word string."}});
    } catch (const std::exception& exception) {
        common_log.error("Failed to find rhymes: {}", exception.what());
        WriteJson(res, 500, {{"error", "Unable to find rhymes."}});
    }
}

std::string JoinTranscription(
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

void LogTranscription(const PhoneticsResult& result) {
    for (const auto& word : result.word_variants) {
        std::string transcr = JoinTranscription(word.transcription);
        phonetics_log.debug("Transcription of \"{}\" ({}, {}): {}",
            word.word_variant.word,
            word.word_variant.accent,
            word.word_variant.part_of_speech,
            transcr
        );
    }
}

void HandlePhoneticsRequest(
    const httplib::Request& req,
    httplib::Response& res,
    const RyfmachService& service) {
    try {
        const json request = json::parse(req.body);
        if (!request.contains("word") || request.at("word").is_null()) {
            WriteJson(
                res,
                200,
                {{"word_variants", json::array()}, {"word_found", false}});
            return;
        }

        PhoneticsResult result;

        const std::string word = request.at("word").get<std::string>();
        if (request.contains("accent") && !request.at("accent").is_null()) {
            const std::size_t accent = request.at("accent").get<std::size_t>();
            common_log.info("Phonetics \"{}\", {} ", word, accent);
            result = service.AnalyzePhonetics(word, accent);
        } else {
            common_log.info("Phonetics \"{}\"", word);
            result = service.AnalyzePhonetics(word);
        }

        LogTranscription(result);
        WriteJson(res, 200, PhoneticsResultToJson(service.AnalyzePhonetics(word)));

    } catch (const json::exception&) {
        common_log.error("Failed to analyze phonetics: wrong format");
        WriteJson(
            res,
            400,
            {{"error", "Request body must be JSON with a word string."}});
    } catch (const std::exception& exception) {
        common_log.error("Failed to analyze phonetics: {}", exception.what());
        WriteJson(res, 500, {{"error", "Unable to analyze phonetics."}});
    }
}

void LogMorphems(const MorphemicsResult& result) {
    for (const auto& word : result.variants) {
        std::string morphems = EncodeMorphemicAnalysis(word.analysis);
        if (!word.sure) {
            morphems += " (NOT SURE)";
        }
        morphemics_log.debug("Morphems: {}", morphems);
    }
}

void HandleMorphemicsRequest(
    const httplib::Request& req,
    httplib::Response& res,
    const RyfmachService& service) {
    try {
        const json request = json::parse(req.body);
        if (!request.contains("word") || request.at("word").is_null()) {
            WriteJson(
                res,
                200,
                {{"variants", json::array()}, {"word_found", false}});
            return;
        }
        
        const std::string word = request.at("word").get<std::string>();
        common_log.info("Morphemics for \"{}\"", word);
        MorphemicsResult result = service.AnalyzeMorphemics(word);
        LogMorphems(result);
        WriteJson(res, 200, MorphemicsResultToJson(result));
    } catch (const json::exception&) {
        common_log.error("Failed to analyze morphemics: wrong format");
        WriteJson(
            res,
            400,
            {{"error", "Request body must be JSON with a word string."}});
    } catch (const std::exception& exception) {
        common_log.error("Failed to analyze morphemics: {}", exception.what());
        WriteJson(res, 500, {{"error", "Unable to analyze morphemics."}});
    }
}

struct RhymeLikeWord {
    std::string word;
    std::size_t stress;
};

struct RhymeLikeRequest {
    RhymeLikeWord target;
    RhymeLikeWord rhyme;
};

RhymeLikeWord ParseRhymeLikeWord(const json& j) {
    RhymeLikeWord liked_word;
    liked_word.word = j.at("word").get<std::string>();
    liked_word.stress = j.at("stress").get<int>();
    if (liked_word.word.empty()) {
        throw json::type_error::create(302, "rhyme words must not be empty", &j);
    }
    return liked_word;
}

RhymeLikeRequest ParseRhymeLikeRequest(const json& j) {
    RhymeLikeRequest request;
    request.target = ParseRhymeLikeWord(j.at("request"));
    request.rhyme = ParseRhymeLikeWord(j.at("rhyme"));
    return request;
}

void HandleRhymeLikeRequest(
    const httplib::Request& req,
    httplib::Response& res,
    RyfmachService& service,
    int delta) {
    try {
        RhymeLikeRequest parsed = ParseRhymeLikeRequest(json::parse(req.body));

        WriteJson(res, 200, {{"score", service.UpdateRhymeLikeScore(
            parsed.target.word, parsed.target.stress,
            parsed.rhyme.word, parsed.rhyme.stress,
            delta)}});
    } catch (const json::exception& exception) {
        common_log.error("Invalid like payload");
        WriteJson(res, 400, {{"error", "Invalid like payload"}});
    } catch (const std::exception& exception) {
        common_log.error("Failed to update rhyme score: {}", exception.what());
        WriteJson(res, 500, {{"error", "Unable to update rhyme score."}});
    }
}

std::string GetRemoteAddr(const httplib::Request& req) {
    auto remote_addr_it = req.headers.find("REMOTE_ADDR");
    std::string remote_addr = (remote_addr_it == req.headers.end()
        ? "0.0.0.0"
        : remote_addr_it->second);
    
    return remote_addr;
}

} // namespace

RyfmachServer::RyfmachServer(RyfmachService& service)
    : service_(service) {
    InitializeLoggers(GetApiLogPath(), max_log_size_, max_log_files_);
}

bool RyfmachServer::Listen(std::string_view host, int port) const {
    httplib::Server server;

    server.Get(
        "/health",
        [](const httplib::Request& /*req*/, httplib::Response& res) {
            WriteJson(res, 200, {{"ok", true}});
        });

    server.Post(
        "/api/rhymes",
        [this](const httplib::Request& req, httplib::Response& res) {
            std::string remote_addr = GetRemoteAddr(req);
            common_log.info("[{}] POST /api/rhymes", remote_addr);
            auto start_time = bench_clock_.now();

            HandleRhymesRequest(req, res, service_);

            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                bench_clock_.now() - start_time);
            common_log.info("[{}] /api/rhymes: responded in {}ms", remote_addr, diff.count());
        });

    server.Post(
        "/api/phonetics",
        [this](const httplib::Request& req, httplib::Response& res) {
            std::string remote_addr = GetRemoteAddr(req);
            common_log.info("[{}] POST /api/phonetics", remote_addr);
            auto start_time = bench_clock_.now();

            HandlePhoneticsRequest(req, res, service_);

            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                bench_clock_.now() - start_time);
            common_log.info("[{}] /api/phonetics: responded in {}ms", remote_addr, diff.count());
        });

    server.Post(
        "/api/morphemics",
        [this](const httplib::Request& req, httplib::Response& res) {
            std::string remote_addr = GetRemoteAddr(req);
            common_log.info("[{}] POST /api/morphemics", remote_addr);
            auto start_time = bench_clock_.now();

            HandleMorphemicsRequest(req, res, service_);

            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                bench_clock_.now() - start_time);
            common_log.info("[{}] /api/morphemics: responded in {}ms", remote_addr, diff.count());
        });

    server.Post(
        "/api/rhyme/like",
        [this](const httplib::Request& req, httplib::Response& res) {
            std::string remote_addr = GetRemoteAddr(req);
            common_log.info("[{}] POST /api/rhyme/like", remote_addr);
            auto start_time = bench_clock_.now();

            HandleRhymeLikeRequest(req, res, service_, 1);

            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                bench_clock_.now() - start_time);
            common_log.info("[{}] /api/rhyme/like: responded in {}ms", remote_addr, diff.count());
        });

    server.Post(
        "/api/rhyme/dislike",
        [this](const httplib::Request& req, httplib::Response& res) {
            std::string remote_addr = GetRemoteAddr(req);
            common_log.info("[{}] POST /api/rhyme/dislike", remote_addr);
            auto start_time = bench_clock_.now();
            
            HandleRhymeLikeRequest(req, res, service_, -1);

            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                bench_clock_.now() - start_time);
            common_log.info("[{}] /api/rhyme/dislike: responded in {}ms", remote_addr, diff.count());
        });

    return server.listen(std::string(host), port);
}

RyfmachServer::~RyfmachServer() {
    spdlog::shutdown();
}

} // namespace ryfmach::app
