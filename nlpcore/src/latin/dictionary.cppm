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

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <vector>

export module fl.nlp.core.latin:dictionary;

import :entry_properties;
import fl.nlp.string;
import fl.nlp.core.common;

namespace fl::nlp {

export const auto FLDIC_SECTION_WORDS = "[words]";
export const auto FLDIC_SECTION_NGRAMS = "[ngrams]";
export const auto FLDIC_SECTION_SHORTCUTS = "[shortcuts]";

export const char FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE = 'p';
export const char FLDIC_FLAG_IS_HIDDEN_BY_USER = 'h';

export const char FLDIC_TOKEN_ID_START_OF_SENTENCE = 0x02; // Start of text ASCII CTRL char
export const char FLDIC_TOKEN_ID_NGRAM_SEPARATOR = 0x1E;   // Record separator ASCII CTRL char
export const char FLDIC_TOKEN_ID_LIMIT = 0x20;             // Space char
export const fl::str::UniChar FLDIC_TOKEN_START_OF_SENTENCE = {1, FLDIC_TOKEN_ID_START_OF_SENTENCE};
export const fl::str::UniChar FLDIC_TOKEN_NGRAM_SEPARATOR = {1, FLDIC_TOKEN_ID_NGRAM_SEPARATOR};
export const std::vector<fl::str::UniChar> FLDIC_SEARCH_TERMINATION_TOKENS = {FLDIC_TOKEN_NGRAM_SEPARATOR};

export enum class LatinDictionarySection {
    UNSPECIFIED,
    WORDS,
    NGRAMS,
    SHORTCUTS,
};

export class LatinDictionary : public Dictionary {
  private:
    using DictIdT = std::uint8_t;
    using SharedNodeT = TrieNode<fl::str::UniChar, EntryProperties>;

  public:
    std::size_t dict_id;
    std::shared_ptr<SharedNodeT> data;

    int32_t global_penalty_words = 0;
    std::map<int32_t, int32_t> global_penalty_ngrams;
    int32_t global_penalty_shortcuts = 0;

    LatinDictionary() = delete;
    LatinDictionary(DictIdT id, std::shared_ptr<SharedNodeT> shared_data) : dict_id(id), data(shared_data) {};
    LatinDictionary(DictIdT id) : dict_id(id), data(std::make_shared<SharedNodeT>()) {};
    LatinDictionary(const LatinDictionary&) = delete;
    LatinDictionary(LatinDictionary&&) = default;
    virtual ~LatinDictionary() = default;

    LatinDictionary& operator=(const LatinDictionary&) = delete;
    LatinDictionary& operator=(LatinDictionary&&) = default;

    TrieNode<fl::str::UniChar, EntryProperties>* insertNgram(std::span<const fl::str::UniString> ngram) {
        auto ngram_node = data.get();
        for (int i = 0; i < ngram.size(); i++) {
            auto& word = ngram[i];
            auto word_node = ngram_node->findOrCreate(word);
            auto word_value = word_node->valueOrCreate(dict_id);
            if (i + 1 != ngram.size()) {
                ngram_node = word_node->findOrCreate(FLDIC_TOKEN_NGRAM_SEPARATOR);
            } else {
                word_value->ngramPropertiesOrCreate();
                return word_node;
            }
        }
        return nullptr;
    }

  private:
    void deserializeContent(std::istream& istream) override {
        auto section = LatinDictionarySection::UNSPECIFIED;
        std::string line;
        std::vector<std::string> line_components;
        fl::str::UniString word;
        std::vector<std::string> id_components;
        std::vector<fl::str::UniString> ngram;
        std::map<WordIdT, fl::str::UniString> id_to_words_map;
        WordIdT current_word_id = 1;

        while (std::getline(istream, line)) {
            fl::str::trim(line);
            if (line.empty() || line.starts_with(FLDIC_LINE_COMMENT)) continue;

            if (line.starts_with('[')) {
                // Section header
                if (line == FLDIC_SECTION_WORDS) {
                    section = LatinDictionarySection::WORDS;
                } else if (line == FLDIC_SECTION_NGRAMS) {
                    section = LatinDictionarySection::NGRAMS;
                } else if (line == FLDIC_SECTION_SHORTCUTS) {
                    section = LatinDictionarySection::SHORTCUTS;
                } else {
                    throw std::runtime_error("Encountered invalid section name!");
                }
                continue;
            }

            if (section == LatinDictionarySection::WORDS) {
                fl::str::split(line, FLDIC_SEPARATOR, line_components);
                if (line_components.size() < 2) {
                    throw std::runtime_error("Invalid line!");
                }
                fl::str::toUniString(line_components[0], word);
                auto node = data->findOrCreate(word);
                auto properties = node->valueOrCreate(dict_id)->wordPropertiesOrCreate();
                // Parse score
                properties->absolute_score = std::stoi(line_components[1]);
                // Parse flags
                if (line_components.size() > 2) {
                    for (const auto& flag : line_components[2]) {
                        if (flag == FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE) {
                            properties->is_possibly_offensive = true;
                        } else if (flag == FLDIC_FLAG_IS_HIDDEN_BY_USER) {
                            properties->is_hidden_by_user = true;
                        }
                    }
                }
                // Assign word to word ID map
                id_to_words_map[current_word_id++] = std::move(word);
            } else if (section == LatinDictionarySection::NGRAMS) {
                fl::str::split(line, FLDIC_SEPARATOR, line_components);
                if (line_components.size() < 2) {
                    throw std::runtime_error("Invalid line!");
                }
                fl::str::split(line_components[0], FLDIC_LIST_SEPARATOR, id_components);
                ngram.resize(id_components.size());
                for (int i = 0; i < id_components.size(); i++) {
                    auto id = std::stoi(id_components[i]);
                    word = isSpecialId(id) ? convertSpecialIdToToken(id) : id_to_words_map[id];
                    ngram[i] = std::move(word);
                }
                auto node = insertNgram(ngram);
                auto properties = node->value(dict_id)->ngramProperties();
                properties->absolute_score = std::stoi(line_components[1]);
            } else if (section == LatinDictionarySection::SHORTCUTS) {
                // throw std::runtime_error("TODO: implement shortcuts");
            }
        }
    }

