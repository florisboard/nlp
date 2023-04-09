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
    const LookupWeights* weights_ = nullptr;
    EntryType entry_type_ = EntryType::word();
    FuzzySearchType search_type_ = FuzzySearchType::Proximity;
    std::vector<LatinDictId> dict_ids_to_search_;

    void init(
        std::span<const fl::str::UniChar> uni_word,
        EntryType entry_type,
        FuzzySearchType search_type,
        const LookupWeights* weights,
        const std::vector<LatinDictId>& dict_ids_to_search
    ) noexcept {
        entry_type_ = entry_type;
        search_type_ = search_type;
        weights_ = weights;
        dict_ids_to_search_ = dict_ids_to_search;
        word.resize(uni_word.size() + 1);
        for (std::size_t i = 0; i < uni_word.size(); i++) {
            word[i + 1] = uni_word[i];
        }
        initOppositeWord();
        prefix.resize(1);
        distances.clear();
        distances.push_back(std::vector(word.size(), 0));
        for (std::size_t i = 1; i < word.size(); i++) {
            distances[0][i] = i * weights_->cost_insert;
        }
    }

    void setPrefixCharAt(std::size_t prefix_index, const fl::str::UniChar& prefix_char) {
        if (prefix_index == 0) {
            return;
        }

        ensureCapacityFor(prefix_index);
        prefix[prefix_index] = prefix_char;
        distances[prefix_index][0] = prefix_index * weights_->cost_insert;

        int penalty;
        int substitution_cost;
        int transpose_cost;

        for (std::size_t i = 1; i < word.size(); i++) {
            // Calculate penalty
            penalty = (i == 1 && prefix_index == 1) ? weights_->penalty_start_of_str : weights_->penalty_default;

            // Calculate SUBSTITUTION / IS EQUAL
            if (prefix_char == word[i]) {
                substitution_cost = weights_->cost_is_equal;
            } else if (prefix_char == word_opposite_case[i]) {
                substitution_cost = weights_->cost_is_opposite_case;
            } else if (prefix_index > 1 && i > 1 && prefix[prefix_index - 1] == word[i] && prefix_char == word[i - 1]) {
                // TODO: investigate if transpose calculation could be incorrect for certain edge cases
                substitution_cost = weights_->cost_transpose - 1 + penalty;
            } else {
                substitution_cost = weights_->cost_substitute + penalty;
            }

            distances[prefix_index][i] = std::min(
                std::min(
                    distances[prefix_index - 1][i] + weights_->cost_insert,
                    distances[prefix_index][i - 1] + weights_->cost_delete
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
            return distances[prefix_index][prefix_index] >= weights_->max_cost;
        } else {
            return editDistanceAt(prefix_index) >= weights_->max_cost;
        }
    }

  private:
    void initOppositeWord() noexcept {
        word_opposite_case.resize(word.size());
        std::size_t i = 0;
        for (auto& word_char : word) {
            auto upper_char = word_char;
            fl::str::uppercase(upper_char);
            auto lower_char = word_char;
            fl::str::uppercase(lower_char);
            word_opposite_case[i] = word_char != upper_char ? upper_char : lower_char;
            i++;
        }
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

template<typename P>
struct ResultInfo {
    double frequency_;
    int edit_distance_;
    P* properties_;
};

export class LatinFuzzySearcher {
  private:
    template<typename P>
    using OnResultLambda = const std::function<void(std::span<const fl::str::UniChar>, ResultInfo<P>&)>;

  private:
    const LatinNlpSessionConfig* session_config_;
    LatinNlpSessionState* session_state_;

  public:
    LatinFuzzySearcher() = delete;
    explicit LatinFuzzySearcher(const LatinNlpSessionConfig* session_config, LatinNlpSessionState* session_state)
        : session_config_(session_config), session_state_(session_state) {}

    void predictWord(
        std::span<fl::str::UniString> sentence, const SuggestionRequestFlags& flags, SuggestionResults& results
    ) noexcept {
        if (sentence.empty()) {
            return;
        }

        // TODO: this is hard-coded
        static std::vector<LatinDictId> dict_ids_to_search = {0, 1};

        auto max_ngram_level = std::max(1, std::min(flags.maxNgramLevel(), static_cast<int>(sentence.size())));
        for (int ngram_level = 1; ngram_level <= max_ngram_level; ngram_level++) {
            if (ngram_level == 1) {
                // We have a uni-gram and only search for proximate words
                auto current_word = sentence.back();
                if (current_word.empty()) continue;
                std::string raw_word;
                fuzzySearchWord(
                    current_word, FuzzySearchType::ProximityOrPrefix, dict_ids_to_search,
                    [&](auto word, auto& result_info) {
                        fl::str::toStdString(word, raw_word);
                        // TODO: this is a mess and needs fixing
                        // double confidence =
                        //     result_info.frequency_ * (0.01 + 0.99 * (1.0 - (result_info.edit_distance_ / 6.0)));
                        double confidence = (6.0 - result_info.edit_distance_) + result_info.frequency_;
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
        const fl::str::UniString& word,
        FuzzySearchType search_type,
        const std::vector<LatinDictId>& dict_ids_to_search,
        OnResultLambda<WordEntryProperties>& on_result
    ) noexcept {
        RecursiveDldCache state;
        state.init(word, EntryType::word(), search_type, &session_config_->weights.words.lookup, dict_ids_to_search);
        fuzzySearchWordRecursiveDld<WordEntryProperties>(session_state_->shared_data.get(), state, 0, on_result);
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
        if (cost <= state.weights_->max_cost && !prefix.empty()) {
            bool is_end_node = false;
            P merged_properties;
            merged_properties.absolute_score = 1;
            ResultInfo<P> result_info {0.0, cost, &merged_properties};
            for (auto& id : state.dict_ids_to_search_) {
                auto* dict = session_state_->getDictionaryById(id);
                auto* value = node->valueOrNull(id);
                if (value == nullptr) continue;
                is_end_node = true;
                if constexpr (std::is_same_v<P, WordEntryProperties>) {
                    auto* p = value->wordPropertiesOrNull();
                    if (p == nullptr) continue;
                    merged_properties.absolute_score += p->absolute_score;
                    merged_properties.is_possibly_offensive =
                        merged_properties.is_possibly_offensive || p->is_possibly_offensive;
                    merged_properties.is_hidden_by_user = merged_properties.is_hidden_by_user || p->is_hidden_by_user;
                    result_info.frequency_ = dict->calculateFrequency(state.entry_type_, p->absolute_score, 1);
                } else if constexpr (std::is_same_v<P, NgramEntryProperties>) {
                    auto* p = value->ngramPropertiesOrNull();
                    if (p == nullptr) continue;
                    merged_properties.absolute_score += p->absolute_score;
                    result_info.frequency_ = dict->calculateFrequency(state.entry_type_, p->absolute_score, 1);
                } else if constexpr (std::is_same_v<P, ShortcutEntryProperties>) {
                    auto* p = value->shortcutPropertiesOrNull();
                    if (p == nullptr) continue;
                    merged_properties.absolute_score += p->absolute_score;
                    result_info.frequency_ = dict->calculateFrequency(state.entry_type_, p->absolute_score, 1);
                }
            }
            if (is_end_node) {
                on_result(prefix, result_info);
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
