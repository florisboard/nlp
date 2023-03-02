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
#include <sstream>
#include <string>
#include <vector>

export module fl.nlp.core.common:dictionary;

import fl.nlp.string;

namespace fl::nlp {

// Atm the schema URL is only used as a long version string, however for the future it enables us to define and support
// different schemas.
export const auto FLDIC_SCHEMA_V0_DRAFT1 = "https://florisboard.org/schemas/fldic/v0~draft1/dictionary.txt";

// The only allowed value for encoding
export const auto FLDIC_ENCODING_UTF_8 = "utf-8";

export const char FLDIC_ASSIGNMENT = '=';
export const char FLDIC_ASSIGNMENT_BY_COLON = ':';
export const char FLDIC_LINE_COMMENT = '#';
export const char FLDIC_NEWLINE = '\n';
export const char FLDIC_SEPARATOR = '\t';
export const char FLDIC_STRING_START = '\"';
export const char FLDIC_STRING_END = '\"';
export const char FLDIC_LIST_START = '[';
export const char FLDIC_LIST_END = ']';
export const char FLDIC_LIST_SEPARATOR = ',';

export const auto FLDIC_GLOBAL_SCHEMA = "#~schema:";
export const auto FLDIC_GLOBAL_ENCODING = "#~encoding:";

export const auto FLDIC_META_NAME = "name";
export const auto FLDIC_META_DISPLAY_NAME = "display_name";
export const auto FLDIC_META_LOCALES = "locales";
export const auto FLDIC_META_GENERATED_BY = "generated_by";
export const auto FLDIC_META_AUTHORS = "authors";
export const auto FLDIC_META_LICENSE = "license";

export const auto FLDIC_SECTION_META = "[meta]";

namespace dictionary_serialization_helpers {

export std::string decodeString(const auto& raw_str) noexcept {
    if (raw_str.starts_with(FLDIC_STRING_START) && raw_str.ends_with(FLDIC_STRING_END)) {
        return raw_str.substr(1, raw_str.length() - 2);
    } else {
        return raw_str;
    }
}

export std::vector<std::string> decodeList(const auto& raw_list) noexcept {
    if (raw_list.starts_with(FLDIC_LIST_START) && raw_list.ends_with(FLDIC_LIST_END)) {
        std::vector<std::string> list;
        fl::str::split(raw_list.substr(1, raw_list.length() - 2), FLDIC_LIST_SEPARATOR, list);
        for (auto& str : list) {
            str = decodeString(str);
        }
        return list;
    } else {
        return {};
    }
}

export std::string encodeString(const auto& str) noexcept {
    return FLDIC_STRING_START + str + FLDIC_STRING_END;
}

export std::string encodeList(const std::vector<std::string>& list) noexcept {
    std::stringstream dst;
    dst << FLDIC_LIST_START;
    int i = 0;
    for (const auto& str : list) {
        dst << encodeString(str);
        if (i++ > 0) {
            dst << FLDIC_LIST_SEPARATOR;
        }
    }
    dst << FLDIC_LIST_END;
    return dst.str();
}

}

export class DictionaryMeta {
  public:
    std::string name;
    std::string display_name;
    std::vector<icu::Locale> locales; // in serialization use BCP 47 tags!
    std::string generated_by;
    std::vector<std::string> authors;
    std::string license;

    void readLine(const auto& line) noexcept {
        using namespace dictionary_serialization_helpers;

        if (line.find(FLDIC_ASSIGNMENT) == std::string::npos) {
            return;
        }

        std::vector<std::string> pair;
        fl::str::split(line, FLDIC_ASSIGNMENT, pair);
        auto& key = pair[0];
        auto& value = pair[1];
        fl::str::trim(key);
        fl::str::trim(value);
        if (key == FLDIC_META_NAME) {
            name = decodeString(value);
        } else if (key == FLDIC_META_DISPLAY_NAME) {
            display_name = decodeString(value);
        } else if (key == FLDIC_META_LOCALES) {
            locales.clear();
            auto locale_tags = decodeList(value);
            for (const auto& locale_tag : locale_tags) {
                UErrorCode status = U_ZERO_ERROR;
                locales.push_back(icu::Locale::forLanguageTag(locale_tag, status));
            }
        } else if (key == FLDIC_META_GENERATED_BY) {
            generated_by = decodeString(value);
        } else if (key == FLDIC_META_AUTHORS) {
            authors = decodeList(value);
        } else if (key == FLDIC_META_LICENSE) {
            license = decodeString(value);
        }
    }

