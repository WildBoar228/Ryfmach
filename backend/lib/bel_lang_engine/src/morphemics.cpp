#include "morphemics.hpp"

#include "language.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace ryfmach::bel {
namespace {

constexpr int kVerbPartOfSpeech = 2;
constexpr int kAdverbPartOfSpeech = 6;
constexpr int kFunctionWordPartOfSpeech = 7;

struct AnalysisContext {
    std::vector<MorphemicPrefixRecord> prefixes;
    std::unordered_map<std::string, std::vector<MorphemicAnalysis>> cache;
    std::unordered_map<std::string, bool> in_progress;
};

bool IsMarkedMorpheme(std::string_view text, char opening, char closing) {
    return text.size() >= 2 && text.front() == opening && text.back() == closing;
}

std::string JoinLetters(std::span<const Letter> letters) {
    std::string word;
    for (const Letter letter : letters) {
        word += Spelling(letter);
    }
    return word;
}

bool IsVariableBasisLetter(Letter letter) noexcept {
    return letter == Letter::kShortU || letter == Letter::kSoftSign ||
           letter == Letter::kShortI || IsApostrophe(letter);
}

std::string ExtractWordBasis(const Slounik& slounik, const WordRecord& word) {
    const auto letters = ParseWord(word.word);
    if (!letters) {
        return word.word;
    }
    std::size_t max_length = letters->size();

    const auto forms = slounik.GetWordForms(word.initial_id);

    std::vector<std::vector<Letter>> parsed_forms;
    parsed_forms.reserve(forms.size());
    for (const auto& form : forms) {
        auto form_letters = ParseWord(form.word);
        if (form_letters) {
            max_length = std::min(max_length, form_letters->size());
            parsed_forms.push_back(std::move(*form_letters));
        }
    }

    for (std::size_t index = 0; index < letters->size(); ++index) {
        if (IsVariableBasisLetter((*letters)[index])) {
            continue;
        }
        for (const auto& form_letters : parsed_forms) {
            if ((*letters)[index] != form_letters[index] &&
                !IsVariableBasisLetter(form_letters[index])) {
                return JoinLetters(std::span(*letters).first(index));
            }
        }
    }

    return word.word;
}

void AppendAnalysis(
    std::vector<MorphemicAnalysis>& destination,
    std::vector<MorphemicAnalysis> source
) {
    destination.insert(
        destination.end(),
        std::make_move_iterator(source.begin()),
        std::make_move_iterator(source.end()));
}

std::vector<MorphemicAnalysis> AnalyzeWord(
    const Slounik& slounik,
    std::string_view word,
    bool fix_similar_letters,
    AnalysisContext& context);

std::vector<MorphemicAnalysis> AnalyzeSurePredictions(
    const Slounik& slounik,
    const WordRecord& word,
    AnalysisContext& context
) {
    std::vector<MorphemicAnalysis> result;

    if (word.part_of_speech_id == kVerbPartOfSpeech) {
        if (word.is_initial && word.word.ends_with("цца")) {
            const std::string non_reflexive =
                word.word.substr(0, word.word.size() - std::string_view("ца").size()) +
                "ь";
            for (auto analysis : AnalyzeWord(slounik, non_reflexive, false, context)) {
                if (!analysis.analysis.empty() &&
                    analysis.analysis.back().text == "ць") {
                    analysis.analysis.back().text = "ц";
                    analysis.analysis.push_back(
                        Morpheme{.type = MorphemeType::kSuffix, .text = "ца"});
                    result.push_back(std::move(analysis));
                }
            }
        } else if (word.word.ends_with("ся")) {
            const std::string non_reflexive =
                word.word.substr(0, word.word.size() - std::string_view("ся").size());
            for (auto analysis : AnalyzeWord(slounik, non_reflexive, false, context)) {
                analysis.analysis.push_back(
                    Morpheme{.type = MorphemeType::kSuffix, .text = "ся"});
                result.push_back(std::move(analysis));
            }
        } else {
            for (const std::string_view suffix : {"ючы", "учы", "ўшы"}) {
                if (!word.word.ends_with(suffix)) {
                    continue;
                }

                const std::string non_reflexive =
                    word.word.substr(0, word.word.size() - suffix.size()) + "ць";
                for (auto analysis :
                     AnalyzeWord(slounik, non_reflexive, false, context)) {
                    if (!analysis.analysis.empty() &&
                        analysis.analysis.back().text == "ць") {
                        analysis.analysis.back().text = std::string(suffix);
                        result.push_back(std::move(analysis));
                    }
                }
                break;
            }
        }
    }

    if (!result.empty()) {
        return result;
    }

    if (!word.is_initial) {
        if (const auto initial = slounik.GetWordById(word.initial_id)) {
            return AnalyzeWord(slounik, initial->word, false, context);
        }
    }

    return {};
}

std::vector<MorphemicAnalysis> AnalyzeFallback(
    const Slounik& slounik,
    const WordRecord& word
) {
    if (word.part_of_speech_id == kVerbPartOfSpeech && word.is_initial &&
        word.word.ends_with("ць")) {
        auto analysis = DecodeMorphemicAnalysis(
            word.word.substr(0, word.word.size() - std::string_view("ць").size()));
        analysis.push_back(Morpheme{.type = MorphemeType::kSuffix, .text = "ць"});
        return {{.analysis = std::move(analysis), .sure = false}};
    }

    if (word.part_of_speech_id == kAdverbPartOfSpeech ||
        word.part_of_speech_id == kFunctionWordPartOfSpeech) {
        return {{
            .analysis = {{.type = MorphemeType::kUnknown, .text = word.word}},
            .sure = false,
        }};
    }

    std::string basis = ExtractWordBasis(slounik, word);
    std::string ending = word.word.substr(basis.size());
    auto analysis = DecodeMorphemicAnalysis(basis);
    if (word.part_of_speech_id == kVerbPartOfSpeech && ending.starts_with("л")) {
        analysis.push_back(Morpheme{.type = MorphemeType::kSuffix, .text = "л"});
        ending.erase(0, std::string_view("л").size());
    }
    analysis.push_back(Morpheme{.type = MorphemeType::kEnding, .text = ending});
    return {{.analysis = std::move(analysis), .sure = false}};
}

std::vector<MorphemicAnalysis> AnalyzeWord(
    const Slounik& slounik,
    std::string_view word,
    bool fix_similar_letters,
    AnalysisContext& context
) {
    const std::string key = std::string(fix_similar_letters ? "1:" : "0:") +
                            std::string(word);
    if (const auto cached = context.cache.find(key); cached != context.cache.end()) {
        return cached->second;
    }
    if (context.in_progress.contains(key)) {
        return {};
    }
    context.in_progress.emplace(key, true);

    std::vector<MorphemicAnalysis> result;
    for (const auto& stored :
         slounik.FindMorphemicAnalyses(word, fix_similar_letters)) {
        result.push_back(MorphemicAnalysis{
            .analysis = DecodeMorphemicAnalysis(stored),
            .sure = true,
        });
    }

    if (result.empty()) {
        const auto word_variants = slounik.FindWords(
            word,
            WordLookupOptions{.fix_similar_letters = fix_similar_letters});
        for (const auto& variant : word_variants) {
            AppendAnalysis(result, AnalyzeSurePredictions(slounik, variant, context));
        }
    }

    if (result.empty()) {
        for (const auto& prefix : context.prefixes) {
            if (!word.starts_with(prefix.text) || word.size() == prefix.text.size()) {
                continue;
            }

            std::string cut_word(word.substr(prefix.text.size()));
            auto prefix_analysis = DecodeMorphemicAnalysis(prefix.analysis);
            if (cut_word.starts_with("'") && !prefix_analysis.empty()) {
                prefix_analysis.back().text += "'";
                cut_word.erase(0, 1);
            }

            for (auto analysis :
                 AnalyzeWord(slounik, cut_word, fix_similar_letters, context)) {
                analysis.analysis.insert(
                    analysis.analysis.begin(),
                    prefix_analysis.begin(),
                    prefix_analysis.end());
                result.push_back(std::move(analysis));
            }
            if (!result.empty()) {
                break;
            }
        }
    }

    if (result.empty()) {
        const auto word_variants = slounik.FindWords(
            word,
            WordLookupOptions{.fix_similar_letters = fix_similar_letters});
        for (const auto& variant : word_variants) {
            AppendAnalysis(result, AnalyzeFallback(slounik, variant));
        }
    }

    context.in_progress.erase(key);
    context.cache.emplace(key, result);
    return result;
}

} // namespace

