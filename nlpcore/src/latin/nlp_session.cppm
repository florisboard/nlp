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

import :dictionary;
import :fuzzy_searcher;
import :nlp_session_config;
import :nlp_session_state;
import :prediction_weights;
import fl.icuext;
import fl.nlp.core.common;
import fl.nlp.string;

namespace fl::nlp {

export const fl::str::UniString FLDIC_UNI_TOKEN_START_OF_SENTENCE = {std::string(1, FLDIC_TOKEN_START_OF_SENTENCE)};

using WordTrieNode = TrieNode<fl::str::UniChar, WordProperties>;
using NgramTrieNode = TrieNode<fl::str::UniChar, NgramProperties>;
using WordTrieMap = TrieMap<fl::str::UniChar, WordProperties>;
using NgramTrieMap = TrieMap<fl::str::UniChar, NgramProperties>;

export class LatinNlpSession {
  public:
    LatinNlpSessionConfig config;
    LatinNlpSessionState state;

  private:
    fl::icuext::BreakIteratorCache iterators;

  public:
    void loadConfigFromFile(const std::filesystem::path& config_path) {
        std::ifstream config_file(config_path);
        if (!config_file.is_open()) {
            throw std::runtime_error("Cannot open config file at '" + config_path.string() + "'!");
        }
        auto json_config = nlohmann::json::parse(config_file);
        config_file.close();
        json_config.get_to(config);
        for (auto& path : config.base_dictionary_paths) {
            loadBaseDictionary(path);
        }
        loadUserDictionary(config.user_dictionary_path);
        UErrorCode status = U_ZERO_ERROR;
        iterators.init(config.primary_locale, status);
    }

    void loadBaseDictionary(const std::filesystem::path& dict_path) {
        auto base_dict = std::make_unique<LatinDictionary>();
        base_dict->loadFromDisk(dict_path);
        state.base_dictionaries.push_back(std::move(base_dict));
    }

    void loadUserDictionary(const std::filesystem::path& dict_path) {
        auto user_dict = std::make_unique<LatinDictionary>();
        user_dict->loadFromDisk(dict_path);
        state.user_dictionary = std::move(user_dict);
    }

    SpellingResult spell(
        const std::string& word,
        const std::vector<std::string>& prev_words,
        const std::vector<std::string>& next_words,
        const SuggestionRequestFlags& flags
    ) const noexcept {
        if (word.empty()) {
            return SpellingResult::unspecified();
        }
        fl::str::UniString uni_word;
        fl::str::toUniString(word, uni_word);
        auto word_node = state.base_dictionaries[0]->words.getOrNull(uni_word);
        if (word_node != nullptr && word_node->is_end_node) {
            return SpellingResult::validWord();
        }

        /*SuggestionResults results;
        fuzzySearch<WordProperties>(
            base_dictionaries[0]->words.rootNode(), FuzzySearchType::ProximityWithoutSelf,
            config.weights.words.lookup.max_cost, flags, uni_word,
            [&](fl::str::UniString& uni_suggested_word, const WordTrieNode* node, int cost) {
                std::string suggested_word;
                fl::str::toStdString(uni_suggested_word, suggested_word);
                double confidence = 1.0 - ((cost) / 6.0);
                //    (static_cast<double>(node->properties.absolute_score) /
                //    _base_dictionaries[0]->max_unigram_score);

                results.insert({std::move(suggested_word), "", confidence}, flags);
            }
        );

        std::vector<std::string> suggested_corrections;
        for (auto& candidate : results.get()) {
            suggested_corrections.push_back(candidate->text);
        }

        return SpellingResult::typo(suggested_corrections);*/
        return SpellingResult::unspecified();
    }

    void suggest(
        const std::string& word,
        const std::vector<std::string>& prev_words,
        const SuggestionRequestFlags& flags,
        SuggestionResults& results
    ) const noexcept {
        results.clear();

        fl::str::UniString tmp_word;
        std::vector<fl::str::UniString> sentence;
        auto max_prev_words = static_cast<int>(flags.maxNgramLevel()) - 1;
        for (int insert_pos = 0; insert_pos < max_prev_words; insert_pos++) {
            int extract_pos = prev_words.size() - max_prev_words + insert_pos;
            if (extract_pos < 0) {
                sentence.push_back(FLDIC_UNI_TOKEN_START_OF_SENTENCE);
            } else {
                fl::str::toUniString(prev_words[extract_pos], tmp_word);
                sentence.push_back(std::move(tmp_word));
            }
        }
        fl::str::toUniString(word, tmp_word);
        sentence.push_back(tmp_word);

        LatinFuzzySearcher fuzzy_searcher(&config, &state);
        return fuzzy_searcher.predictWord(sentence, flags, results);
    }

  public:
    void train(const std::vector<std::string>& sentence, int32_t max_prev_words) noexcept {
        if (sentence.empty()) return;

        std::vector<fl::str::UniString> uni_sentence;
        fl::str::UniString uni_word;

        // Read and insert words
        for (auto& word : sentence) {
            fl::str::toUniString(word, uni_word);
            auto word_node = state.user_dictionary->words.getOrCreate(uni_word);
            word_node->value.absolute_score += config.weights.words.training.usage_bonus;
            word_node->value.absolute_score += config.weights.words.training.usage_reduction_others;
            state.user_dictionary->global_penalty_words += config.weights.words.training.usage_reduction_others;
            uni_sentence.push_back(std::move(uni_word));
        }

        // Insert "start of sentence" tokens
        for (int i = 0; i < max_prev_words - 1; i++) {
            uni_sentence.insert(uni_sentence.begin(), FLDIC_UNI_TOKEN_START_OF_SENTENCE);
        }

        // Read and insert ngrams
        for (int ngram_level = 2; ngram_level <= max_prev_words; ngram_level++) {
            for (int i = max_prev_words - ngram_level; i < uni_sentence.size() - ngram_level + 1; i++) {
                auto ngram = std::span(uni_sentence.begin() + i, ngram_level);
                auto ngram_node = state.user_dictionary->insertNgram(ngram);
                ngram_node->value.absolute_score += config.weights.ngrams.training.usage_bonus;
                ngram_node->value.absolute_score += config.weights.ngrams.training.usage_reduction_others;
                state.user_dictionary->global_penalty_ngrams[ngram_level] +=
                    config.weights.ngrams.training.usage_reduction_others;
            }
        }
    }

  private:
    bool baseDictsContainWord(fl::str::UniString& uni_word) const noexcept {
        for (auto& base_dict : state.base_dictionaries) {
            auto word_node = base_dict->words.getOrNull(uni_word);
            if (word_node != nullptr && word_node->is_end_node) {
                return true;
            }
        }
        return false;
    }
};

} // namespace fl::nlp
