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
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

export module fl.nlp.core.latin:prediction;

import :algorithms;
import :dictionary;
import :entry_properties;
import :nlp_session_config;
import :nlp_session_state;
import :prediction_weights;
import fl.nlp.core.common;
import fl.string;
import fl.utils;

namespace fl::nlp {

export enum class LatinFuzzySearchType {
    Proximity,
    ProximityWithoutSelf,
    ProximityOrPrefix,
};

template<typename P>
std::pair<P, double> mergeProperties(
    LatinTrieNode* node, EntryType entry_type, const std::vector<const LatinDictionary*>& dicts_to_search
) noexcept {
    std::size_t end_node_count = 0;
    double frequency = 0.0;
    P merged_properties;
    for (const auto* dict : dicts_to_search) {
        auto* value = node->valueOrNull(dict->dict_id_);
        if (value == nullptr) continue;
        end_node_count++;
        if constexpr (std::is_same_v<P, WordEntryProperties>) {
            auto* p = value->wordPropertiesOrNull();
            if (p == nullptr) continue;
            merged_properties.absolute_score += p->absolute_score;
            merged_properties.is_possibly_offensive =
                merged_properties.is_possibly_offensive || p->is_possibly_offensive;
            merged_properties.is_hidden_by_user = merged_properties.is_hidden_by_user || p->is_hidden_by_user;
            frequency += dict->calculateFrequency(entry_type, p->absolute_score, 1);
        } else if constexpr (std::is_same_v<P, NgramEntryProperties>) {
            auto* p = value->ngramPropertiesOrNull();
            if (p == nullptr) continue;
            merged_properties.absolute_score += p->absolute_score;
            frequency += dict->calculateFrequency(entry_type, p->absolute_score, 1);
        } else if constexpr (std::is_same_v<P, ShortcutEntryProperties>) {
            auto* p = value->shortcutPropertiesOrNull();
            if (p == nullptr) continue;
            merged_properties.absolute_score += p->absolute_score;
            frequency += dict->calculateFrequency(entry_type, p->absolute_score, 1);
        }
    }
    if (end_node_count > 0) {
        frequency /= end_node_count;
    }
    return std::make_pair(merged_properties, frequency);
}

struct RecursiveFuzzySearchParams {
    SuggestionRequestFlags flags_;
    LatinFuzzySearchType search_type_;
    std::vector<const LatinDictionary*> dicts_to_search_;
    LatinTrieNode* shared_data_;
    const LookupWeights& weights_;
    const KeyProximityChecker& key_proximity_checker_;
    TransientSuggestionResults<LatinTrieNode>& results_;
};

struct RecursiveFuzzySearchDistanceCell {
    double cost_ = 0.0;
    bool is_equal_ = false;
    bool is_equal_ignoring_case_ = false;
};

class RecursiveFuzzySearchState {
  public:
    const LookupWeights& weights_;
    const KeyProximityChecker& key_proximity_checker_;
    EntryType entry_type_;
    fl::str::UniString cached_word_ = {""};
    std::span<const fl::str::UniChar> cached_word_span_;
    fl::str::UniString cached_word_opposite_case_ = {""};
    fl::str::UniString cached_token_ = {""};
    std::vector<std::vector<RecursiveFuzzySearchDistanceCell>> distances_;

    RecursiveFuzzySearchState(
        const RecursiveFuzzySearchParams& params, EntryType entry_type, const fl::str::UniString& word
    )
        : weights_(params.weights_), key_proximity_checker_(params.key_proximity_checker_), entry_type_(entry_type) {
        initCachedWord(word);
        initCachedWordOppositeCase(word);
        initCachedToken();
        initDistances();
        // TODO: has this pre-allocation really an effect on performance??
        ensureCapacityFor(31);
    }

  private:
    void initCachedWord(const fl::str::UniString& word) noexcept {
        cached_word_.resize(word.size() + 1);
        for (std::size_t i = 0; i < word.size(); i++) {
            cached_word_[i + 1] = word[i];
        }
        cached_word_span_ = std::span(cached_word_.begin() + 1, cached_word_.end());
    }

    void initCachedWordOppositeCase(const fl::str::UniString& word) noexcept {
        cached_word_opposite_case_.resize(word.size() + 1);
        for (std::size_t i = 0; i < word.size(); i++) {
            const auto& word_char = word[i];
            auto upper_char = word_char;
            fl::str::uppercase(upper_char);
            auto lower_char = word_char;
            fl::str::lowercase(lower_char);
            cached_word_opposite_case_[i + 1] = word_char != upper_char ? upper_char : lower_char;
        }
    }

    void initCachedToken() noexcept {
        cached_token_.resize(1);
    }

    void initDistances() noexcept {
        distances_.clear();
        distances_.push_back(std::vector(cached_word_.size(), RecursiveFuzzySearchDistanceCell()));
        for (std::size_t i = 0; i < cached_word_.size(); i++) {
            distances_[0][i].cost_ = i * insertCostFor(i);
            distances_[0][i].is_equal_ = (i == 0);
            distances_[0][i].is_equal_ignoring_case_ = (i == 0);
        }
    }

