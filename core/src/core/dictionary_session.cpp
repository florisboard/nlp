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

#include <unicode/ubrk.h>
#include <unicode/utext.h>

#include <cmath>

namespace fl::nlp {

void dictionary_session::load_base_dictionary(const std::filesystem::path& dict_path) {
    auto base_dict = std::make_unique<dictionary>(dict_path);
    _base_dictionaries.push_back(std::move(base_dict));
}

void dictionary_session::load_user_dictionary(const std::filesystem::path& dict_path) {
    _user_dictionary = std::make_unique<mutable_dictionary>(dict_path);
}

bool strcmp(const std::vector<fl::u8str>& a, const std::vector<fl::u8str>& b) {
    if (a.size() != b.size()) return false;
    for (int i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

void dictionary_session::fuzzy_search_recursive_dld(const trie_node* node, fuzzy_search_state& state, int prefix_index)
    const noexcept {
    // Result check
    if (state.edit_distance_at(prefix_index) <= state.max_distance && node->is_terminal) {
        auto prefix = state.prefix_str_at(prefix_index);
        if (!prefix.empty()) {
            state.on_result(prefix, node, state.edit_distance_at(prefix_index));
        }
    }

    // Exit unnecessary recursive loop
    if (state.is_dead_end_at(prefix_index)) {
        return;
    }

    for (auto& [ch, child_node] : node->children) {
        // FIXME: below line is not correct for UTF-8 characters!!
        fl::u8str chstr(1, ch);
        state.set_prefix_chstr_at(prefix_index + 1, chstr);
        fuzzy_search_recursive_dld(child_node.get(), state, prefix_index + 1);
    }
}

void dictionary_session::fuzzy_search(
    const trie_node* root_node,
    fuzzy_search_type type,
    int max_distance,
    const fl::u8str& word,
    std::function<void(const fl::u8str&, const trie_node*, int)> on_result
) const noexcept {
    if (word.empty()) return;

    fuzzy_search_state state(*this, type, max_distance, word);
    state.on_result = on_result;
    fuzzy_search_recursive_dld(root_node, state, 0);
}

bool suggestions_sorter(
    std::unique_ptr<fl::nlp::suggestion_candidate>& a,
    std::unique_ptr<fl::nlp::suggestion_candidate>& b
) {
    if (a->edit_distance == b->edit_distance) {
        return a->confidence > b->confidence;
    }
    return a->edit_distance < b->edit_distance && a->confidence * 100.0 > b->confidence;
}

spelling_result dictionary_session::spell(
    fl::u8str& word,
    const std::vector<fl::u8str>& prev_words,
    const std::vector<fl::u8str>& next_words,
    suggestion_request_flags& flags
) {
    if (word.empty()) {
        return spelling_result::unspecified();
    }
    auto word_node = _base_dictionaries[0]->root_node.resolve_key(word);
    if (word_node != nullptr && word_node->is_terminal) {
        return spelling_result::valid_word();
    }

    std::vector<std::unique_ptr<suggestion_candidate>> results;
    fuzzy_search(
        &_base_dictionaries[0]->root_node, fuzzy_search_type::proximity_without_self, MAX_COST, word,
        [&](const fl::u8str& suggested_word, const fl::nlp::trie_node* node, int cost) {
            double confidence =
                (static_cast<double>(node->properties.absolute_score) / _base_dictionaries[0]->max_unigram_score);

            auto existing_candidate = std::find_if(results.begin(), results.end(), [&](auto& candidate) -> bool {
                return candidate->text == suggested_word;
            });
            if (existing_candidate != results.end()) {
                cost = std::min(cost, (*existing_candidate)->edit_distance);
                confidence = std::max(confidence, (*existing_candidate)->confidence);
                results.erase(existing_candidate);
            }

            auto candidate =
                std::make_unique<suggestion_candidate>(suggestion_candidate { suggested_word, "", cost, confidence });
            results.push_back(std::move(candidate));
            std::sort(results.begin(), results.end(), suggestions_sorter);

            if (results.size() > flags.max_suggestion_count()) {
                results.erase(results.end());
            }
        }
    );

    std::vector<fl::u8str> suggested_corrections;
    for (auto& candidate : results) {
        suggested_corrections.push_back(std::move(candidate->text));
    }

    return spelling_result::typo(suggested_corrections);
}

void dictionary_session::suggest(
    fl::u8str& word,
    const std::vector<fl::u8str>& prev_words,
    suggestion_request_flags& flags,
    std::vector<std::unique_ptr<suggestion_candidate>>& results
) {
    results.clear();
    if (word.empty()) return;

    fuzzy_search(
        &_base_dictionaries[0]->root_node, fuzzy_search_type::proximity_or_prefix, MAX_COST, word,
        [&](const fl::u8str& suggested_word, const fl::nlp::trie_node* node, int cost) {
            double confidence =
                (static_cast<double>(node->properties.absolute_score) / _base_dictionaries[0]->max_unigram_score);

            auto existing_candidate = std::find_if(results.begin(), results.end(), [&](auto& candidate) -> bool {
                return candidate->text == suggested_word;
            });
            if (existing_candidate != results.end()) {
                cost = std::min(cost, (*existing_candidate)->edit_distance);
                confidence = std::max(confidence, (*existing_candidate)->confidence);
                results.erase(existing_candidate);
            }

            auto candidate =
                std::make_unique<suggestion_candidate>(suggestion_candidate { suggested_word, "", cost, confidence });
            results.push_back(std::move(candidate));
            std::sort(results.begin(), results.end(), suggestions_sorter);

            if (results.size() > flags.max_suggestion_count()) {
                results.erase(results.end());
            }
        }
    );
}

// ----- # fuzzy_search_state # -----

dictionary_session::fuzzy_search_state::fuzzy_search_state(
    const dictionary_session& session,
    const fuzzy_search_type type,
    const int max_distance,
    const fl::u8str& word
)
    : session(session), type(type), max_distance(max_distance) {
    init_word_chars(word);
    set_prefix_chstr_at(0, "");
}

void dictionary_session::fuzzy_search_state::set_prefix_chstr_at(
    std::size_t prefix_index,
    const fl::u8str& chstr
) noexcept {
    ensure_capacity_for(prefix_index + 1);
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
            } else if (prefix_index > 1 && i > 1 && prefix_chars[prefix_index - 1] == word_chars[i] && chstr == word_chars[i - 1]) {
                // TODO: investigate if transpose calculation could be incorrect for certain edge cases
                substitution_cost = COST_TRANSPOSE - 1 + penalty;
                //} else if (false && session.key_proximity_mapping.is_in_proximity(chstr, word_chars[i])) {
                //    substitution_cost = COST_SUBSTITUTE_IN_PROXIMITY + penalty;
            } else {
                substitution_cost = COST_SUBSTITUTE_DEFAULT + penalty;
            }

            distances[prefix_index][i] = std::min(
                std::min(
                    distances[prefix_index - 1][i] + COST_INSERT, // DELETION
                    distances[prefix_index][i - 1] + COST_DELETE  // INSERTION
                ),
                distances[prefix_index - 1][i - 1] + substitution_cost
            );
        }
    } else {
        for (std::size_t i = 0; i < word_chars.size(); i++) {
            distances[0][i] = i * COST_INSERT;
        }
    }
}

