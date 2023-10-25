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

#include <fmt/core.h>
#include <unicode/locid.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>

export module fl.nlp.core.common:dictionary;

import fl.string;

namespace fl::nlp {

// Atm the schema URL is only used as a long version string, however for the future it enables us to define and support
// different schemas.
export const auto FLDIC_SCHEMA_V0_DRAFT1 = "https://schemas.florisboard.org/nlp/v0~draft1/fldic.txt";
export const auto FLDIC_SCHEMA_LATEST = FLDIC_SCHEMA_V0_DRAFT1;

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

} // namespace dictionary_serialization_helpers

class WrappedJThread {
public:
    std::thread thread;
    WrappedJThread() {
    }
    WrappedJThread(std::thread& other) {
        thread.swap(other);
    }
    WrappedJThread& operator=(WrappedJThread&& other) {
        thread.swap(other.thread);
        return *this;
    }
    WrappedJThread(WrappedJThread&& other) {
        thread.swap(other.thread);
    }
    ~WrappedJThread() {
        thread.join();
    }
}

export class Dictionary {
  public:
    std::filesystem::path file_path;
    std::string schema = FLDIC_SCHEMA_V0_DRAFT1;
    std::string encoding = FLDIC_ENCODING_UTF_8;
    std::vector<WrappedJThread> loading_threads;

    void loadFromDiskInternal(const std::filesystem::path& path) {
        std::ifstream dict_file(path);
        if (dict_file.is_open()) {
            file_path = path;
            deserialize(dict_file);
            dict_file.close();
        } else {
            throw std::runtime_error("Cannot open file '" + file_path.string() + "'");
        }
    }

    void loadFromDisk(const std::filesystem::path& path) {
        std::thread thread(&Dictionary::loadFromDiskInternal, this, path);
        loading_threads.emplace(loading_threads.end(), thread);
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
    static bool isValidSchema(const std::string& schema) noexcept {
        return schema == FLDIC_SCHEMA_V0_DRAFT1;
    }

    static bool isValidEncoding(const std::string& encoding) noexcept {
        return encoding == FLDIC_ENCODING_UTF_8;
    }

    void deserialize(std::istream& istream) {
        schema = FLDIC_SCHEMA_LATEST;
        encoding = FLDIC_ENCODING_UTF_8;

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

            if (line.starts_with(FLDIC_LIST_START)) {
                // We have reached a section, whcih means that's the begin of content
                break;
            }

            if (line.find(FLDIC_ASSIGNMENT_BY_COLON) == std::string::npos) {
                continue;
            }
            fl::str::split(line, FLDIC_ASSIGNMENT_BY_COLON, line_components, 1);
            auto& value = line_components[1];
            fl::str::trim(value);
            if (line.starts_with(FLDIC_GLOBAL_SCHEMA)) {
                if (!isValidSchema(value)) {
                    throw std::runtime_error(fmt::format("Invalid or unsupported schema: '{}'", schema));
                }
                schema.assign(value);
            } else if (line.starts_with(FLDIC_GLOBAL_ENCODING)) {
                if (!isValidEncoding(value)) {
                    throw std::runtime_error(fmt::format("Invalid or unsupported encoding: '{}'", encoding));
                }
                encoding.assign(value);
            }
        }

        istream.seekg(prev_pos);
        deserializeContent(istream);
    }

    void serialize(std::ostream& ostream) {
        ostream << FLDIC_GLOBAL_SCHEMA << " " << schema << FLDIC_NEWLINE << FLDIC_GLOBAL_ENCODING << " " << encoding
                << FLDIC_NEWLINE;
        serializeContent(ostream);
    }
};

} // namespace fl::nlp