    inline void ensureCapacityFor(std::size_t token_index) {
        std::size_t desired_size = token_index + 1;
        if (cached_token_.size() < desired_size) {
            cached_token_.resize(desired_size);
            distances_.resize(desired_size, std::vector(cached_word_.size(), RecursiveFuzzySearchDistanceCell()));
        }
    }

    [[nodiscard]]
    inline double insertCostFor(std::size_t token_index) const noexcept {
        return (token_index == 1) ? weights_.cost_insert_start_of_str_ : weights_.cost_insert_;
    }

    [[nodiscard]]
    inline double deleteCostFor(std::size_t token_index) const noexcept {
        return (token_index == 1) ? weights_.cost_delete_start_of_str_ : weights_.cost_delete_;
    }

    [[nodiscard]]
    inline double substitutionCostFor(std::size_t token_index) const noexcept {
        return (token_index == 1) ? weights_.cost_substitute_start_of_str_ : weights_.cost_substitute_;
    }

  public:
    [[nodiscard]]
    inline const std::span<const fl::str::UniChar>& wordSpan() const noexcept {
        return cached_word_span_;
    }

    [[nodiscard]]
    inline std::span<const fl::str::UniChar> tokenSpanAt(std::size_t token_index) const {
        return std::span(cached_token_.begin() + 1, cached_token_.begin() + token_index + 1);
    }

    [[nodiscard]]
    inline double editDistanceAt(std::size_t token_index) const {
        return distances_[token_index][cached_word_.size() - 1].cost_;
    }

    [[nodiscard]]
    inline bool isDeadEndAt(std::size_t token_index) const noexcept {
        if (token_index < cached_word_.size() - 1) {
            return distances_[token_index][token_index].cost_ >= weights_.max_cost_sum_;
        } else {
            return editDistanceAt(token_index) >= weights_.max_cost_sum_;
        }
    }

    [[nodiscard]]
    inline bool isPrefixAt(std::size_t token_index) const {
        return token_index > (cached_word_.size() - 1) && (cached_word_.size() == 1 || distances_[1][1].is_equal_ignoring_case_);
    }