int dictionary_session::fuzzy_search_state::edit_distance_at(std::size_t prefix_index) const noexcept {
    return distances[prefix_index][word_chars.size() - 1];
}

fl::u8str dictionary_session::fuzzy_search_state::prefix_str_at(std::size_t prefix_index) const noexcept {
    fl::u8str suggested_word;
    for (std::size_t i = 1; i <= prefix_index; i++) {
        suggested_word.append(prefix_chars[i]);
    }
    return suggested_word;
}

bool dictionary_session::fuzzy_search_state::is_dead_end_at(std::size_t prefix_index) const noexcept {
    if (prefix_index < word_chars.size() - 1) {
        return distances[prefix_index][prefix_index] >= max_distance;
    } else {
        return edit_distance_at(prefix_index) >= max_distance;
    }
}

void dictionary_session::fuzzy_search_state::init_word_chars(const fl::u8str& word) noexcept {
    word_chars.push_back(""); // Empty top-left cell
    if (word.empty()) return;

    UErrorCode status = U_ZERO_ERROR;
    auto ut = utext_openUTF8(nullptr, word.c_str(), -1, &status);
    auto ub = ubrk_open(UBRK_CHARACTER, session.locale_tag.c_str(), nullptr, 0, &status);
    ubrk_setUText(ub, ut, &status);

    if (U_SUCCESS(status)) {
        int32_t prev_n = 0;
        int32_t curr_n = 0;

        while ((curr_n = ubrk_next(ub)) != UBRK_DONE) {
            word_chars.push_back(word.substr(prev_n, curr_n - prev_n));
            prev_n = curr_n;
        }
    }

    utext_close(ut);
    ubrk_close(ub);
}

void dictionary_session::fuzzy_search_state::ensure_capacity_for(std::size_t prefix_index) noexcept {
    while (prefix_chars.size() <= prefix_index) {
        prefix_chars.push_back("");
    }
    while (distances.size() <= prefix_index) {
        distances.push_back(std::vector(word_chars.size(), 0));
    }
}

} // namespace fl::nlp
