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

#include "core/dictionary.hpp"

#include <unicode/utypes.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fl::nlp {

// ----- dictionary_header ----- //

size_t dictionary_header::read_from(std::basic_istream<fl::u8char>& istream) noexcept {
    size_t line_count;
    fl::u8str line;
    fl::u8str key;
    fl::u8str value;
    while (std::getline(istream, line)) {
        line_count++;
        fl::str::trim(line);
        if (line.empty()) break;
        auto pos = std::find(line.begin(), line.end(), FLDIC_ASSIGNMENT);
        if (pos == line.end()) continue;
        key.assign(line.begin(), pos);
        value.assign(pos + 1, line.end());
        fl::str::trim(key);
        fl::str::trim(value);
        if (value.empty()) continue;
        if (key == FLDIC_HEADER_SCHEMA) {
            schema = value;
        } else if (key == FLDIC_HEADER_NAME) {
            name = value;
        } else if (key == FLDIC_HEADER_LOCALES) {
            std::vector<fl::u8str> locale_tags;
            fl::str::split(value, FLDIC_LIST_SEPARATOR, locale_tags);
            for (auto& locale_tag : locale_tags) {
                UErrorCode status;
                auto locale = icu::Locale::forLanguageTag(locale_tag, status);
                if (U_SUCCESS(status)) {
                    locales.push_back(locale);
                }
            }
        } else if (key == FLDIC_HEADER_GENERATED_BY) {
            generated_by = value;
        } else {
            // Ignore this header line
        }
    }
    return line_count;
}

size_t dictionary_header::write_to(std::basic_ostream<fl::u8char>& ostream) const noexcept {
    size_t line_count = 3;
    ostream << FLDIC_HEADER_SCHEMA << FLDIC_ASSIGNMENT << schema << FLDIC_NEWLINE;
    ostream << FLDIC_HEADER_NAME << FLDIC_ASSIGNMENT << name << FLDIC_NEWLINE;
    std::stringstream ss;
    int i = 0;
    for (auto& locale : locales) {
        if (i++ > 0) ss << FLDIC_LIST_SEPARATOR;
        UErrorCode status;
        ss << locale.toLanguageTag<fl::u8str>(status);
        if (U_FAILURE(status)) {
            i--;
        }
    }
    auto locale_tags = ss.str();
    if (!locale_tags.empty()) {
        ostream << FLDIC_HEADER_LOCALES << FLDIC_ASSIGNMENT << locale_tags << FLDIC_NEWLINE;
        line_count++;
    }
    ostream << FLDIC_HEADER_GENERATED_BY << FLDIC_ASSIGNMENT << generated_by << FLDIC_NEWLINE << FLDIC_NEWLINE;
    return line_count;
}

void dictionary_header::reset() noexcept {
    schema.assign(FLDIC_SCHEMA_V0_DRAFT1);
    name.clear();
    locales.clear();
    generated_by.clear();
}

// ----- dictionary ----- //

dictionary::dictionary() = default;

dictionary::dictionary(const std::filesystem::path& path) : dictionary(path, path) {};

dictionary::dictionary(const std::filesystem::path& src_path, const std::filesystem::path& dst_path)
    : src_path(src_path), dst_path(dst_path), max_unigram_score(0), max_bigram_score(0), max_trigram_score(0) {
    std::ifstream dict_file;
    dict_file.open(src_path);
    if (dict_file.is_open()) {
        deserialize(dict_file);
        dict_file.close();
    } else {
        throw std::runtime_error("File not open!");
    }
}

dictionary::~dictionary() = default;

void dictionary::deserialize(std::basic_istream<fl::u8char>& istream) {
    size_t line_num = 1 + header.read_from(istream);
    fl::u8str line;
    std::vector<fl::u8str> line_split_results;
    fl::u8str word;
    uint8_t prev_ngram_level = 1;
    std::array<trie_node*, 9> prev_parent_nodes { &root_node, nullptr };
    while (std::getline(istream, line)) {
        line_num++;
        // TODO: Add proper section support
        if (line.starts_with('[')) continue;

        // Set up node
        ngram_properties properties;

        // Get current ngram level
        auto word_begin = std::find_if_not(line.begin(), line.end(), [](auto ch) { return ch == FLDIC_SEPARATOR; });
        auto ngram_level = std::distance(line.begin(), word_begin) + 1;
        if (ngram_level > 8) {
            throw_fatal_deseralization_error(line_num, "Cannot read/process ngram levels greater than 8!");
        } else if (ngram_level <= 0) {
            throw_fatal_deseralization_error(line_num, "ngram level <= 0");
        }
        line.erase(line.begin(), word_begin);

        // Find which node is responsible for getting the new node attached to
        if (ngram_level - prev_ngram_level > 1) {
            throw_fatal_deseralization_error(line_num, "Invalid definition of n-gram or bad formatting!");
        }
        trie_node* parent_node = prev_parent_nodes[ngram_level - 1];
        if (parent_node == nullptr) {
            if (ngram_level <= 1) {
                throw_fatal_deseralization_error(
                    line_num, "Encountered an ngram which does not have a corresponding parent!"
                );
            } else {
                auto new_node = prev_parent_nodes[ngram_level - 2]->subsequent_words_or_create();
                prev_parent_nodes[ngram_level - 1] = new_node;
                parent_node = new_node;
            }
        }

        // Read in node data
        fl::str::split(line, FLDIC_SEPARATOR, line_split_results);
        if (line_split_results.size() < 2) continue;
        // Word
        fl::str::trim(line_split_results[0]);
        word.assign(line_split_results[0]);
        if (word.empty()) continue;
        // Score
        fl::str::trim(line_split_results[1]);
        properties.absolute_score = std::stoi(line_split_results[1]);
        // Parameters (Optional)
        if (line_split_results.size() > 2) {
            fl::str::trim(line_split_results[2]);
            for (auto f : line_split_results[2]) {
                if (f == FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE) {
                    properties.is_possibly_offensive = true;
                } else if (f == FLDIC_FLAG_IS_HIDDEN_BY_USER) {
                    properties.is_hidden_by_user = true;
                }
            }
        }

        // Insert node into trie
        auto node = parent_node->insert(word);
        node->properties = properties;
        prev_ngram_level = ngram_level;
        if (ngram_level == 1 && max_unigram_score < properties.absolute_score) {
            max_unigram_score = properties.absolute_score;
        } else if (ngram_level == 2 && max_bigram_score < properties.absolute_score) {
            max_bigram_score = properties.absolute_score;
        } else if (ngram_level == 3 && max_trigram_score < properties.absolute_score) {
            max_trigram_score = properties.absolute_score;
        }
    }
}

