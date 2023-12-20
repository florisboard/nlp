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
#include <set>
#include <mutex>
#include <shared_mutex>
#include <cmath>

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

#define MAX_SEARCH_RESULTS 8

namespace fl::nlp {

export enum class LatinFuzzySearchType {
    Proximity,
    ProximityWithoutSelf,
    ProximityOrPrefix,
};

/* Compute frequency and merged properties from word-level, n-gram level, and shortcut level.
 * Returns pair:
 * - merged_properties: absolute score = sum frequencies from all types, offensive/hidden = or(that of each n)
 * - frequency: average of (smoothed) frequencies of all types. TODO: frequencies from dictionaries should add together, not averaged
 ******************************************************************************/
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
    // TODO: a sum of numerators over a sum of denominators is better than this average.
    if (end_node_count > 0) {
        frequency /= end_node_count;
    }
    return std::make_pair(merged_properties, frequency);
}

struct RecursiveFuzzySearchParams {
    SuggestionRequestFlags flags_;
    LatinFuzzySearchType search_type_;
    // The dictionaries that we search. These dictionary carry the dict ID's. The first one is the user custom dictionary, and the rest are from language packs.
    std::vector<const LatinDictionary*> dicts_to_search_;
    // Root node of Trie (shared among all dictionaries)
    LatinTrieNode* shared_data_;
    const LookupWeights& weights_;
    const KeyProximityChecker& key_proximity_checker_;
    // The place to store search results, as trie nodes
    TransientSuggestionResults<LatinTrieNode>& results_;
    // Maximum items to search for
    std::size_t top_k;
};

struct RecursiveFuzzySearchDistanceCell {
    double cost_ = 0.0;
    bool is_equal_ = false;
    bool is_equal_ignoring_case_ = false;
};

double computeConfidence(bool isWordPrefix, float cost, float frequency, std::size_t word_size, size_t token_size) {
    // NOTE: now, this returns log confidence instead of a 0-1 value.
    // TODO: reevaluate the weighting and calculation
    double w1 = 1.0;
    double w2 = 0.1;  // this means 2^(1/w2) times the frequency can offset an edit distance of w1
    double similarity;
    // if (isWordPrefix) {
    //     similarity = 1.0 - (cost / std::max(static_cast<std::size_t>(1), word_size));
    // } else {
    //     similarity = 1.0 - (cost / std::max(token_size, word_size));
    // }
    // originally, confidence is weighted average of:
    // - similarity, 1 identical, 0 all different. larger weight.
    // - frequency, smaller weight. Computed in `mergeProperties` (which calls `LatinDictionary.calculateFrequency`) as:
    //     - average of this n-gram (or word or shortcut)'s frequency in all dictionaries (language pack + user). Each is:
    //         - P(n-gram) (among all n-grams at that level)
    // now, it's negative cost (as log prob) and log frequency, weighted. frequency is n-gram probability, so it's pretty small.
    // FIXME: What was prefix score before? It was legible.
    similarity = -cost;
    double confidence = (w1 * similarity + w2 * std::log2(frequency)) / (w1 + w2);
    return confidence;
}

class RecursiveFuzzySearchState {
  public:
    // The costs of each edit type in the edit distance
    const LookupWeights& weights_;
    // Tool for checking if two characters are close to each other on the keyboard layout.
    const KeyProximityChecker& key_proximity_checker_;
    // Whether searching for a word or an n-gram (if n-gram, also specifies the n value). The last word may be incomplete.
    EntryType entry_type_;
    // The query word (user input, possibly incomplete). First character is a dummy.
    fl::str::UniString cached_word_ = {""};
    // A span subset of the query word, without the dummy character.
    std::span<const fl::str::UniChar> cached_word_span_;
    // An alternate version of query (user input) with swapped capitalization, used for case-agnostic search.
    fl::str::UniString cached_word_opposite_case_ = {""};
    // The partial word that is represented by the path from the Trie root till the currently node being processed. First character is a dummy.
    fl::str::UniString cached_token_ = {""};
    // Edit distance; [i][j] is the distance from query[0:j] vs searching[0:i]. Used for dynamic programming.
    std::vector<std::vector<RecursiveFuzzySearchDistanceCell>> distances_;
    // Leaderboard of results.
    std::set<std::pair<double, const std::string>> best_nodes;
    std::size_t top_k;