    void setTokenCharAt(std::size_t token_index, const fl::str::UniChar& token_char) {
        if (token_index == 0) return;

        ensureCapacityFor(token_index);
        cached_token_[token_index] = token_char;
        distances_[token_index][0].cost_ = token_index * insertCostFor(token_index);

        double substitution_cost;
        bool is_equal = false;
        bool is_equal_ignoring_case = false;

        for (std::size_t i = 1; i < cached_word_.size(); i++) {
            if (token_char == cached_word_[i]) {
                // EQUAL
                substitution_cost = weights_.cost_is_equal_;
                is_equal = true;
                is_equal_ignoring_case = true;
            } else if (token_char == cached_word_opposite_case_[i]) {
                // EQUAL (ignoring case)
                substitution_cost = weights_.cost_is_equal_ignoring_case_;
                is_equal_ignoring_case = true;
            } else if (token_index > 1 && i > 1 && cached_token_[token_index - 1] == cached_word_[i] && token_char == cached_word_[i - 1]) {
                // TRANSPOSE
                substitution_cost = weights_.cost_transpose_;
            } else if (key_proximity_checker_.isInProximity(token_char, cached_word_[i])) {
                // SUBSTITUTE (in proximity)
                substitution_cost = weights_.cost_substitute_in_proximity_;
            } else {
                // SUBSTITUTE
                substitution_cost = substitutionCostFor(token_index);
            }

            distances_[token_index][i].cost_ = std::min(
                std::min(
                    distances_[token_index - 1][i].cost_ + insertCostFor(token_index),
                    distances_[token_index][i - 1].cost_ + deleteCostFor(token_index)
                ),
                distances_[token_index - 1][i - 1].cost_ + substitution_cost
            );
            distances_[token_index][i].is_equal_ = distances_[token_index - 1][i - 1].is_equal_ && is_equal;
            distances_[token_index][i].is_equal_ignoring_case_ =
                distances_[token_index - 1][i - 1].is_equal_ignoring_case_ && is_equal_ignoring_case;
        }
    }
};

template<typename P>
void fuzzySearchRecursive(
    LatinTrieNode* node,
    const RecursiveFuzzySearchParams& params,
    RecursiveFuzzySearchState& state,
    std::size_t token_index
) noexcept {
    auto& word = state.wordSpan();
    auto candidateCost = state.editDistanceAt(token_index);
    auto isWordCandidate = token_index > 0 && candidateCost <= state.weights_.max_cost_sum_;
    auto prefixCost = 0.0; // Is initialized in next line only if isWordPrefix results in true
    // TODO: improve prefix searching performance (run time and stop detection)
    auto isWordPrefix = params.search_type_ == LatinFuzzySearchType::ProximityOrPrefix &&
                        state.isPrefixAt(token_index) &&
                        (prefixCost = state.editDistanceAt(word.size())) <= state.weights_.max_cost_sum_;
    auto cost = isWordPrefix ? prefixCost : candidateCost;

    if (isWordCandidate || isWordPrefix) {
        auto [merged_properties, frequency] = mergeProperties<P>(node, state.entry_type_, params.dicts_to_search_);
        if (frequency > 0.0) {
            auto token = state.tokenSpanAt(token_index);
            auto is_same_but_should_not =
                params.search_type_ == LatinFuzzySearchType::ProximityWithoutSelf && fl::utils::equal(token, word);
            bool is_offensive;
            bool is_hidden;
            if constexpr (std::is_same_v<P, WordEntryProperties> || std::is_same_v<P, ShortcutEntryProperties>) {
                is_offensive = !params.flags_.allowPossiblyOffensive() && merged_properties.is_possibly_offensive;
                is_hidden = !params.flags_.overrideHiddenFlag() && merged_properties.is_hidden_by_user;
            } else {
                is_offensive = false;
                is_hidden = false;
            }
            if (is_same_but_should_not || is_offensive || is_hidden) {
                // Do nothing
            } else {
                // TODO: reevaluate the weighting and calculation
                double w1 = 1.0;
                double w2 = 0.1;
                double similarity;
                if (isWordPrefix) {
                    similarity = 1.0 - (cost / std::max(1uL, word.size()));
                } else {
                    similarity = 1.0 - (cost / std::max(token.size(), word.size()));
                }
                double confidence = (w1 * similarity + w2 * frequency) / (w1 + w2);
                std::string raw_word;
                fl::str::toStdString(token, raw_word);
                params.results_.insert({raw_word, confidence}, params.flags_);
            }
        }
    }

    if (!isWordPrefix && state.isDeadEndAt(token_index)) {
        return;
    }

    for (auto& child_node : node->children_) {
        if (isSpecialToken(child_node->key_)) {
            continue;
        }
        state.setTokenCharAt(token_index + 1, child_node->key_);
        fuzzySearchRecursive<P>(child_node.get(), params, state, token_index + 1);
    }
}

void predictWordInternal(std::span<const fl::str::UniString> sentence, const RecursiveFuzzySearchParams& params) {
    auto max_ngram_level = std::max(1, std::min(params.flags_.maxNgramLevel(), static_cast<int>(sentence.size())));
    for (int ngram_level = 1; ngram_level <= max_ngram_level; ngram_level++) {
        auto ngram = sentence.subspan(sentence.size() - ngram_level, ngram_level);
        if (ngram_level == 1) {
            // We have a uni-gram and only search for proximate words
            auto current_word = sentence.back();
            if (current_word.empty()) continue;
            // Word fuzzy matching
            RecursiveFuzzySearchState state = {params, EntryType::word(), current_word};
            fuzzySearchRecursive<WordEntryProperties>(params.shared_data_, params, state, 0);
            // Shortcut exact matching
            for (auto* dict : params.dicts_to_search_) {
                auto* shortcut_node = params.shared_data_->findOrNull(current_word);
                if (shortcut_node == nullptr) continue;
                auto* value = shortcut_node->valueOrNull(dict->dict_id_);
                if (value == nullptr) continue;
                auto* properties = value->shortcutPropertiesOrNull();
                if (properties == nullptr) continue;
                params.results_.insert({properties->shortcut_phrase, 1.0}, params.flags_);
            }
        } else {
            // We have an n-gram
            auto* dict = params.dicts_to_search_[0];
            auto subngram = ngram.subspan(0, ngram.size() - 1);
            auto subngram_nodes = algorithms::findNgramIgnoringCase(dict->data_.get(), dict->dict_id_, subngram);
            for (auto* subngram_node : subngram_nodes) {
                auto* word_node = subngram_node->findOrNull(LATIN_TOKEN_NGRAM_SEPARATOR);
                if (word_node == nullptr) continue;
                auto current_word = ngram.back();
                RecursiveFuzzySearchState state = {params, EntryType::ngram(ngram_level), current_word};
                fuzzySearchRecursive<NgramEntryProperties>(word_node, params, state, 0);
            }
        }
    }
}

export class LatinPredictionWrapper {
  private:
    const LatinNlpSessionConfig& session_config_;
    const LatinNlpSessionState& session_state_;

  public:
    LatinPredictionWrapper() = delete;
    LatinPredictionWrapper(const LatinNlpSessionConfig& session_config, const LatinNlpSessionState& session_state)
        : session_config_(session_config), session_state_(session_state) {}

    void predictWord(
        std::span<const fl::str::UniString> sentence,
        const SuggestionRequestFlags& flags,
        LatinFuzzySearchType search_type,
        TransientSuggestionResults<LatinTrieNode>& results
    ) {
        if (sentence.empty()) {
            return;
        }

        // TODO: this is hard-coded
        std::vector<const LatinDictionary*> dicts_to_search = {
            session_state_.getDictionaryById(0), session_state_.getDictionaryById(1)};

        RecursiveFuzzySearchParams params = {
            flags,
            search_type,
            dicts_to_search,
            session_state_.shared_data.get(),
            session_config_.weights_.lookup_,
            session_config_.key_proximity_checker_,
            results};
        predictWordInternal(sentence, params);
    }
};

} // namespace fl::nlp
