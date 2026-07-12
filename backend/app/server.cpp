#include "server.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
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
        "/rhymes",
        [this](const httplib::Request& req, httplib::Response& res) {
            HandleRhymesRequest(req, res, service_);
        });

    return server.listen(std::string(host), port);
}

} // namespace ryfmach::app