    RecursiveFuzzySearchState(
        const RecursiveFuzzySearchParams& params, EntryType entry_type, const fl::str::UniString& word
    )
        : weights_(params.weights_), key_proximity_checker_(params.key_proximity_checker_), entry_type_(entry_type), top_k(params.top_k) {
        initCachedWord(word);
        initCachedWordOppositeCase(word);
        initCachedToken();
        initDistances();
        // TODO: has this pre-allocation really an effect on performance??
        ensureCapacityFor(31);
    }

  private:
    // Initialize search query
    void initCachedWord(const fl::str::UniString& word) noexcept {
        cached_word_.resize(word.size() + 1);
        for (std::size_t i = 0; i < word.size(); i++) {
            cached_word_[i + 1] = word[i];
        }
#ifdef ANDROID
        cached_word_span_ = fl::utils::make_span(cached_word_.begin() + 1, cached_word_.end());
#else
        cached_word_span_ = std::span(cached_word_.begin() + 1, cached_word_.end());
#endif
    }

    // Initialize search query (inverted capitalization)
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

    // Initialize the string representation of path from root
    void initCachedToken() noexcept {
        cached_token_.resize(1);
    }

    // Clear scratch space for dynamic programming
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

    // Cost for character insertion (start of string or middle)
    [[nodiscard]]
    inline double insertCostFor(std::size_t token_index) const noexcept {
        return (token_index == 1) ? weights_.cost_insert_start_of_str_ : weights_.cost_insert_;
    }

    // Cost for character deletion (start of string or middle)
    [[nodiscard]]
    inline double deleteCostFor(std::size_t token_index) const noexcept {
        return (token_index == 1) ? weights_.cost_delete_start_of_str_ : weights_.cost_delete_;
    }

    // Cost for character substitution (start of string or middle)
    [[nodiscard]]
    inline double substitutionCostFor(std::size_t token_index) const noexcept {
        return (token_index == 1) ? weights_.cost_substitute_start_of_str_ : weights_.cost_substitute_;
    }

  public:
    // Gets the span for the query word, without the dummy character.
    [[nodiscard]]
    inline const std::span<const fl::str::UniChar>& wordSpan() const noexcept {
        return cached_word_span_;
    }

    // Gets the string representation of the node currently being searched, as a span, without the first dummy character.
    [[nodiscard]]
    inline std::span<const fl::str::UniChar> tokenSpanAt(std::size_t token_index) const {
#ifdef ANDROID
        return fl::utils::make_span(cached_token_.begin() + 1, cached_token_.begin() + token_index + 1);
#else
        return std::span(cached_token_.begin() + 1, cached_token_.begin() + token_index + 1);
#endif
    }

    // edit distance from `cached_word_` (user input) to the span [0, token_index] TODO: verify
    [[nodiscard]]
    inline double editDistanceAt(std::size_t token_index) const {
        return distances_[token_index][cached_word_.size() - 1].cost_;
    }

    /* branch pruning:
     * - discard if cost larger than hard threshold
     * - discard if cost larger than top_k-th best so far
     **************************************************************************/
    [[nodiscard]]
    inline bool isDeadEndAt(std::size_t token_index) const noexcept {
        double cost;
        if (token_index < cached_word_.size() - 1) {
            cost = distances_[token_index][token_index].cost_;
        } else {
            cost = editDistanceAt(token_index);
        }
        if (cost >= weights_.max_cost_sum_)
            return true;
        // Making an approximation: for each n = 1..N, the scores that ranks >= params.top_k-th are the same as the one that ranks params.top_k)-th.
        // With this approximation, to be top top_k in the final result, a word needs to be top top_k in at least one n = 1..N.
        if (best_nodes.size() >= top_k) {
            double max_possible_confidence = computeConfidence(false, cost, 1.0, cached_word_.size() - 1, token_index);
            if (max_possible_confidence <= best_nodes.cbegin()->first)
                return true;
        }
        return false;
    }

