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
#include <span>
#include <sstream>
#include <string>
#include <vector>

export module fl.nlp.core.latin.dictionary;

import fl.nlp.string;
import fl.nlp.core.common;

namespace fl::nlp {

export const std::string FLDIC_SECTION_WORDS = "[words]";
export const std::string FLDIC_SECTION_NGRAMS = "[ngrams]";
export const std::string FLDIC_SECTION_SHORTCUTS = "[shortcuts]";

export const char FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE = 'p';
export const char FLDIC_FLAG_IS_HIDDEN_BY_USER = 'h';

export struct WordProperties {
    uint32_t internal_id = 0;
    int32_t absolute_score = 0;
    bool is_possibly_offensive = false;
    bool is_hidden_by_user = false;
};

export struct NgramProperties {
    int32_t absolute_score = 0;
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

  private:
    void deserializeContent(std::istream& istream) override {
        auto section = LatinDictionarySection::UNSPECIFIED;
        std::string line;
        std::vector<std::string> line_components;
        fl::str::UniString word;

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
                if (line_components.size() < 3) {
                    throw std::runtime_error("Invalid line!");
                }
                fl::str::toUniString(line_components[1], word);
                auto node = words.getOrCreate(word);
                // Parse ID
                node->value.internal_id = std::stoi(line_components[0]);
                // Parse scores
                node->value.absolute_score = std::stoi(line_components[2]);
                // Parse flags
                if (line_components.size() > 3) {
                    for (const auto& flag : line_components[3]) {
                        if (flag == FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE) {
                            node->value.is_possibly_offensive = true;
                        } else if (flag == FLDIC_FLAG_IS_HIDDEN_BY_USER) {
                            node->value.is_hidden_by_user = true;
                        }
                    }
                }
            } else if (section == LatinDictionarySection::NGRAMS) {
                // throw std::runtime_error("TODO: implement ngrams");
            } else {
                // throw std::runtime_error("TODO: implement shortcuts");
            }
        }
    }

    void serializeContent(std::ostream& ostream) override {
        std::string word;
        ostream << FLDIC_SECTION_WORDS << FLDIC_NEWLINE;
        words.forEach([&](const std::span<fl::str::UniChar>& uni_word, const WordProperties& properties) {
            fl::str::toStdString(uni_word, word);
            ostream << word << FLDIC_SEPARATOR << properties.absolute_score;
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
        });
    }
};

} // namespace fl::nlp
