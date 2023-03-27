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

#include <algorithm>
#include <functional>
#include <iterator>
#include <set>
#include <span>
#include <string>

export module fl.nlp.core.latin:fuzzy_searcher;

import :dictionary;
import :entry_properties;
import :nlp_session_config;
import :nlp_session_state;
import fl.nlp.core.common;
import fl.nlp.string;

namespace fl::nlp {

enum class FuzzySearchType {
    Proximity,
    ProximityWithoutSelf,
    ProximityOrPrefix,
};

class RecursiveDldCache {
  private:
    std::vector<std::vector<int>> distances;
    fl::str::UniString word = {""};
    fl::str::UniString word_opposite_case = {""};
    fl::str::UniString prefix = {""};

  public:
    const LookupWeights* weights = nullptr;
    FuzzySearchType type = FuzzySearchType::Proximity;
    std::vector<std::size_t> dict_ids_to_search;

    void init(
        std::span<const fl::str::UniChar> uni_word,
        FuzzySearchType type_,
        const LookupWeights* weights_,
        const std::vector<std::size_t>& dict_ids_to_search_
    ) noexcept {
        type = type_;
        weights = weights_;
        dict_ids_to_search = dict_ids_to_search_;
        word.resize(uni_word.size() + 1);
        for (std::size_t i = 0; i < uni_word.size(); i++) {
            word[i + 1] = uni_word[i];
        }
        initOppositeWord();
        prefix.resize(1);
        distances.clear();
        distances.push_back(std::vector(word.size(), 0));
        for (std::size_t i = 1; i < word.size(); i++) {
            distances[0][i] = i * weights->cost_insert;
        }
    }

    void setPrefixCharAt(std::size_t prefix_index, const fl::str::UniChar& prefix_char) {
        if (prefix_index == 0) {
            return;
        }

        ensureCapacityFor(prefix_index);
        prefix[prefix_index] = prefix_char;
        distances[prefix_index][0] = prefix_index * weights->cost_insert;

        int penalty;
        int substitution_cost;
        int transpose_cost;

        for (std::size_t i = 1; i < word.size(); i++) {
            // Calculate penalty
            penalty = (i == 1 && prefix_index == 1) ? weights->penalty_start_of_str : weights->penalty_default;

            // Calculate SUBSTITUTION / IS EQUAL
            if (prefix_char == word[i]) {
                substitution_cost = weights->cost_is_equal;
            } else if (prefix_char == word_opposite_case[i]) {
                substitution_cost = weights->cost_is_opposite_case;
            } else if (prefix_index > 1 && i > 1 && prefix[prefix_index - 1] == word[i] && prefix_char == word[i - 1]) {
                // TODO: investigate if transpose calculation could be incorrect for certain edge cases
                substitution_cost = weights->cost_transpose - 1 + penalty;
            } else {
                substitution_cost = weights->cost_substitute + penalty;
            }

            distances[prefix_index][i] = std::min(
                std::min(
                    distances[prefix_index - 1][i] + weights->cost_insert,
                    distances[prefix_index][i - 1] + weights->cost_delete
                ),
                distances[prefix_index - 1][i - 1] + substitution_cost
            );
        }
    }

    inline std::span<const fl::str::UniChar> prefixSpanAt(std::size_t prefix_index) const {
        return {prefix.begin() + 1, prefix.begin() + prefix_index + 1};
    }

    inline int editDistanceAt(std::size_t prefix_index) const {
        return distances[prefix_index][word.size() - 1];
    }

    bool isDeadEndAt(std::size_t prefix_index) const noexcept {
        if (prefix_index < word.size() - 1) {
            return distances[prefix_index][prefix_index] >= weights->max_cost;
        } else {
            return editDistanceAt(prefix_index) >= weights->max_cost;
        }
    }

  private:
    void initOppositeWord() noexcept {
        // TODO
        word_opposite_case = word;
    }

    void ensureCapacityFor(std::size_t prefix_index) noexcept {
        while (prefix.size() <= prefix_index) {
            prefix.emplace_back("");
        }
        while (distances.size() <= prefix_index) {
            distances.emplace_back(word.size(), 0);
        }
    }
};

export class LatinFuzzySearcher {
  private:
    template<typename P>
    using OnResultLambda = const std::function<void(std::span<const fl::str::UniChar>, int, P&)>;

  private:
    const LatinNlpSessionConfig* session_config;
    LatinNlpSessionState* session_state;

  public:
    LatinFuzzySearcher() = delete;
    explicit LatinFuzzySearcher(const LatinNlpSessionConfig* session_config_, LatinNlpSessionState* session_state_)
        : session_config(session_config_), session_state(session_state_) {}

