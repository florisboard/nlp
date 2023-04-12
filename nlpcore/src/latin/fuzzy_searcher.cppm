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

#include <fmt/ranges.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <set>
#include <span>
#include <string>

export module fl.nlp.core.latin:fuzzy_searcher;

import :algorithms;
import :dictionary;
import :entry_properties;
import :nlp_session_config;
import :nlp_session_state;
import fl.nlp.core.common;
import fl.string;
import fl.utils;

namespace fl::nlp {

export enum class FuzzySearchType {
    Proximity,
    ProximityWithoutSelf,
    ProximityOrPrefix,
};

class RecursiveDldCache {
  public:
    std::vector<std::vector<int>> distances;
    fl::str::UniString word = {""};
    fl::str::UniString word_opposite_case = {""};
    fl::str::UniString token_ = {""};

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
        token_.resize(1);
        distances.clear();
        distances.push_back(std::vector(word.size(), 0));
        for (std::size_t i = 1; i < word.size(); i++) {
            distances[0][i] = i * weights_->cost_insert;
        }
    }

    void setTokenCharAt(std::size_t token_index, const fl::str::UniChar& token_char) {
        if (token_index == 0) {
            return;
        }

        ensureCapacityFor(token_index);
        token_[token_index] = token_char;
        distances[token_index][0] = token_index * weights_->cost_insert;

        int penalty;
        int substitution_cost;

        for (std::size_t i = 1; i < word.size(); i++) {
            // Calculate penalty
            penalty = (i == 1 && token_index == 1) ? weights_->penalty_start_of_str : weights_->penalty_default;

            // Calculate SUBSTITUTION / IS EQUAL
            if (token_char == word[i]) {
                substitution_cost = weights_->cost_is_equal;
            } else if (token_char == word_opposite_case[i]) {
                substitution_cost = weights_->cost_is_opposite_case;
            } else if (token_index > 1 && i > 1 && token_[token_index - 1] == word[i] && token_char == word[i - 1]) {
                // TODO: investigate if transpose calculation could be incorrect for certain edge cases
                substitution_cost = weights_->cost_transpose + penalty;
            } else {
                substitution_cost = weights_->cost_substitute + penalty;
            }

            distances[token_index][i] = std::min(
                std::min(
                    distances[token_index - 1][i] + weights_->cost_insert,
                    distances[token_index][i - 1] + weights_->cost_delete
                ),
                distances[token_index - 1][i - 1] + substitution_cost
            );
        }
    }

    [[nodiscard]]
    inline std::span<const fl::str::UniChar> tokenSpanAt(std::size_t token_index) const {
        return {token_.begin() + 1, token_.begin() + token_index + 1};
    }

    [[nodiscard]]
    inline int editDistanceAt(std::size_t token_index) const {
        return distances[token_index][word.size() - 1];
    }

    [[nodiscard]]
    bool isDeadEndAt(std::size_t token_index) const noexcept {
        if (token_index < word.size() - 1) {
            return distances[token_index][token_index] >= weights_->max_cost;
        } else {
            return editDistanceAt(token_index) >= weights_->max_cost;
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
            fl::str::lowercase(lower_char);
            word_opposite_case[i] = word_char != upper_char ? upper_char : lower_char;
            i++;
        }
    }

    void ensureCapacityFor(std::size_t token_index) noexcept {
        while (token_.size() <= token_index) {
            token_.emplace_back("");
        }
        while (distances.size() <= token_index) {
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
        std::span<fl::str::UniString> sentence,
        const SuggestionRequestFlags& flags,
        FuzzySearchType search_type,
        SuggestionResults& results
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
                fuzzySearchWord(current_word, search_type, dict_ids_to_search, [&](auto word, auto& result_info) {
                    fl::str::toStdString(word, raw_word);
                    // TODO: this is a mess and needs fixing
                    // double confidence =
                    //     result_info.frequency_ * (0.01 + 0.99 * (1.0 - (result_info.edit_distance_ / 6.0)));
                    double confidence = (6.0 - result_info.edit_distance_) + result_info.frequency_;
                    results.insert({std::move(raw_word), "", confidence}, flags);
                });
            } else {
                // We have an n-gram
                auto ngram = sentence.subspan(sentence.size() - ngram_level, ngram_level);
                std::string raw_word;
                fuzzySearchNgram(ngram, search_type, dict_ids_to_search, [&](auto word, auto& result_info) {
                    fl::str::toStdString(word, raw_word);
                    // TODO: this is a mess and needs fixing
                    // double confidence =
                    //     result_info.frequency_ * (0.01 + 0.99 * (1.0 - (result_info.edit_distance_ / 6.0)));
                    double confidence = (6.0 - result_info.edit_distance_) + result_info.frequency_;
                    results.insert({std::move(raw_word), "", confidence * 2}, flags);
                });
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
        std::span<const fl::str::UniString> ngram,
        FuzzySearchType search_type,
        const std::vector<LatinDictId>& dict_ids_to_search,
        OnResultLambda<NgramEntryProperties>& on_result
    ) noexcept {
        if (ngram.size() < 2) {
            return;
        }

        auto subgram = ngram.subspan(0, ngram.size() - 1);
        auto* subgram_node = algorithms::findNgramOrNull(session_state_->shared_data.get(), ANY_DICT, subgram);
        if (subgram_node == nullptr) return;
        auto* subgram_search_node = subgram_node->findOrNull(LATIN_TOKEN_NGRAM_SEPARATOR);
        if (subgram_search_node == nullptr) return;

        RecursiveDldCache state;
        state.init(
            ngram.back(), EntryType::ngram(ngram.size()), search_type, &session_config_->weights.ngrams.lookup,
            dict_ids_to_search
        );
        fuzzySearchWordRecursiveDld<NgramEntryProperties>(subgram_search_node, state, 0, on_result);
    }

    template<typename P>
    void fuzzySearchWordRecursiveDld(
        LatinTrieNode* node, RecursiveDldCache& state, std::size_t token_index, OnResultLambda<P>& on_result
    ) noexcept {
        auto token = state.tokenSpanAt(token_index);
        auto candidateCost = state.editDistanceAt(token_index);
        auto isWordCandidate = !token.empty() && candidateCost <= state.weights_->max_cost;
        auto prefixCost = 0; // Is initialized in next line only if isWordPrefix results in true
        auto isWordPrefix = state.search_type_ == FuzzySearchType::ProximityOrPrefix &&
                            token.size() >= (state.word.size() - 1) &&
                            (prefixCost = state.editDistanceAt(state.word.size() - 1)) <= state.weights_->max_cost;
        auto cost = isWordPrefix ? prefixCost : candidateCost;

        if (isWordCandidate || isWordPrefix) {
            bool is_end_node = false;
            P merged_properties;
            merged_properties.absolute_score = 0;
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
                if (state.search_type_ == FuzzySearchType::ProximityWithoutSelf &&
                    fl::utils::equal(token, std::span(state.word))) {
                    // Do nothing
                } else {
                    on_result(token, result_info);
                }
            }
        }

        // Exit unnecessary recursive loop
        if (!isWordPrefix && state.isDeadEndAt(token_index)) {
            return;
        }

        for (auto& [child_char, child_node] : node->children) {
            if (isSpecialToken(child_char)) {
                continue;
            }
            state.setTokenCharAt(token_index + 1, child_char);
            fuzzySearchWordRecursiveDld(child_node.get(), state, token_index + 1, on_result);
        }
    }
};

} // namespace fl::nlp
