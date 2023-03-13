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
#include <span>
#include <sstream>
#include <string>
#include <vector>

export module fl.nlp.core.latin:dictionary;

import fl.nlp.string;
import fl.nlp.core.common;

namespace fl::nlp {

export const auto FLDIC_SECTION_WORDS = "[words]";
export const auto FLDIC_SECTION_NGRAMS = "[ngrams]";
export const auto FLDIC_SECTION_SHORTCUTS = "[shortcuts]";

export const char FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE = 'p';
export const char FLDIC_FLAG_IS_HIDDEN_BY_USER = 'h';

export const char FLDIC_TOKEN_START_OF_SENTENCE = 0x02; // Start of text ASCII CTRL char
export const char FLDIC_TOKEN_LIMIT = 0x20;             // Space char

using WordIdT = int32_t;

export struct WordProperties {
    WordIdT internal_id = 0;
    int32_t absolute_score = 0;
    bool is_possibly_offensive = false;
    bool is_hidden_by_user = false;
};

export struct NgramProperties {
    int32_t absolute_score = 0;
    std::unique_ptr<TrieMap<fl::str::UniChar, NgramProperties>> sub_ngrams = nullptr;

    TrieMap<fl::str::UniChar, NgramProperties>* getOrCreateSubNgrams() noexcept {
        if (sub_ngrams == nullptr) {
            sub_ngrams = std::make_unique<TrieMap<fl::str::UniChar, NgramProperties>>();
        }
        return sub_ngrams.get();
    }
};

export struct ShortcutProperties {
    std::string shortcut_phrase;
    int32_t absolute_score;
    bool is_possibly_offensive = false;
    bool is_hidden_by_user = false;
};

export enum class LatinDictionarySection {
    UNSPECIFIED,
    WORDS,
    NGRAMS,
    SHORTCUTS,
};

export class LatinDictionary : public Dictionary {
  public:
    TrieMap<fl::str::UniChar, WordProperties> words;
    TrieMap<fl::str::UniChar, NgramProperties> ngrams;
    TrieMap<fl::str::UniChar, ShortcutProperties> shortcuts;

    int32_t global_penalty_words = 0;
    std::map<int32_t, int32_t> global_penalty_ngrams;
    int32_t global_penalty_shortcuts = 0;

    LatinDictionary() = default;
    ~LatinDictionary() = default;

    TrieNode<fl::str::UniChar, NgramProperties>* insertNgram(std::span<fl::str::UniString> ngram) {
        auto current_map = &ngrams;
        for (int i = 0; i < ngram.size(); i++) {
            auto& word = ngram[i];
            auto word_node = current_map->getOrCreate(word);
            if (i + 1 != ngram.size()) {
                current_map = word_node->value.getOrCreateSubNgrams();
            } else {
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
                auto node = words.getOrCreate(word);
                // Parse score
                node->value.absolute_score = std::stoi(line_components[1]);
                // Parse flags
                if (line_components.size() > 2) {
                    for (const auto& flag : line_components[2]) {
                        if (flag == FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE) {
                            node->value.is_possibly_offensive = true;
                        } else if (flag == FLDIC_FLAG_IS_HIDDEN_BY_USER) {
                            node->value.is_hidden_by_user = true;
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
                    word = id < 0 ? getTokenForSpecialId(id) : id_to_words_map[id];
                    ngram[i] = std::move(word);
                }
                auto ngram_node = insertNgram(ngram);
                ngram_node->value.absolute_score = std::stoi(line_components[1]);
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
        words.forEach([&](const std::span<fl::str::UniChar>& uni_word, WordProperties& properties) {
            fl::str::toStdString(uni_word, word);
            auto score = properties.absolute_score - global_penalty_words;
            ostream << word << FLDIC_SEPARATOR << score;
            if (properties.is_possibly_offensive || properties.is_hidden_by_user) {
                ostream << FLDIC_SEPARATOR;
                if (properties.is_possibly_offensive) {
                    ostream << FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE;
                }
                if (properties.is_hidden_by_user) {
                    ostream << FLDIC_FLAG_IS_HIDDEN_BY_USER;
                }
            }
            ostream << FLDIC_NEWLINE;
            properties.internal_id = current_word_id++;
        });
    }

    void serializeNgrams(std::ostream& ostream) noexcept {
        ostream << FLDIC_NEWLINE << FLDIC_SECTION_NGRAMS << FLDIC_NEWLINE;
        std::vector<WordIdT> ngram;
        serializeNgrams(ostream, ngram, 1, &ngrams);
    }

    void serializeNgrams(
        std::ostream& ostream,
        std::vector<WordIdT>& ngram,
        int32_t current_ngram_level,
        TrieMap<fl::str::UniChar, NgramProperties>* current_map
    ) noexcept {
        current_map->forEach([&ostream, &ngram, &current_ngram_level, this](
                                 auto uni_word, const NgramProperties& properties
                             ) {
            ngram.resize(current_ngram_level);
            ngram[current_ngram_level - 1] = getWordId(uni_word);
            if (current_ngram_level >= 2 && !std::all_of(ngram.begin(), ngram.end(), [](auto id) { return id < 0; })) {
                for (int i = 0; i < ngram.size(); i++) {
                    ostream << ngram[i];
                    if (i + 1 != ngram.size()) {
                        ostream << FLDIC_LIST_SEPARATOR;
                    }
                }
                ostream << FLDIC_SEPARATOR << properties.absolute_score - global_penalty_ngrams[current_ngram_level]
                        << FLDIC_NEWLINE;
            }
            if (properties.sub_ngrams != nullptr) {
                serializeNgrams(ostream, ngram, current_ngram_level + 1, properties.sub_ngrams.get());
            }
        });
    }

    void serializeShortcuts(std::ostream& ostream) noexcept {
        ostream << FLDIC_NEWLINE << FLDIC_SECTION_SHORTCUTS << FLDIC_NEWLINE;
    }

    int32_t getWordId(std::span<const fl::str::UniChar> uni_word) const noexcept {
        if (uni_word[0][0] < FLDIC_TOKEN_LIMIT) {
            return (-1) * static_cast<int32_t>(uni_word[0][0]);
        } else {
            return words.get(uni_word)->value.internal_id;
        }
    }

    fl::str::UniString getTokenForSpecialId(WordIdT id) const noexcept {
        // TODO: CHeck if id is correct
        return {std::string(1, FLDIC_TOKEN_START_OF_SENTENCE)};
    }
};

} // namespace fl::nlp