    void predictWord(
        std::span<fl::str::UniString> sentence, const SuggestionRequestFlags& flags, SuggestionResults& results
    ) noexcept {
        if (sentence.empty()) {
            return;
        }
        auto max_ngram_level = std::max(1, std::min(flags.maxNgramLevel(), static_cast<int>(sentence.size())));
        for (int ngram_level = 1; ngram_level <= max_ngram_level; ngram_level++) {
            if (ngram_level == 1) {
                // We have a uni-gram and only search for proximate words
                auto current_word = sentence.back();
                if (current_word.empty()) continue;
                std::string raw_word;
                fuzzySearchWord(
                    current_word, FuzzySearchType::ProximityOrPrefix,
                    [&](auto word, int cost, WordEntryProperties& properties) {
                        fl::str::toStdString(word, raw_word);
                        double confidence = 1.0 - (cost / 6.0);
                        results.insert({std::move(raw_word), "", confidence}, flags);
                    }
                );
            } else {
                // We have an n-gram
                auto ngram = sentence.subspan(sentence.size() - ngram_level, ngram_level);
            }
        }
    }

  private:
    void fuzzySearchWord(
        const fl::str::UniString& word, FuzzySearchType type, OnResultLambda<WordEntryProperties>& on_result
    ) noexcept {
        RecursiveDldCache state;
        state.init(word, type, &session_config->weights.words.lookup, {0, 1});
        fuzzySearchWordRecursiveDld<WordEntryProperties>(session_state->shared_data.get(), state, 0, on_result);
    }

    void fuzzySearchNgram(
        std::span<const fl::str::UniString> ngram, FuzzySearchType type, OnResultLambda<NgramEntryProperties>& on_result
    ) noexcept {
        if (ngram.size() < 2) {
            return;
        }
        /*RecursiveDldCache state;
        auto current_ngram_root_node = session_state->shared_data.get();
        for (const auto& token : ngram) {
            if (LatinDictionary::isSpecialToken(token) || true) {
                auto node = current_ngram_root_node->findOrNull(token);
                if (node != nullptr) {
                    auto it = node->values.find(LatinNlpSessionState::USER_DICTIONARY_ID);
                    if (it != node->values.end() && it->second.ngramPropertiesOrNull() != nullptr) {
                        current_ngram_root_node = ;
                    }
                } else {
                    break;
                }
            } else {
                // state.init(token, type, &session_config->weights.ngrams.lookup);
                // fuzzySearchWordRecursiveDld(current_map->rootNode(), state, 0, on_result);
            }
        }*/
    }

    template<typename P>
    void fuzzySearchWordRecursiveDld(
        TrieNode<fl::str::UniChar, EntryProperties>* node,
        RecursiveDldCache& state,
        std::size_t prefix_index,
        OnResultLambda<P>& on_result
    ) noexcept {
        auto cost = state.editDistanceAt(prefix_index);
        auto prefix = state.prefixSpanAt(prefix_index);
        if (cost <= state.weights->max_cost && !prefix.empty()) {
            bool is_end_node = false;
            P merged_properties;
            merged_properties.absolute_score = 1;
            for (auto& id : state.dict_ids_to_search) {
                auto value = node->valueOrNull(id);
                if (value == nullptr) continue;
                is_end_node = true;
                if constexpr (std::is_same_v<P, WordEntryProperties>) {
                    auto p = value->wordPropertiesOrNull();
                    if (p == nullptr) continue;
                    merged_properties.absolute_score += p->absolute_score;
                    merged_properties.is_possibly_offensive =
                        merged_properties.is_possibly_offensive || p->is_possibly_offensive;
                    merged_properties.is_hidden_by_user = merged_properties.is_hidden_by_user || p->is_hidden_by_user;
                } else if constexpr (std::is_same_v<P, NgramEntryProperties>) {
                    auto p = value->ngramPropertiesOrNull();
                    if (p == nullptr) continue;
                    merged_properties.absolute_score += p->absolute_score;
                } else if constexpr (std::is_same_v<P, ShortcutEntryProperties>) {
                    auto p = value->shortcutPropertiesOrNull();
                    if (p == nullptr) continue;
                    merged_properties.absolute_score += p->absolute_score;
                }
            }
            if (is_end_node) {
                on_result(prefix, cost, merged_properties);
            }
        }

        // Exit unnecessary recursive loop
        if (state.isDeadEndAt(prefix_index)) {
            return;
        }

        for (auto& [child_char, child_node] : node->children) {
            if (LatinDictionary::isSpecialToken(child_char)) {
                continue;
            }
            state.setPrefixCharAt(prefix_index + 1, child_char);
            fuzzySearchWordRecursiveDld(child_node.get(), state, prefix_index + 1, on_result);
        }
    }
};

} // namespace fl::nlp