void dictionary::serialize(std::basic_ostream<fl::u8char>& ostream) {
    header.write_to(ostream);
    ostream << FLDIC_SECTION_WORDS << FLDIC_NEWLINE;
    trie_write_ngrams_to(ostream, &root_node, 1);
}

void dictionary::trie_write_ngrams_to(
    std::basic_ostream<fl::u8char>& ostream,
    trie_node* base_node,
    uint8_t ngram_level
) const {
    if (base_node == nullptr) return;
    base_node->for_each([&](const fl::u8str& word, trie_node* node) {
        for (size_t n = 1; n < ngram_level; n++) {
            ostream << FLDIC_SEPARATOR;
        }
        ostream << word << FLDIC_SEPARATOR << node->properties.absolute_score;
        if (node->properties.is_possibly_offensive || node->properties.is_hidden_by_user) {
            ostream << FLDIC_SEPARATOR;
            if (node->properties.is_possibly_offensive) {
                ostream << FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE;
            }
            if (node->properties.is_hidden_by_user) {
                ostream << FLDIC_FLAG_IS_HIDDEN_BY_USER;
            }
        }
        ostream << FLDIC_NEWLINE;
        trie_write_ngrams_to(ostream, node->subsequent_words(), ngram_level + 1);
    });
}

void dictionary::throw_fatal_deseralization_error(size_t line_num, const char* msg) const {
    throw dictionary_serialization_error(src_path, line_num, msg);
}

// ----- mutable_dictionary ----- //

mutable_dictionary::mutable_dictionary() : dictionary() {};

mutable_dictionary::mutable_dictionary(const std::filesystem::path& path) : dictionary(path) {}

mutable_dictionary::mutable_dictionary(const std::filesystem::path& src_path, const std::filesystem::path& dst_path)
    : dictionary(src_path, dst_path) {}

mutable_dictionary::~mutable_dictionary() = default;

bool mutable_dictionary::adjust_scores_if_necessary() noexcept {
    /*bool adj_unigrams = max_unigram_score > SCORE_ADJUSTMENT_THRESHOLD;
    bool adj_bigrams = max_bigram_score > SCORE_ADJUSTMENT_THRESHOLD;
    bool adj_trigrams = max_trigram_score > SCORE_ADJUSTMENT_THRESHOLD;

    if (!adj_unigrams && !adj_bigrams && !adj_trigrams) return false;

    for (auto it1 = root_node.begin(); it1 != ngram_tree.end(); it1++) {
        auto& node1 = it1.value();
        if (adj_unigrams) {
            node1.properties.absolute_score /= 2;
        }
        if (adj_bigrams || adj_trigrams) {
            for (auto it2 = node1.ngram_tree.begin(); it2 != node1.ngram_tree.end(); it2++) {
                auto& node2 = it2.value();
                if (adj_bigrams) {
                    node2.properties.absolute_score /= 2;
                }
                if (adj_trigrams) {
                    for (auto it3 = node2.ngram_tree.begin(); it3 != node2.ngram_tree.end(); it3++) {
                        auto& node3 = it3.value();
                        if (adj_trigrams) {
                            node3.properties.absolute_score /= 2;
                        }
                    }
                }
            }
        }
    }

    if (adj_unigrams) max_unigram_score /= 2;
    if (adj_bigrams) max_bigram_score /= 2;
    if (adj_trigrams) max_trigram_score /= 2;*/

    return true;
}

ngram_properties& mutable_dictionary::insert(const fl::u8str& word1) noexcept {
    return root_node.insert(word1)->properties;
}

void mutable_dictionary::insert(const fl::u8str& word1, const fl::u8str& word2) noexcept {
    root_node.insert(word1)->subsequent_words_or_create()->insert(word2);
}

void mutable_dictionary::insert(const fl::u8str& word1, const fl::u8str& word2, const fl::u8str& word3) noexcept {
    root_node.insert(word1)->subsequent_words_or_create()->insert(word2)->subsequent_words_or_create()->insert(word3);
}

void mutable_dictionary::persist() {
    std::ofstream dict_file;
    dict_file.open(dst_path);
    if (dict_file.is_open()) {
        serialize(dict_file);
        dict_file.close();
    }
}

} // namespace fl::nlp
