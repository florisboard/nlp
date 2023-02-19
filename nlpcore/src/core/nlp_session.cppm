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

#include <unicode/locid.h>
#include <unicode/ubrk.h>
#include <unicode/utext.h>

#include <filesystem>
#include <functional>
#include <utility>
#include <vector>

export module fl.nlp.core.nlp_session;

import fl.nlp.core.common;
import fl.nlp.core.dictionary;
import fl.nlp.core.key_proximity_map;
import fl.nlp.string;
import fl.nlp.core.trie_map;

namespace fl::nlp {

static const int MAX_COST = 6;
static const int COST_IS_EQUAL = 0;
static const int COST_IS_OPPOSITE_CASE = 1;
static const int COST_INSERT = 2;
static const int COST_DELETE = 2;
static const int COST_SUBSTITUTE_DEFAULT = 2;
static const int COST_SUBSTITUTE_IN_PROXIMITY = 1;
static const int COST_TRANSPOSE = 1;
static const int PENALTY_DEFAULT = 0;
static const int PENALTY_START_OF_STR = 2;

export struct PredictionWeights {
    //
};

enum class FuzzySearchType {
    Proximity,
    ProximityWithoutSelf,
    ProximityOrPrefix,
};

using WordTrieNode = TrieNode<fl::str::UniChar, NgramProperties>;

export class NlpSession {
  public:
    icu::Locale primary_locale;
    std::vector<icu::Locale> secondary_locales;
    std::vector<std::unique_ptr<fl::nlp::LatinDictionary>> dictionaries;
    KeyProximityMap key_proximity_map;
    PredictionWeights prediction_weights;

    void loadBaseDictionary(const std::filesystem::path& dict_path) {
        auto base_dict = std::make_unique<LatinDictionary>();
        base_dict->loadFromDisk(dict_path);
        dictionaries.push_back(std::move(base_dict));
    }

    SpellingResult spell(const std::string& word, const std::vector<std::string>& prev_words,
                         const std::vector<std::string>& next_words,
                         const SuggestionRequestFlags& flags) const noexcept {
        if (word.empty()) {
            return SpellingResult::unspecified();
        }
        fl::str::UniString uni_word;
        fl::str::toUniString(word, uni_word);
        auto word_node = dictionaries[0]->words.getOrNull(uni_word);
        if (word_node != nullptr && word_node->is_end_node) {
            return SpellingResult::validWord();
        }

        std::vector<std::unique_ptr<SuggestionCandidate>> results;
        fuzzySearch(dictionaries[0]->words.rootNode(), FuzzySearchType::ProximityWithoutSelf, MAX_COST, flags, word,
                    [&](std::string&& suggested_word, const WordTrieNode* node, int cost) {
                        double confidence = 1.0;
                        //    (static_cast<double>(node->properties.absolute_score) /
                        //    _base_dictionaries[0]->max_unigram_score);

                        auto candidate = std::make_unique<SuggestionCandidate>(
                            SuggestionCandidate {std::move(suggested_word), "", cost, confidence});
                        results.push_back(std::move(candidate));
                        std::sort(results.begin(), results.end(), suggestions_sorter);

                        if (results.size() > flags.maxSuggestionCount()) {
                            results.erase(results.end() - 1);
                        }
                    });

        std::vector<std::string> suggested_corrections;
        for (auto& candidate : results) {
            suggested_corrections.push_back(candidate->text);
        }

        return SpellingResult::typo(suggested_corrections);
    }

    void suggest(const std::string& word, const std::vector<std::string>& prev_words,
                 const SuggestionRequestFlags& flags,
                 std::vector<std::unique_ptr<SuggestionCandidate>>& results) const noexcept {
        results.clear();
        if (word.empty()) return;

        auto& dict = dictionaries[0];

        fuzzySearch(dict->words.rootNode(), FuzzySearchType::ProximityOrPrefix, MAX_COST, flags, word,
                    [&](std::string&& suggested_word, const WordTrieNode* node, int cost) {
                        double confidence =
                            1.0; //(static_cast<double>(node->properties.absolute_score) / dict->max_unigram_score);

                        auto candidate = std::make_unique<SuggestionCandidate>(
                            SuggestionCandidate {suggested_word, "", cost, confidence});
                        results.push_back(std::move(candidate));
                        std::sort(results.begin(), results.end(), suggestions_sorter);

                        if (results.size() > flags.maxSuggestionCount()) {
                            results.erase(results.end() - 1);
                        }
                    });
    }

