/*
 * Copyright 2022 Patrick Goldinger
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

#include "core/dictionary_session.hpp"

#include "core/dictionary.hpp"
#include "core/string.hpp"

#include <unicode/ubrk.h>
#include <unicode/utext.h>

#include <cmath>

namespace fl::nlp {

void DictionarySession::loadBaseDictionary(const std::filesystem::path& dict_path) {
    auto base_dict = std::make_unique<Dictionary>(dict_path);
    _base_dictionaries.push_back(std::move(base_dict));
}

void DictionarySession::loadUserDictionary(const std::filesystem::path& dict_path) {
    _user_dictionary = std::make_unique<MutableDictionary>(dict_path);
}

bool strcmp(const fl::u8chstr_vec& word_chars, const fl::u8chstr_vec& prefix_chars, std::size_t prefix_index) {
    if (word_chars.size() != prefix_index) return false;
    for (int i = 0; i < word_chars.size(); i++) {
        if (word_chars[i] != prefix_chars[i]) return false;
    }
    return true;
}

void DictionarySession::fuzzySearchRecursiveDld(const TrieNode* node, FuzzySearchState& state,
                                                int prefix_index) const noexcept {
    // Result check
    if (state.editDistanceAt(prefix_index) <= state.max_distance && node->is_terminal) {
        if (node->properties.is_possibly_offensive && !state.flags.allowPossiblyOffensive() ||
            node->properties.is_hidden_by_user) {
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

    for (auto& [chstr, child_node] : node->children) {
        state.setPrefixChstrAt(prefix_index + 1, chstr);
        fuzzySearchRecursiveDld(child_node.get(), state, prefix_index + 1);
    }
}

void DictionarySession::fuzzySearch(const TrieNode* root_node, FuzzySearchType type, int max_distance,
                                    const SuggestionRequestFlags& flags, const fl::u8str& word,
                                    std::function<void(fl::u8str&&, const TrieNode*, int)> on_result) const noexcept {
    if (word.empty()) return;

    FuzzySearchState state(*this, type, max_distance, flags, word);
    state.on_result = on_result;
    fuzzySearchRecursiveDld(root_node, state, 0);
}

bool suggestions_sorter(const std::unique_ptr<fl::nlp::SuggestionCandidate>& a,
                        const std::unique_ptr<fl::nlp::SuggestionCandidate>& b) {
    if (a->edit_distance == b->edit_distance) {
        return a->confidence > b->confidence;
    }
    return a->edit_distance < b->edit_distance && a->confidence * 100.0 > b->confidence;
}

SpellingResult DictionarySession::spell(fl::u8str& word, const std::vector<fl::u8str>& prev_words,
                                        const std::vector<fl::u8str>& next_words, SuggestionRequestFlags& flags) {
    if (word.empty()) {
        return SpellingResult::unspecified();
    }
    fl::u8chstr_vec word_chars;
    fl::chstr::strToVec(word, word_chars);
    auto word_node = _base_dictionaries[0]->root_node.resolveKey(word_chars);
    if (word_node != nullptr && word_node->is_terminal) {
        return SpellingResult::validWord();
    }

    std::vector<std::unique_ptr<SuggestionCandidate>> results;
    fuzzySearch(&_base_dictionaries[0]->root_node, FuzzySearchType::ProximityWithoutSelf, MAX_COST, flags, word,
                [&](fl::u8str&& suggested_word, const fl::nlp::TrieNode* node, int cost) {
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

    std::vector<fl::u8str> suggested_corrections;
    for (auto& candidate : results) {
        suggested_corrections.push_back(std::move(candidate->text));
    }

    return SpellingResult::typo(suggested_corrections);
}

void DictionarySession::suggest(fl::u8str& word, const std::vector<fl::u8str>& prev_words,
                                SuggestionRequestFlags& flags,
                                std::vector<std::unique_ptr<SuggestionCandidate>>& results) {
    results.clear();
    if (word.empty()) return;

    auto& dict = _base_dictionaries[0];

    fuzzySearch(&dict->root_node, FuzzySearchType::ProximityOrPrefix, MAX_COST, flags, word,
                [&](fl::u8str&& suggested_word, const fl::nlp::TrieNode* node, int cost) {
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

// ----- # FuzzySearchState # -----

DictionarySession::FuzzySearchState::FuzzySearchState(const DictionarySession& session, const FuzzySearchType type,
                                                      const int max_distance, const SuggestionRequestFlags& flags,
                                                      const fl::u8str& word)
    : session(session), type(type), max_distance(max_distance), flags(flags) {
    initWordChars(word);
    setPrefixChstrAt(0, "");
}

void DictionarySession::FuzzySearchState::setPrefixChstrAt(std::size_t prefix_index,
                                                           const fl::u8chstr& chstr) noexcept {
    ensureCapacityFor(prefix_index + 1);
    prefix_chars[prefix_index] = chstr;
    distances[prefix_index][0] = prefix_index * COST_INSERT;

    if (prefix_index > 0) {
        int penalty;
        int substitution_cost;
        int transpose_cost;

        for (std::size_t i = 1; i < word_chars.size(); i++) {
            // Calculate penalty
            if (prefix_index == 1 && i == 1) {
                penalty = PENALTY_START_OF_STR;
            } else {
                penalty = PENALTY_DEFAULT;
            }

            // Calculate SUBSTITUTION / IS EQUAL
            if (word_chars[i] == chstr) {
                substitution_cost = COST_IS_EQUAL;
            } else if (word_chars_opposite_case[i] == chstr) {
                substitution_cost = COST_IS_OPPOSITE_CASE; // No penalty even on start of word
            } else if (prefix_index > 1 && i > 1 && prefix_chars[prefix_index - 1] == word_chars[i] &&
                       chstr == word_chars[i - 1]) {
                // TODO: investigate if transpose calculation could be incorrect for certain edge cases
                substitution_cost = COST_TRANSPOSE - 1 + penalty;
                //} else if (false && session.key_proximity_mapping.isInProximity(chstr, word_chars[i])) {
                //    substitution_cost = COST_SUBSTITUTE_IN_PROXIMITY + penalty;
            } else {
                substitution_cost = COST_SUBSTITUTE_DEFAULT + penalty;
            }

            distances[prefix_index][i] = std::min(std::min(distances[prefix_index - 1][i] + COST_INSERT, // DELETION
                                                           distances[prefix_index][i - 1] + COST_DELETE  // INSERTION
                                                           ),
                                                  distances[prefix_index - 1][i - 1] + substitution_cost);
        }
    } else {
        for (std::size_t i = 0; i < word_chars.size(); i++) {
            distances[0][i] = i * COST_INSERT;
        }
    }
}

int DictionarySession::FuzzySearchState::editDistanceAt(std::size_t prefix_index) const noexcept {
    return distances[prefix_index][word_chars.size() - 1];
}

fl::u8str DictionarySession::FuzzySearchState::prefixStrAt(std::size_t prefix_index) const noexcept {
    fl::u8str suggested_word;
    for (std::size_t i = 1; i <= prefix_index; i++) {
        suggested_word.append(prefix_chars[i]);
    }
    return suggested_word;
}

bool DictionarySession::FuzzySearchState::isDeadEndAt(std::size_t prefix_index) const noexcept {
    if (prefix_index < word_chars.size() - 1) {
        return distances[prefix_index][prefix_index] >= max_distance;
    } else {
        return editDistanceAt(prefix_index) >= max_distance;
    }
}

void DictionarySession::FuzzySearchState::initWordChars(const fl::u8str& word) noexcept {
    word_chars.push_back("");               // Empty top-left cell
    word_chars_opposite_case.push_back(""); // Empty top-left cell
    if (word.empty()) return;

    UErrorCode status = U_ZERO_ERROR;
    auto ut = utext_openUTF8(nullptr, word.c_str(), -1, &status);
    auto ub = ubrk_open(UBRK_CHARACTER, session.locale_tag.c_str(), nullptr, 0, &status);
    ubrk_setUText(ub, ut, &status);

    if (U_SUCCESS(status)) {
        int32_t prev_n = 0;
        int32_t curr_n = 0;

        while ((curr_n = ubrk_next(ub)) != UBRK_DONE) {
            auto chstr = word.substr(prev_n, curr_n - prev_n);
            auto chstr_mod(chstr);
            fl::str::uppercase(chstr_mod);
            if (chstr != chstr_mod) {
                word_chars_opposite_case.push_back(std::move(chstr_mod));
            } else {
                fl::str::lowercase(chstr_mod);
                word_chars_opposite_case.push_back(std::move(chstr_mod));
            }
            word_chars.push_back(std::move(chstr));
            prev_n = curr_n;
        }
    }

    ubrk_close(ub);
    utext_close(ut);
}

void DictionarySession::FuzzySearchState::ensureCapacityFor(std::size_t prefix_index) noexcept {
    while (prefix_chars.size() <= prefix_index) {
        prefix_chars.push_back("");
    }
    while (distances.size() <= prefix_index) {
        distances.push_back(std::vector(word_chars.size(), 0));
    }
}

} // namespace fl::nlp
