/*
 * Copyright 2023 Patrick Goldinger
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

module;

#include <nlohmann/json.hpp>
#include <unicode/locid.h>
#include <unicode/utext.h>

#include <filesystem>
#include <functional>
#include <span>
#include <utility>
#include <vector>

export module fl.nlp.core.latin:nlp_session;

import :algorithms;
import :dictionary;
import :entry_properties;
import :nlp_session_config;
import :nlp_session_state;
import :prediction;
import :prediction_weights;
import fl.icuext;
import fl.nlp.core.common;
import fl.string;

namespace fl::nlp {

export class LatinNlpSession {
  private:
    fl::icuext::BreakIteratorCache iterators;

  public:
    LatinNlpSessionConfig config;
    LatinNlpSessionState state;

  public:
    void loadConfigFromFile(const std::filesystem::path& config_path) {
        std::ifstream config_file(config_path);
        if (!config_file.is_open()) {
            throw std::runtime_error("Cannot open config file at '" + config_path.string() + "'!");
        }
        auto json_config = nlohmann::json::parse(config_file);
        config_file.close();
        json_config.get_to(config);
        loadDictionary(state.USER_DICTIONARY_ID, config.user_dictionary_path);
        auto id = state.USER_DICTIONARY_ID + 1;
        for (auto& path : config.base_dictionary_paths) {
            loadDictionary(id++, path);
        }
        UErrorCode status = U_ZERO_ERROR;
        iterators.init(config.primary_locale, status);
    }

    void loadDictionary(LatinDictId id, const std::filesystem::path& dict_path) {
        auto dict = std::make_unique<LatinDictionary>(id, state.shared_data);
        dict->loadFromDisk(dict_path);
        if (id >= state.dictionaries.size()) {
            state.dictionaries.resize(id + 1);
        }
        state.dictionaries[id] = std::move(dict);
    }

    SpellingResult spell(
        const std::string& raw_word, const std::vector<std::string>& prev_raw_words, const SuggestionRequestFlags& flags
    ) noexcept {
        if (raw_word.empty()) {
            return SpellingResult::unspecified();
        }

        fl::str::UniString word;
        fl::str::toUniString(raw_word, word);
        auto word_node = state.shared_data->findOrNull(word);
        if (word_node != nullptr && word_node->isEndNode()) {
            return SpellingResult::validWord();
        }

        std::vector<fl::str::UniString> sentence;
        auto max_prev_words = static_cast<int>(flags.maxNgramLevel()) - 1;
        for (int insert_pos = 0; insert_pos < max_prev_words; insert_pos++) {
            int extract_pos = prev_raw_words.size() - max_prev_words + insert_pos;
            if (extract_pos < 0) {
                sentence.push_back({LATIN_TOKEN_START_OF_SENTENCE});
            } else {
                fl::str::toUniString(prev_raw_words[extract_pos], word);
                sentence.push_back(std::move(word));
            }
        }
        fl::str::toUniString(raw_word, word);
        sentence.push_back(word);

        /*LatinFuzzySearcher fuzzy_searcher(&config, &state);
        TransientSuggestionResults<LatinTrieNode> transient_results;
        SuggestionResults results;
        fuzzy_searcher.predictWord(sentence, flags, FuzzySearchType::ProximityWithoutSelf, transient_results);
        algorithms::writeSuggestionResults(transient_results, results);*/
        LatinPredictionWrapper prediction_wrapper(config, state);
        TransientSuggestionResults<LatinTrieNode> transient_results;
        SuggestionResults results;
        prediction_wrapper.predictWord(sentence, flags, LatinFuzzySearchType::ProximityWithoutSelf, transient_results);
        algorithms::writeSuggestionResults(transient_results, results);

        std::vector<std::string> suggested_corrections;
        for (auto& candidate : results) {
            suggested_corrections.push_back(candidate->text);
        }

        return SpellingResult::typo(suggested_corrections);
    }

    void suggest(
        const std::string& raw_word,
        const std::vector<std::string>& prev_raw_words,
        const SuggestionRequestFlags& flags,
        SuggestionResults& results
    ) noexcept {
        results.clear();

        fl::str::UniString word;
        std::vector<fl::str::UniString> sentence;
        auto max_prev_words = static_cast<int>(flags.maxNgramLevel()) - 1;
        for (int insert_pos = 0; insert_pos < max_prev_words; insert_pos++) {
            int extract_pos = prev_raw_words.size() - max_prev_words + insert_pos;
            if (extract_pos < 0) {
                sentence.push_back({LATIN_TOKEN_START_OF_SENTENCE});
            } else {
                fl::str::toUniString(prev_raw_words[extract_pos], word);
                sentence.push_back(std::move(word));
            }
        }
        fl::str::toUniString(raw_word, word);
        sentence.push_back(word);

        /*LatinFuzzySearcher fuzzy_searcher(&config, &state);
        TransientSuggestionResults<LatinTrieNode> transient_results;
        fuzzy_searcher.predictWord(sentence, flags, FuzzySearchType::ProximityOrPrefix, transient_results);
        algorithms::writeSuggestionResults(transient_results, results);*/
        LatinPredictionWrapper prediction_wrapper(config, state);
        TransientSuggestionResults<LatinTrieNode> transient_results;
        prediction_wrapper.predictWord(sentence, flags, LatinFuzzySearchType::ProximityOrPrefix, transient_results);
        algorithms::writeSuggestionResults(transient_results, results);
    }

  public:
    void train(
        fl::nlp::LatinDictionary* target_dictionary, const std::vector<std::string>& sentence, int32_t max_prev_words
    ) noexcept {
        if (sentence.empty()) return;

        const auto id = target_dictionary->dict_id_;
        std::vector<fl::str::UniString> uni_sentence;
        fl::str::UniString uni_word;

        // Read and insert words
        for (auto& word : sentence) {
            fl::str::toUniString(word, uni_word);
            auto word_node = target_dictionary->data_->findOrCreate(uni_word);
            auto properties = word_node->valueOrCreate(id)->wordPropertiesOrCreate();
            properties->absolute_score += config.weights_.training_.usage_bonus_;
            properties->absolute_score += config.weights_.training_.usage_reduction_others_;
            target_dictionary->global_penalties_[EntryType::word()] +=
                config.weights_.training_.usage_reduction_others_;
            uni_sentence.push_back(std::move(uni_word));
        }

        // Insert "start of sentence" tokens
        for (int i = 0; i < max_prev_words - 1; i++) {
            uni_sentence.insert(uni_sentence.begin(), {LATIN_TOKEN_START_OF_SENTENCE});
        }

        // Read and insert ngrams
        for (int ngram_level = 2; ngram_level <= max_prev_words; ngram_level++) {
            for (int i = max_prev_words - ngram_level; i < uni_sentence.size() - ngram_level + 1; i++) {
                auto ngram = std::span(uni_sentence.begin() + i, ngram_level);
                auto ngram_node = target_dictionary->insertNgram(ngram);
                auto properties = ngram_node->valueOrCreate(id)->ngramPropertiesOrCreate();
                properties->absolute_score += config.weights_.training_.usage_bonus_;
                properties->absolute_score += config.weights_.training_.usage_reduction_others_;
                target_dictionary->global_penalties_[EntryType::ngram(ngram_level)] +=
                    config.weights_.training_.usage_reduction_others_;
            }
        }
    }
};

} // namespace fl::nlp