  private:
    class FuzzySearchState {
      public:
        const NlpSession& session;
        const FuzzySearchType type;
        const int max_distance;
        const SuggestionRequestFlags flags;
        fl::str::UniString uni_word;
        fl::str::UniString uni_word_opposite_case;
        fl::str::UniString prefix_chars;
        std::vector<std::vector<int>> distances;
        std::function<void(std::string&&, const WordTrieNode*, int)> on_result;

        FuzzySearchState(const NlpSession& session, const FuzzySearchType type, const int max_distance,
                         const SuggestionRequestFlags& flags, const std::string& word)
            : session(session), type(type), max_distance(max_distance), flags(flags) {
            initUniWord(word);
            setPrefixUniCharAt(0, "");
        }
        FuzzySearchState(const FuzzySearchState&) = delete;
        FuzzySearchState(FuzzySearchState&&) = delete;
        ~FuzzySearchState() = default;

        void setPrefixUniCharAt(std::size_t prefix_index, const fl::str::UniChar& uni_char) noexcept {
            ensureCapacityFor(prefix_index + 1);
            prefix_chars[prefix_index] = uni_char;
            distances[prefix_index][0] = prefix_index * COST_INSERT;

            if (prefix_index > 0) {
                int penalty;
                int substitution_cost;
                int transpose_cost;

                for (std::size_t i = 1; i < uni_word.size(); i++) {
                    // Calculate penalty
                    if (prefix_index == 1 && i == 1) {
                        penalty = PENALTY_START_OF_STR;
                    } else {
                        penalty = PENALTY_DEFAULT;
                    }

                    // Calculate SUBSTITUTION / IS EQUAL
                    if (uni_word[i] == uni_char) {
                        substitution_cost = COST_IS_EQUAL;
                    } else if (uni_word_opposite_case[i] == uni_char) {
                        substitution_cost = COST_IS_OPPOSITE_CASE; // No penalty even on start of word
                    } else if (prefix_index > 1 && i > 1 && prefix_chars[prefix_index - 1] == uni_word[i] &&
                               uni_char == uni_word[i - 1]) {
                        // TODO: investigate if transpose calculation could be incorrect for certain edge cases
                        substitution_cost = COST_TRANSPOSE - 1 + penalty;
                        //} else if (false && session.key_proximity_mapping.isInProximity(uni_char, uni_word[i])) {
                        //    substitution_cost = COST_SUBSTITUTE_IN_PROXIMITY + penalty;
                    } else {
                        substitution_cost = COST_SUBSTITUTE_DEFAULT + penalty;
                    }

                    distances[prefix_index][i] =
                        std::min(std::min(distances[prefix_index - 1][i] + COST_INSERT, // DELETION
                                          distances[prefix_index][i - 1] + COST_DELETE  // INSERTION
                                          ),
                                 distances[prefix_index - 1][i - 1] + substitution_cost);
                }
            } else {
                for (std::size_t i = 0; i < uni_word.size(); i++) {
                    distances[0][i] = i * COST_INSERT;
                }
            }
        }

        int editDistanceAt(std::size_t prefix_index) const noexcept {
            return distances[prefix_index][uni_word.size() - 1];
        }

        std::string prefixStrAt(std::size_t prefix_index) const noexcept {
            std::string suggested_word;
            for (std::size_t i = 1; i <= prefix_index; i++) {
                suggested_word.append(prefix_chars[i]);
            }
            return suggested_word;
        }

        bool isDeadEndAt(std::size_t prefix_index) const noexcept {
            if (prefix_index < uni_word.size() - 1) {
                return distances[prefix_index][prefix_index] >= max_distance;
            } else {
                return editDistanceAt(prefix_index) >= max_distance;
            }
        }

