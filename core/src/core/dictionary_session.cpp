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
#include <fstream>

namespace fl::nlp {

void dictionary_session::load_base_dictionary(const std::filesystem::path& dict_path) {
    auto base_dict = std::make_unique<dictionary>(dict_path);
    base_dictionaries.push_back(std::move(base_dict));
}

void dictionary_session::load_user_dictionary(const std::filesystem::path& dict_path) {
    user_dictionary = std::make_unique<mutable_dictionary>(dict_path);
}

static const int MAX_COST = 3;

static const int COST_IS_EQUAL = 0;
static const int COST_INSERT = 1;
static const int COST_DELETE = 1;
static const int COST_SUBSTITUTE = 1;
static const int COST_TRANSPOSE = 1;
static const int PENALTY_DEFAULT = 0;
static const int PENALTY_START_OF_STR = 1;
static const int PENALTY_END_OF_STR = 1;

/**
 * UTF-8 aware fuzzy search algorithm searching a trie and returning all words within a certain given distance.
 *
 * This algorithm utilizes the
 * [Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance) as a bas
 * distance metric, together with a penalty system for unusual operations.
 *
 * TODO: This algorithm is not fully UTF-8 aware yet. Has severe issues outside ASCII in the prefix and chstr area
 */
void fuzzy_search_recursive_dld(
    const basic_trie_node* node,
    const std::vector<fl::u8str_view>& word_chars,
    int word_index,
    std::vector<fl::u8str>& prefix_chars,
    int prefix_index,
    int edit_distance, // = word.length if root node
    int max_distance,
    std::function<void(const fl::u8str&, const basic_trie_node*, int)> on_result
) {
    // Result check
    if (word_index >= word_chars.size() && edit_distance <= max_distance && node->is_terminal &&
        !prefix_chars.empty()) {
        fl::u8str suggested_word;
        for (int i = 0; i < prefix_index; i++) {
            suggested_word.append(prefix_chars[i]);
        }
        on_result(suggested_word, node, edit_distance);
    }

    // Exit unnecessary recursive loop
    if (edit_distance - (word_chars.size() - word_index - 1) > max_distance) {
        return;
    }

    // Calculate penalty
    int penalty;
    if (word_index == 0 && edit_distance == word_chars.size()) {
        penalty = PENALTY_START_OF_STR;
    } else if (word_index + 1 == word_chars.size()) {
        penalty = PENALTY_END_OF_STR;
    } else {
        penalty = PENALTY_DEFAULT;
    }

    // DELETE
    if (word_index < word_chars.size()) {
        fuzzy_search_recursive_dld(
            node, word_chars, word_index + 1, prefix_chars, prefix_index, edit_distance - 1 + COST_DELETE + penalty,
            max_distance, on_result
        );
    }

    for (std::size_t i = 0; i < node->children.size(); i++) {
        auto& child_node = node->children.at(i);
        if (child_node == nullptr) continue;

        auto chstr = fl::u8str(1, node->children.to_char(i));
        if (prefix_index == prefix_chars.size()) {
            prefix_chars.push_back(chstr);
        } else {
            prefix_chars[prefix_index] = chstr;
        }

        if (word_index < word_chars.size()) {
            // TRANSPOSE
            if (word_index + 1 < word_chars.size() && word_chars[word_index] != word_chars[word_index + 1] &&
                chstr == word_chars[word_index + 1]) {
                auto sub_chstr = std::string(word_chars[word_index]);
                auto sub_child_node = child_node->resolve_key_or_null_const(sub_chstr);
                if (sub_child_node != nullptr) {
                    if (prefix_index + 1 == prefix_chars.size()) {
                        prefix_chars.push_back(sub_chstr);
                    } else {
                        prefix_chars[prefix_index + 1] = sub_chstr;
                    }
                    fuzzy_search_recursive_dld(
                        sub_child_node, word_chars, word_index + 2, prefix_chars, prefix_index + 2,
                        edit_distance - 2 + COST_TRANSPOSE + penalty, max_distance, on_result
                    );
                }
            }

            // SUBSTITUTION / IS EQUAL
            int substitution_cost = (word_chars[word_index] == chstr) ? COST_IS_EQUAL : (COST_SUBSTITUTE + penalty);
            fuzzy_search_recursive_dld(
                child_node.get(), word_chars, word_index + 1, prefix_chars, prefix_index + 1,
                edit_distance - 1 + substitution_cost, max_distance, on_result
            );
        }

        // INSERT
        fuzzy_search_recursive_dld(
            child_node.get(), word_chars, word_index, prefix_chars, prefix_index + 1,
            edit_distance + COST_INSERT + penalty, max_distance, on_result
        );
    }
}

void fuzzy_search(
    const basic_trie_node* node,
    const fl::u8str& word,
    const fl::u8str& locale_tag,
    int max_distance,
    std::function<void(const fl::u8str&, const basic_trie_node*, int)> on_result
) {
    if (word.empty()) return;

    UErrorCode status = U_ZERO_ERROR;
    auto ut = utext_openUTF8(nullptr, word.c_str(), -1, &status);
    auto ub = ubrk_open(UBRK_CHARACTER, locale_tag.c_str(), nullptr, 0, &status);
    ubrk_setUText(ub, ut, &status);

    if (U_SUCCESS(status)) {
        std::vector<fl::u8str_view> word_chars;
        std::vector<fl::u8str> prefix_chars;
        int32_t prev_n = 0;
        int32_t curr_n = 0;
        while ((curr_n = ubrk_next(ub)) != UBRK_DONE) {
            word_chars.push_back(std::string_view(word).substr(prev_n, curr_n - prev_n));
            prev_n = curr_n;
        }
        fuzzy_search_recursive_dld(node, word_chars, 0, prefix_chars, 0, word_chars.size(), max_distance, on_result);
    }

    utext_close(ut);
    ubrk_close(ub);
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
    if (base_dictionaries[0]->root_node.resolve_key_or_null_const(word) != nullptr) {
        return spelling_result::valid_word();
    }

    std::vector<std::unique_ptr<suggestion_candidate>> results;
    fuzzy_search(
        &base_dictionaries[0]->root_node, word, "en_us", MAX_COST,
        [&](const fl::u8str& suggested_word, const fl::nlp::basic_trie_node* node, int cost) {
            double confidence =
                (static_cast<double>(node->properties.absolute_score) / base_dictionaries[0]->max_unigram_score);

            auto existing_candidate = std::find_if(results.begin(), results.end(), [&](auto& candidate) -> bool {
                return candidate->text == suggested_word;
            });
            if (existing_candidate != results.end()) {
                cost = std::min(cost, (*existing_candidate)->edit_distance);
                confidence = std::max(confidence, (*existing_candidate)->confidence);
                results.erase(existing_candidate);
            }

            auto candidate = std::make_unique<suggestion_candidate>(suggestion_candidate { suggested_word, std::nullopt,
                                                                                           cost, confidence });
            results.push_back(std::move(candidate));
            std::sort(results.begin(), results.end(), [](auto& a, auto& b) -> bool {
                if (a->edit_distance == b->edit_distance) {
                    return a->confidence > b->confidence;
                }
                return a->edit_distance < b->edit_distance && a->confidence * 100.0 > b->confidence;
            });

            if (results.size() > flags.max_suggestion_count()) {
                results.erase(results.end());
            }
        }
    );

    std::vector<fl::u8str> suggested_corrections;
    std::transform(
        results.begin(), results.end(), suggested_corrections.begin(),
        [](auto& candidate) -> auto{ return candidate->text; }
    );

    return spelling_result::typo(std::make_optional(std::move(suggested_corrections)));
}

void dictionary_session::suggest(
    fl::u8str& word,
    const std::vector<fl::u8str>& prev_words,
    suggestion_request_flags& flags,
    std::vector<std::unique_ptr<suggestion_candidate>>& results
) {
    results.clear();
    if (word.empty()) return;
    std::ofstream test;
    test.open("test.txt");

    fuzzy_search(
        &base_dictionaries[0]->root_node, word, "en_us", MAX_COST,
        [&](const fl::u8str& suggested_word, const fl::nlp::basic_trie_node* node, int cost) {
            test << suggested_word << " " << cost << "\n";
            // double confidence =
            //     ((1.0 / pow(static_cast<double>(cost + 1), 2.0)) +
            //      (static_cast<double>(node->properties.absolute_score) / base_dictionaries[0]->max_unigram_score)) /
            //     2.0;
            double confidence =
                (static_cast<double>(node->properties.absolute_score) / base_dictionaries[0]->max_unigram_score);

            auto existing_candidate = std::find_if(results.begin(), results.end(), [&](auto& candidate) -> bool {
                return candidate->text == suggested_word;
            });
            if (existing_candidate != results.end()) {
                cost = std::min(cost, (*existing_candidate)->edit_distance);
                confidence = std::max(confidence, (*existing_candidate)->confidence);
                results.erase(existing_candidate);
            }

            auto candidate = std::make_unique<suggestion_candidate>(suggestion_candidate { suggested_word, std::nullopt,
                                                                                           cost, confidence });
            results.push_back(std::move(candidate));
            std::sort(results.begin(), results.end(), [](auto& a, auto& b) -> bool {
                if (a->edit_distance == b->edit_distance) {
                    return a->confidence > b->confidence;
                }
                return a->edit_distance < b->edit_distance && a->confidence * 100.0 > b->confidence;
            });

            if (results.size() > flags.max_suggestion_count()) {
                results.erase(results.end());
            }
        }
    );
}

} // namespace fl::nlp