std::vector<Morpheme> DecodeMorphemicAnalysis(std::string_view encoded) {
    std::vector<Morpheme> result;
    for (std::size_t start = 0; start <= encoded.size();) {
        const std::size_t end = encoded.find('-', start);
        const std::string_view part = encoded.substr(
            start,
            end == std::string_view::npos ? std::string_view::npos : end - start);
        if (!part.empty()) {
            MorphemeType type = MorphemeType::kUnknown;
            std::string_view text = part;
            if (IsMarkedMorpheme(part, '|', '|')) {
                type = MorphemeType::kPrefix;
                text = part.substr(1, part.size() - 2);
            } else if (IsMarkedMorpheme(part, '(', ')')) {
                type = MorphemeType::kRoot;
                text = part.substr(1, part.size() - 2);
            } else if (IsMarkedMorpheme(part, '<', '>')) {
                type = MorphemeType::kSuffix;
                text = part.substr(1, part.size() - 2);
            } else if (IsMarkedMorpheme(part, '[', ']')) {
                type = MorphemeType::kEnding;
                text = part.substr(1, part.size() - 2);
            }
            result.push_back(Morpheme{.type = type, .text = std::string(text)});
        }

        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return result;
}

std::string EncodeMorphemicAnalysis(std::span<const Morpheme> analysis) {
    std::string encoded;
    for (std::size_t index = 0; index < analysis.size(); ++index) {
        if (index != 0) {
            encoded += '-';
        }

        const Morpheme& morpheme = analysis[index];
        switch (morpheme.type) {
            case MorphemeType::kPrefix:
                encoded += '|';
                encoded += morpheme.text;
                encoded += '|';
                break;
            case MorphemeType::kRoot:
                encoded += '(';
                encoded += morpheme.text;
                encoded += ')';
                break;
            case MorphemeType::kSuffix:
                encoded += '<';
                encoded += morpheme.text;
                encoded += '>';
                break;
            case MorphemeType::kEnding:
                encoded += '[';
                encoded += morpheme.text;
                encoded += ']';
                break;
            case MorphemeType::kUnknown:
                encoded += morpheme.text;
                break;
        }
    }
    return encoded;
}

MorphemicAnalyzer::MorphemicAnalyzer(const Slounik& slounik) : slounik_(slounik) {}

std::vector<MorphemicAnalysis> MorphemicAnalyzer::Analyze(
    std::string_view word,
    bool fix_similar_letters
) const {
    if (!ParseWord(word)) {
        return {};
    }

    AnalysisContext context{
        .prefixes = slounik_.GetMorphemicPrefixes(),
    };
    return AnalyzeWord(slounik_, word, fix_similar_letters, context);
}

} // namespace ryfmach::bel
