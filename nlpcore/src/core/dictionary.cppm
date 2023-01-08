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
#include <memory>
#include <sstream>
#include <string>
#include <vector>

export module fl.nlp.core.dictionary;

import fl.nlp.string;
import fl.nlp.core.common;
import fl.nlp.core.trie_map;

namespace fl::nlp {

// Atm the schema URL is only used as a long version string, however for the future it enables us to define and support
// different schemas.
static const std::string FLDIC_SCHEMA_V0_DRAFT1 = "https://florisboard.org/schemas/fldic/v0~draft1/dictionary.txt";

static const char FLDIC_ASSIGNMENT = '=';
static const char FLDIC_NEWLINE = '\n';
static const char FLDIC_LIST_SEPARATOR = ',';
static const char FLDIC_SEPARATOR = '\t';

static const std::string FLDIC_HEADER_SCHEMA = "schema";
static const std::string FLDIC_HEADER_NAME = "name";
static const std::string FLDIC_HEADER_LOCALES = "locales";
static const std::string FLDIC_HEADER_GENERATED_BY = "generated_by";

static const std::string FLDIC_SECTION_WORDS = "[words]";
static const std::string FLDIC_SECTION_SHORTCUTS = "[shortcuts]";

static const char FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE = 'p';
static const char FLDIC_FLAG_IS_HIDDEN_BY_USER = 'h';

export class DictionaryHeader {
  public:
    std::string schema = FLDIC_SCHEMA_V0_DRAFT1;
    std::string name;
    std::vector<icu::Locale> locales; // in serialization use BCP 47 tags!
    std::string generated_by;

    size_t writeTo(std::ostream& ostream) const noexcept {
        size_t line_count = 3;
        ostream << FLDIC_HEADER_SCHEMA << FLDIC_ASSIGNMENT << schema << FLDIC_NEWLINE;
        ostream << FLDIC_HEADER_NAME << FLDIC_ASSIGNMENT << name << FLDIC_NEWLINE;
        std::stringstream ss;
        int i = 0;
        for (auto& locale : locales) {
            if (i++ > 0) ss << FLDIC_LIST_SEPARATOR;
            UErrorCode status;
            ss << locale.toLanguageTag<std::string>(status);
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

    void reset() noexcept {
        schema.assign(FLDIC_SCHEMA_V0_DRAFT1);
        name.clear();
        locales.clear();
        generated_by.clear();
    }

    size_t readFrom(std::istream& istream) noexcept {
        size_t line_count = 0;
        std::string line;
        std::string key;
        std::string value;
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
                std::vector<std::string> locale_tags;
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
};

export struct NgramProperties {
    score_t absolute_score = 0;
    bool is_possibly_offensive = false;
    bool is_hidden_by_user = false;
    std::unique_ptr<TrieMap<fl::str::UniChar, NgramProperties>> child_ngrams = nullptr;
};

export struct ShortcutProperties {
    score_t absolute_score;
    bool is_possibly_offensive = false;
    bool is_hidden_by_user = false;
    std::string shortcut_phrase;
};

export enum class LatinDictionarySection {
    UNSPECIFIED,
    WORDS,
    SHORTCUTS,
};

export class LatinDictionary {
  public:
    std::filesystem::path file_path;
    DictionaryHeader header;
    TrieMap<fl::str::UniChar, NgramProperties> words;
    TrieMap<fl::str::UniChar, ShortcutProperties> shortcuts;

    void loadFromDisk(const std::filesystem::path& path) {
        std::ifstream dict_file(path);
        if (dict_file.is_open()) {
            file_path = path;
            deserialize(dict_file);
            dict_file.close();
        } else {
            throw std::runtime_error("Dictionary file could not be opened!");
        }
    }

  private:
    void deserialize(std::istream& istream) {
        header.readFrom(istream);

        auto section = LatinDictionarySection::UNSPECIFIED;
        std::string line;
        std::vector<std::string> line_components;
        fl::str::UniString word;

        while (std::getline(istream, line)) {
            if (line.empty()) continue;
            if (line.starts_with('[')) {
                // Section header
                if (line == FLDIC_SECTION_WORDS) {
                    section = LatinDictionarySection::WORDS;
                } else if (line == FLDIC_SECTION_SHORTCUTS) {
                    section = LatinDictionarySection::SHORTCUTS;
                } else {
                    throw std::runtime_error("Encountered invalid section name!");
                }
                continue;
            }

            if (section == LatinDictionarySection::WORDS) {
                auto word_begin =
                    std::find_if_not(line.begin(), line.end(), [](auto ch) { return ch == FLDIC_SEPARATOR; });
                auto ngram_level = std::distance(line.begin(), word_begin) + 1;
                //
                fl::str::split(line, FLDIC_SEPARATOR, line_components);
                if (line_components.size() < 2) {
                    throw std::runtime_error("Invalid line!");
                }
                fl::str::trim(line_components[0]);
                fl::str::trim(line_components[1]);
                fl::str::toUniString(line_components[0], word);
                auto node = words.getOrCreate(word);
                node->value.absolute_score = std::stoi(line_components[1]);
                if (line_components.size() >= 3) {
                    fl::str::trim(line_components[2]);
                    for (auto f : line_components[2]) {
                        if (f == FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE) {
                            node->value.is_possibly_offensive = true;
                        } else if (f == FLDIC_FLAG_IS_HIDDEN_BY_USER) {
                            node->value.is_hidden_by_user = true;
                        }
                    }
                }
            } else if (section == LatinDictionarySection::SHORTCUTS) {
                throw std::runtime_error("TODO: implement shortcuts");
            } else {
                throw std::runtime_error("Encountered line without previous section tag!");
            }
        }
    }
};

} // namespace fl::nlp
