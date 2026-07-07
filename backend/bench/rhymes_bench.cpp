#include "rhymes.hpp"
#include "slounik.hpp"
#include "transcription.hpp"

#include <benchmark/benchmark.h>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef RYFMACH_BENCH_RHYMES_DATASET
#error "RYFMACH_BENCH_RHYMES_DATASET must point to rhymes.jsonl"
#endif

#ifndef RYFMACH_BENCH_WORDS_DATASET
#error "RYFMACH_BENCH_WORDS_DATASET must point to words.jsonl"
#endif

namespace {

using Json = nlohmann::json;

struct WordEntry {
    std::string word;
    std::size_t accent;
};

struct RhymeQuery {
    std::vector<ryfmach::bel::Sound> query_transcription;
    std::vector<std::vector<ryfmach::bel::Sound>> rhyme_transcriptions;
};

WordEntry ParseWordEntry(const Json& value) {
    return {
        value.at("word").get<std::string>(),
        value.at("accent").get<std::size_t>(),
    };
}

std::ifstream OpenDataset(const char* path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error(std::string("failed to open dataset: ") + path);
    }
    return input;
}

std::vector<WordEntry> LoadWordsDataset(const char* path) {
    std::vector<WordEntry> words;
    auto input = OpenDataset(path);

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        words.push_back(ParseWordEntry(Json::parse(line)));
    }

    return words;
}

std::vector<RhymeQuery> LoadRhymeQueriesDataset(const char* path) {
    std::vector<RhymeQuery> queries;
    auto input = OpenDataset(path);

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto item = Json::parse(line);
        const auto query = ParseWordEntry(item.at("query_word"));
        const auto query_transcription =
            ryfmach::bel::GetTranscriptionSounds(query.word, query.accent);
        if (!query_transcription) {
            continue;
        }

        RhymeQuery rhyme_query;
        rhyme_query.query_transcription = *query_transcription;

        for (const auto& rhyme_item : item.at("rhymes")) {
            const auto rhyme = ParseWordEntry(rhyme_item);
            const auto rhyme_transcription =
                ryfmach::bel::GetTranscriptionSounds(rhyme.word, rhyme.accent);
            if (!rhyme_transcription) {
                continue;
            }

            rhyme_query.rhyme_transcriptions.push_back(*rhyme_transcription);
        }

        if (!rhyme_query.rhyme_transcriptions.empty()) {
            queries.push_back(std::move(rhyme_query));
        }
    }

    return queries;
}

const std::vector<WordEntry>& WordsDataset() {
    static const auto words = LoadWordsDataset(RYFMACH_BENCH_WORDS_DATASET);
    return words;
}

const std::vector<RhymeQuery>& RhymeQueriesDataset() {
    static const auto queries = LoadRhymeQueriesDataset(RYFMACH_BENCH_RHYMES_DATASET);
    return queries;
}

void BM_Transcriptions(benchmark::State& state) {
    const std::vector<WordEntry>* words = nullptr;
    try {
        words = &WordsDataset();
    } catch (const std::exception& error) {
        state.SkipWithError(error.what());
        return;
    }

    auto iter = words->begin();
    for (auto _ : state) {
        if (iter == words->end()) {
            iter = words->begin();
        }
        auto transcription =
            ryfmach::bel::GetTranscriptionSounds(iter->word, iter->accent);
        ++iter;
        benchmark::DoNotOptimize(transcription);
        benchmark::DoNotOptimize(iter);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_Transcriptions)->Unit(benchmark::kMicrosecond);

void BM_RhymeQuality(benchmark::State& state) {
    const std::vector<RhymeQuery>* queries = nullptr;
    try {
        queries = &RhymeQueriesDataset();
    } catch (const std::exception& error) {
        state.SkipWithError(error.what());
        return;
    }
    if (queries->empty()) {
        state.SkipWithError("no valid rhyme queries in dataset");
        return;
    }

    std::size_t query_index = 0;
    int64_t rhyme_pairs_processed = 0;
    for (auto _ : state) {
        const auto& query = (*queries)[query_index];
        query_index = (query_index + 1) % queries->size();

        for (const auto& rhyme_transcription : query.rhyme_transcriptions) {
            auto quality = ryfmach::bel::CalcRhymeQualityKey(
                query.query_transcription, rhyme_transcription, 5);
            benchmark::DoNotOptimize(quality);
        }
        rhyme_pairs_processed +=
            static_cast<int64_t>(query.rhyme_transcriptions.size());
        benchmark::DoNotOptimize(query_index);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["rhyme_pairs"] = benchmark::Counter(
        static_cast<double>(rhyme_pairs_processed), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_RhymeQuality)->Unit(benchmark::kMicrosecond);

void BM_FindWordsInSlounik(benchmark::State& state) {
    const std::vector<WordEntry>* words = nullptr;
    try {
        words = &WordsDataset();
    } catch (const std::exception& error) {
        state.SkipWithError(error.what());
        return;
    }
    if (words->empty()) {
        state.SkipWithError("Words dataset is empty");
    }

    ryfmach::bel::Slounik slounik;
    const ryfmach::bel::WordLookupOptions opt{
        .fix_similar_letters = true,
        .include_compound_tail = true,
        .max_similar_letter_replacements = 5
    };
    
    std::size_t word_index = 0;
    for (auto _ : state) {
        const auto& word = (*words)[word_index];
        word_index = (word_index + 1) % words->size();

        auto found = slounik.FindWords(word.word, opt);
        if (found.empty()) {
            state.SkipWithError("Word not found: " + word.word);
        }
    }
    
    state.SetItemsProcessed(static_cast<uint64_t>(state.iterations()));
}
BENCHMARK(BM_FindWordsInSlounik)->Unit(benchmark::kMicrosecond);

} // namespace