    void writeAllTo(std::ostream& ostream) const noexcept {
        using namespace dictionary_serialization_helpers;

        std::vector<std::string> locale_tags;
        for (auto& locale : locales) {
            UErrorCode status;
            locale_tags.push_back(locale.toLanguageTag<std::string>(status));
        }

        ostream << FLDIC_META_NAME << FLDIC_ASSIGNMENT << encodeString(name) << FLDIC_NEWLINE;
        ostream << FLDIC_META_DISPLAY_NAME << FLDIC_ASSIGNMENT << encodeString(display_name) << FLDIC_NEWLINE;
        ostream << FLDIC_META_LOCALES << FLDIC_ASSIGNMENT << encodeList(locale_tags) << FLDIC_NEWLINE;
        ostream << FLDIC_META_GENERATED_BY << FLDIC_ASSIGNMENT << encodeString(generated_by) << FLDIC_NEWLINE;
        ostream << FLDIC_META_AUTHORS << FLDIC_ASSIGNMENT << encodeList(authors) << FLDIC_NEWLINE;
        ostream << FLDIC_META_LICENSE << FLDIC_ASSIGNMENT << encodeString(license) << FLDIC_NEWLINE;
    }

    void reset() noexcept {
        name.clear();
        display_name.clear();
        locales.clear();
        generated_by.clear();
        authors.clear();
        license.clear();
    }
};

enum class DictionarySection {
    GLOBAL,
    META,
};

export class Dictionary {
  public:
    std::filesystem::path file_path;
    std::string schema = FLDIC_SCHEMA_V0_DRAFT1;
    std::string encoding = FLDIC_ENCODING_UTF_8;
    DictionaryMeta meta;

    void loadFromDisk(const std::filesystem::path& path) {
        std::ifstream dict_file(path);
        if (dict_file.is_open()) {
            file_path = path;
            deserialize(dict_file);
            dict_file.close();
        } else {
            throw std::runtime_error("Cannot open file '" + file_path.string() + "'");
        }
    }

    void persistToDisk() {
        std::ofstream dict_file(file_path);
        if (dict_file.is_open()) {
            serialize(dict_file);
            dict_file.close();
        } else {
            throw std::runtime_error("Cannot open file '" + file_path.string() + "'");
        }
    }

  protected:
    virtual void deserializeContent(std::istream& istream) = 0;
    virtual void serializeContent(std::ostream& ostream) = 0;

  private:
    void deserialize(std::istream& istream) {
        auto section = DictionarySection::GLOBAL;
        auto prev_pos = istream.tellg();
        std::string line;
        std::vector<std::string> line_components;

        while (true) {
            prev_pos = istream.tellg();
            if (!std::getline(istream, line)) {
                break;
            }

            fl::str::trim(line);
            if (line.empty()) continue;

            if (line.starts_with(FLDIC_SECTION_META[0])) {
                if (line == FLDIC_SECTION_META) {
                    section = DictionarySection::META;
                } else {
                    break;
                }
            }

            if (section == DictionarySection::GLOBAL) {
                if (line.find(FLDIC_ASSIGNMENT_BY_COLON) == std::string::npos) {
                    continue;
                }
                fl::str::split(line, FLDIC_ASSIGNMENT_BY_COLON, line_components, 1);
                auto& value = line_components[1];
                fl::str::trim(value);
                if (line.starts_with(FLDIC_GLOBAL_SCHEMA)) {
                    schema.assign(value);
                } else if (line.starts_with(FLDIC_GLOBAL_ENCODING)) {
                    encoding.assign(value);
                }
            } else if (section == DictionarySection::META) {
                if (line.starts_with(FLDIC_LINE_COMMENT)) {
                    continue;
                }
                meta.readLine(line);
            }
        }

        istream.seekg(prev_pos);
        deserializeContent(istream);
    }

    void serialize(std::ostream& ostream) {
        ostream << FLDIC_GLOBAL_SCHEMA << " " << schema << FLDIC_NEWLINE
                << FLDIC_GLOBAL_ENCODING << " " << encoding << FLDIC_NEWLINE
                << FLDIC_NEWLINE
                << FLDIC_SECTION_META << FLDIC_NEWLINE;
        meta.writeAllTo(ostream);
        serializeContent(ostream);
    }
};

} // namespace fl::nlp
