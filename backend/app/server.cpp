#include "server.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <utility>

namespace ryfmach::app {
namespace {

using json = nlohmann::json;

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

json RhymesResultToJson(const RhymesResult& result) {
    json rhyme_groups = json::array();
    for (const auto& rhyme_group : result.rhymes_list) {
        json rhymes = json::array();
        for (const auto& rhyme : rhyme_group.rhymes) {
            rhymes.push_back(WordRecordToJson(rhyme));
        }

        rhyme_groups.push_back({
            {"word_variant", WordRecordToJson(rhyme_group.word_variant)},
            {"rhymes_data", std::move(rhymes)},
        });
    }

    return {
        {"rhymes_list", std::move(rhyme_groups)},
        {"word_found", result.word_found},
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

void HandleRhymesRequest(
    const httplib::Request& req,
    httplib::Response& res,
    const RyfmachService& service) {
    try {
        const json request = json::parse(req.body);
        const std::string word = request.value("word", std::string{});
        if (word.empty()) {
            WriteJson(
                res,
                200,
                {{"rhymes_list", json::array()}, {"word_found", false}});
            return;
        }

        if (request.contains("accent")) {
            const std::size_t accent = request.at("accent").get<std::size_t>();
            WriteJson(res, 200, RhymesResultToJson(service.FindRhymes(word, accent)));
        } else {
            WriteJson(res, 200, RhymesResultToJson(service.FindRhymes(word)));
        }
    } catch (const json::exception&) {
        WriteJson(
            res,
            400,
            {{"error", "Request body must be JSON with a word string."}});
    } catch (const std::exception& exception) {
        std::cerr << "failed to find rhymes: " << exception.what() << '\n';
        WriteJson(res, 500, {{"error", "Unable to find rhymes."}});
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

        const std::string word = request.at("word").get<std::string>();
        if (request.contains("accent") && !request.at("accent").is_null()) {
            const std::size_t accent = request.at("accent").get<std::size_t>();
            WriteJson(
                res,
                200,
                PhoneticsResultToJson(service.AnalyzePhonetics(word, accent)));
        } else {
            WriteJson(res, 200, PhoneticsResultToJson(service.AnalyzePhonetics(word)));
        }
    } catch (const json::exception&) {
        WriteJson(
            res,
            400,
            {{"error", "Request body must be JSON with a word string."}});
    } catch (const std::exception& exception) {
        std::cerr << "failed to analyze phonetics: " << exception.what() << '\n';
        WriteJson(res, 500, {{"error", "Unable to analyze phonetics."}});
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
        WriteJson(res, 200, MorphemicsResultToJson(service.AnalyzeMorphemics(word)));
    } catch (const json::exception&) {
        WriteJson(
            res,
            400,
            {{"error", "Request body must be JSON with a word string."}});
    } catch (const std::exception& exception) {
        std::cerr << "failed to analyze morphemics: " << exception.what() << '\n';
        WriteJson(res, 500, {{"error", "Unable to analyze morphemics."}});
    }
}

} // namespace

RyfmachServer::RyfmachServer(const RyfmachService& service)
    : service_(service) {}

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
            std::cout << "/api/rhymes\n";
            HandleRhymesRequest(req, res, service_);
        });

    server.Post(
        "/api/phonetics",
        [this](const httplib::Request& req, httplib::Response& res) {
            std::cout << "/api/phonetics\n";
            HandlePhoneticsRequest(req, res, service_);
        });

    server.Post(
        "/api/morphemics",
        [this](const httplib::Request& req, httplib::Response& res) {
            HandleMorphemicsRequest(req, res, service_);
        });

    return server.listen(std::string(host), port);
}

} // namespace ryfmach::app