    void serializeContent(std::ostream& ostream) override {
        serializeWords(ostream);
        serializeNgrams(ostream);
        serializeShortcuts(ostream);
    }

    void serializeWords(std::ostream& ostream) noexcept {
        ostream << FLDIC_NEWLINE << FLDIC_SECTION_WORDS << FLDIC_NEWLINE;
        WordIdT current_word_id = 1;
        std::string word;
        data->forEach(
            FLDIC_SEARCH_TERMINATION_TOKENS,
            [&](std::span<const fl::str::UniChar> uni_word, TrieNode<fl::str::UniChar, EntryProperties>* node) {
                auto value = node->valueOrNull(dict_id);
                if (value == nullptr) {
                    return;
                }
                auto word_properties = value->wordPropertiesOrNull();
                if (word_properties == nullptr) {
                    return;
                }
                fl::str::toStdString(uni_word, word);
                auto score = word_properties->absolute_score - global_penalty_words;
                ostream << word << FLDIC_SEPARATOR << score;
                if (word_properties->is_possibly_offensive || word_properties->is_hidden_by_user) {
                    ostream << FLDIC_SEPARATOR;
                    if (word_properties->is_possibly_offensive) {
                        ostream << FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE;
                    }
                    if (word_properties->is_hidden_by_user) {
                        ostream << FLDIC_FLAG_IS_HIDDEN_BY_USER;
                    }
                }
                ostream << FLDIC_NEWLINE;
                word_properties->internal_id = current_word_id++;
            }
        );
    }

    void serializeNgrams(std::ostream& ostream) noexcept {
        ostream << FLDIC_NEWLINE << FLDIC_SECTION_NGRAMS << FLDIC_NEWLINE;
        std::vector<WordIdT> ngram;
        serializeNgrams(ostream, ngram, 1, data.get());
    }

    void serializeNgrams(
        std::ostream& ostream,
        std::vector<WordIdT>& ngram,
        int32_t current_ngram_level,
        TrieNode<fl::str::UniChar, EntryProperties>* current_root_node
    ) noexcept {
        current_root_node->forEach(
            FLDIC_SEARCH_TERMINATION_TOKENS,
            [&ostream, &ngram, &current_ngram_level,
             this](auto uni_word, TrieNode<fl::str::UniChar, EntryProperties>* node) {
                ngram.resize(current_ngram_level);
                ngram[current_ngram_level - 1] = getWordId(uni_word);
                auto value = node->valueOrNull(dict_id);
                if (value != nullptr) {
                    auto ngram_properties = value->ngramPropertiesOrNull();
                    if (ngram_properties != nullptr && current_ngram_level >= 2) {
                        if (!std::all_of(ngram.begin(), ngram.end(), [](auto id) { return id < 0; })) {
                            for (int i = 0; i < ngram.size(); i++) {
                                ostream << ngram[i];
                                if (i + 1 != ngram.size()) {
                                    ostream << FLDIC_LIST_SEPARATOR;
                                }
                            }
                            ostream << FLDIC_SEPARATOR
                                    << ngram_properties->absolute_score - global_penalty_ngrams[current_ngram_level]
                                    << FLDIC_NEWLINE;
                        }
                    }
                }

                auto next_ngram_root_node = node->findOrNull(FLDIC_TOKEN_NGRAM_SEPARATOR);
                if (next_ngram_root_node != nullptr) {
                    serializeNgrams(ostream, ngram, current_ngram_level + 1, next_ngram_root_node);
                }
            }
        );
    }

    void serializeShortcuts(std::ostream& ostream) noexcept {
        ostream << FLDIC_NEWLINE << FLDIC_SECTION_SHORTCUTS << FLDIC_NEWLINE;
    }

  public:
    [[nodiscard]]
    int32_t getWordId(std::span<const fl::str::UniChar> uni_word) const {
        if (isSpecialToken(uni_word)) {
            return convertSpecialTokenToId(uni_word);
        } else {
            auto value = data->find(uni_word)->valueOrNull(dict_id);
            if (value == nullptr) return -1;
            auto properties = value->wordPropertiesOrNull();
            return properties == nullptr ? -1 : properties->internal_id;
        }
    }

    static inline bool isSpecialToken(const fl::str::UniChar& token) noexcept {
        return token.size() == 1 && token[0] < FLDIC_TOKEN_ID_LIMIT;
    }

    static inline bool isSpecialToken(std::span<const fl::str::UniChar> token) noexcept {
        return token.size() == 1 && isSpecialToken(token[0]);
    }

    static inline bool isSpecialId(int32_t id) noexcept {
        return id < 0;
    }

    static inline int32_t convertSpecialTokenToId(std::span<const fl::str::UniChar> token) noexcept {
        return (-1) * static_cast<int32_t>(token[0][0]);
    }

    static inline fl::str::UniString convertSpecialIdToToken(int32_t id) noexcept {
        return {std::string(1, static_cast<char>((-1) * id))};
    }
};

} // namespace fl::nlp