    // Whether the query word is equal to the current traversing word in range [0:token_index].
    [[nodiscard]]
    inline bool isPrefixAt(std::size_t token_index) const {
        std::size_t l = cached_word_.size() - 1;
        return token_index > l &&
               (cached_word_.size() == 1 || distances_[l][l].is_equal_ignoring_case_);
    }

    // Set up state for visiting the next Trie node with `token_char` as its character.
    void setTokenCharAt(std::size_t token_index, const fl::str::UniChar& token_char) {
        // 0-th row pre-filled
        if (token_index == 0) return;

        // Put char in current search node's position
        ensureCapacityFor(token_index);
        cached_token_[token_index] = token_char;
        // From empty string to current searched path is just inserting everything.
        distances_[token_index][0].cost_ = token_index * insertCostFor(token_index);
        // How much cost to substitute last character to make them the same
        double substitution_cost;
        bool is_equal = false;
        bool is_equal_ignoring_case = false;

        // iterate over the length of query word (from the 2nd character to end)
        // compute the distance of query string[0:i] to current searched path
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

            // current distance (edit from query[0:i] to searched[0:token_index]
            // is previous distance + cost of the one action to get here from there
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

/* Given current search node, search config, current search state and depth, traverse the node.
 * Input:
 * - sentence: a span of strings, each string being one of the last few words of the sentence
 * - params: config of the search, and the place to store the search results
 ******************************************************************************/
template<typename P>
void fuzzySearchRecursive(
    LatinTrieNode* node,
    const RecursiveFuzzySearchParams& params,
    RecursiveFuzzySearchState& state,
    std::size_t token_index
) noexcept {
    // The last input word (potentially partial)
    auto& word = state.wordSpan();
    // Current cost
    auto candidateCost = state.editDistanceAt(token_index);
    // Eligible as word candidate? current traversing word length > 0, and candidate cost within bound
    auto isWordCandidate = token_index > 0 && candidateCost <= state.weights_.max_cost_sum_;
    auto prefixCost = 0.0; // Is initialized in next line only if isWordPrefix results in true
    // TODO: improve prefix searching performance (run time and stop detection)
    // Eligible as prefix candidate? searching for prefix, current word and query word same until `token_index`, and cost at word size within bound.
    auto isWordPrefix = params.search_type_ == LatinFuzzySearchType::ProximityOrPrefix &&
                        state.isPrefixAt(token_index) &&
                        (prefixCost = state.editDistanceAt(word.size())) <= state.weights_.max_cost_sum_;
    auto cost = isWordPrefix ? prefixCost : candidateCost;

    if (isWordCandidate || isWordPrefix) {
        // get word's properties from n-grams (n = 1..N) and averaged smoothed frequency.
        auto [merged_properties, frequency] = mergeProperties<P>(node, state.entry_type_, params.dicts_to_search_);
        // if current word is never a leaf, frequency will be 0, and skipped.
        if (frequency > 0.0) {
            // currently traversing word
            auto token = state.tokenSpanAt(token_index);
            auto is_same_but_should_not =
                params.search_type_ == LatinFuzzySearchType::ProximityWithoutSelf && fl::utils::equal(token, word);
            // conditions for offensive and hidden word
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
                // Compute score, and add to results.
                double confidence = computeConfidence(isWordPrefix, cost, frequency, word.size(), token.size());

                std::string raw_word;
                fl::str::toStdString(token, raw_word);

                // maintain top-params.top_k items
                if (state.best_nodes.size() >= params.top_k && confidence > state.best_nodes.cbegin()->first) {
                    state.best_nodes.erase(state.best_nodes.cbegin());
                }
                state.best_nodes.insert(std::make_pair(confidence, raw_word));
            }
        }
    }

    if (!isWordPrefix && state.isDeadEndAt(token_index)) {
        return;
    }

    // Prepare iterating from cheapest: first, build sorted list
    std::set<std::pair<double, LatinTrieNode*>> sorted_children;
    for (auto& child_node : node->children_) {
        if (isSpecialToken(child_node->key_)) {
            continue;
        }
        state.setTokenCharAt(token_index + 1, child_node->key_);
        sorted_children.insert(std::make_pair(state.editDistanceAt(token_index + 1), child_node.get()));
    }
    // Then, iterate from cheapest.
    for (auto [distance, child_node] : sorted_children) {
        state.setTokenCharAt(token_index + 1, child_node->key_);
        fuzzySearchRecursive<P>(child_node, params, state, token_index + 1);
    }
}

/* Given part of a sentence (as a by-word std::span), predict word, and put into &params.
 * Input:
 * - sentence: a span of strings, each string being one of the last few words of the sentence
 * - params: config of the search, and the place to store the search results
 ******************************************************************************/
void predictWordInternal(std::span<const fl::str::UniString> sentence, const RecursiveFuzzySearchParams& params) {
    // Current maximum N for n-gram
    auto max_ngram_level = std::max(1, std::min(params.flags_.maxNgramLevel(), static_cast<int>(sentence.size())));

    // Compute n-gram for each 1 <= n <= N
    for (int ngram_level = 1; ngram_level <= max_ngram_level; ngram_level++) {

        // The last `ngram_level` words
        auto ngram = sentence.subspan(sentence.size() - ngram_level, ngram_level);
        // If we have a uni-gram: only search for proximate words
        if (ngram_level == 1) {
            // Get last word (possibly incomplete)
            auto current_word = sentence.back();
            // No word: this is prediction. Rely on n-gram (n > 1)
            if (current_word.empty()) continue;
            // To save time, for short words, do not do prefix matching
            RecursiveFuzzySearchParams new_params(params);
            if (current_word.size() < 3 && params.search_type_ == LatinFuzzySearchType::ProximityOrPrefix) {
                new_params.search_type_ = LatinFuzzySearchType::Proximity;
            }
            // Word fuzzy matching, searching `current_word` (last user input word, possibly incomplete)
            RecursiveFuzzySearchState state = {new_params, EntryType::word(), current_word};
            // Start search, from the node `params.shared_data_`, given search config, initial state, and start search from first character.
            fuzzySearchRecursive<WordEntryProperties>(new_params.shared_data_, new_params, state, 0);
            // TODO: for n-grams, each n is one entry in the `results_` vector. We should do a weighted average for each unique word for smoothing.
            for (auto& [confidence, raw_word] : state.best_nodes)
                params.results_.insert({raw_word, std::exp2(confidence)}, params.flags_);
            // Shortcut exact matching
            for (auto* dict : params.dicts_to_search_) {
                // add to search results only if finds exact match in the trie, and value is not empty, and is a shortcut with properties.
                auto* shortcut_node = params.shared_data_->findOrNull(current_word);
                if (shortcut_node == nullptr) continue;
                auto* value = shortcut_node->valueOrNull(dict->dict_id_);
                if (value == nullptr) continue;
                auto* properties = value->shortcutPropertiesOrNull();
                if (properties == nullptr) continue;
                params.results_.insert({properties->shortcut_phrase, std::exp2(0.0)}, params.flags_);
            }
        } else {
            // We have an n-gram (n > 1)
            // Only search the user's dictionary for n-grams for now. TODO: support n-gram in language dictionaries as well.
            auto* dict = params.dicts_to_search_[0];
            // Subset of the n-gram, words other than the last one
            auto subngram = ngram.subspan(0, ngram.size() - 1);
            // Find all nodes matching the sub-n-gram in the first dictionary (they share the same Trie, with each node labeled with its dictionary ID) TODO: verify
            auto subngram_nodes = algorithms::findNgramIgnoringCase(&(dict->data_->node), dict->dict_id_, subngram);
            for (auto* subngram_node : subngram_nodes) {
                // Find the children whose character is ' ', getting the nodes for searching the last user input word
                auto* word_node = subngram_node->findOrNull(LATIN_TOKEN_NGRAM_SEPARATOR);
                if (word_node == nullptr) continue;
                // Search current word (possibly partial) on that node, add to results
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

        std::shared_lock<std::shared_mutex> lock(session_state_.shared_data->lock);
        RecursiveFuzzySearchParams params = {
            flags,
            search_type,
            dicts_to_search,
            &(session_state_.shared_data->node),
            session_config_.weights_.lookup_,
            session_config_.key_proximity_checker_,
            results,
            MAX_SEARCH_RESULTS + 1,
        };
        predictWordInternal(sentence, params);
    }
};

} // namespace fl::nlp