      private:
        void initUniWord(const std::string& word) noexcept {
            uni_word.emplace_back("");               // Empty top-left cell
            uni_word_opposite_case.emplace_back(""); // Empty top-left cell
            if (word.empty()) return;

            UErrorCode status = U_ZERO_ERROR;
            auto ut = utext_openUTF8(nullptr, word.c_str(), -1, &status);
            auto ub = ubrk_open(UBRK_CHARACTER, session.primary_locale.getLanguage(), nullptr, 0, &status);
            ubrk_setUText(ub, ut, &status);

            if (U_SUCCESS(status)) {
                int32_t prev_n = 0;
                int32_t curr_n = 0;

                while ((curr_n = ubrk_next(ub)) != UBRK_DONE) {
                    auto uni_char = word.substr(prev_n, curr_n - prev_n);
                    auto uni_char_mod(uni_char);
                    fl::str::uppercase(uni_char_mod);
                    if (uni_char != uni_char_mod) {
                        uni_word_opposite_case.push_back(std::move(uni_char_mod));
                    } else {
                        fl::str::lowercase(uni_char_mod);
                        uni_word_opposite_case.push_back(std::move(uni_char_mod));
                    }
                    uni_word.push_back(std::move(uni_char));
                    prev_n = curr_n;
                }
            }

            ubrk_close(ub);
            utext_close(ut);
        }

        void ensureCapacityFor(std::size_t prefix_index) noexcept {
            while (prefix_chars.size() <= prefix_index) {
                prefix_chars.emplace_back("");
            }
            while (distances.size() <= prefix_index) {
                distances.emplace_back(uni_word.size(), 0);
            }
        }
    };

    /**
     * UTF-8 aware fuzzy search algorithm searching a trie and returning all words within a certain given distance.
     *
     * This algorithm utilizes the
     * [Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance) as a bas
     * distance metric, together with a penalty system for unusual operations.
     *
     * TODO: This algorithm is not fully UTF-8 aware yet. Has severe issues outside ASCII in the prefix and chstr area
     */
    void fuzzySearchRecursiveDld(const WordTrieNode* node, FuzzySearchState& state, int prefix_index) const noexcept {
        // Result check
        if (state.editDistanceAt(prefix_index) <= state.max_distance && node->is_end_node) {
            if (node->value.is_possibly_offensive && !state.flags.allowPossiblyOffensive() ||
                node->value.is_hidden_by_user) {
                // Ignore
            } else {
                auto prefix = state.prefixStrAt(prefix_index);
                if (!prefix.empty()) {
                    state.on_result(std::move(prefix), node, state.editDistanceAt(prefix_index));
                }
            }
        }

        // Exit unnecessary recursive loop
        if (state.isDeadEndAt(prefix_index)) {
            return;
        }

        for (auto& [uni_char, child_node] : node->children) {
            state.setPrefixUniCharAt(prefix_index + 1, uni_char);
            fuzzySearchRecursiveDld(child_node.get(), state, prefix_index + 1);
        }
    }

    void fuzzySearch(const WordTrieNode* root_node, FuzzySearchType type, int max_distance,
                     const SuggestionRequestFlags& flags, const std::string& word,
                     std::function<void(std::string&&, const WordTrieNode*, int)> on_result) const noexcept {
        if (word.empty()) return;

        FuzzySearchState state(*this, type, max_distance, flags, word);
        state.on_result = std::move(on_result);
        fuzzySearchRecursiveDld(root_node, state, 0);
    }

    static bool suggestions_sorter(const std::unique_ptr<fl::nlp::SuggestionCandidate>& a,
                                   const std::unique_ptr<fl::nlp::SuggestionCandidate>& b) {
        if (a->edit_distance == b->edit_distance) {
            return a->confidence > b->confidence;
        }
        return a->edit_distance < b->edit_distance && a->confidence * 100.0 > b->confidence;
    }
};

} // namespace fl::nlp
