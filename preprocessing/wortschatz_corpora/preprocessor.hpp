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

#ifndef __FLORISNLP_WORTSCHATZ_CORPORA_PREPROCESSOR_H__
#define __FLORISNLP_WORTSCHATZ_CORPORA_PREPROCESSOR_H__

#include "core/dictionary.hpp"
#include "core/string.hpp"

#include <nlohmann/json.hpp>
#include <unicode/uchar.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

#include <fstream>
#include <string>
#include <vector>

namespace fl::nlp::preprocessing {

// The separator of the Wortschatz Corpora word list file
static const char SEPARATOR = '\t';

bool validate_word(UText* ut) {
    UChar32 cp;
    while ((cp = utext_next32(ut)) != U_SENTINEL) {
        if (!u_isalpha(cp) && cp != '\'' && cp != '-') return false;
    }
    return true;
}

void read_corpora_into_dictionary(
    const std::filesystem::path& word_list_path,
    fl::nlp::mutable_dictionary& dict
) noexcept {
    UText* ut = nullptr;
    UErrorCode status;

    std::ifstream word_list_file;
    word_list_file.open(word_list_path, std::ios::in);
    std::string line;
    std::vector<std::string> columns;

    while (std::getline(word_list_file, line)) {
        fl::str::trim(line);
        fl::str::split(line, SEPARATOR, columns);
        if (columns.size() < 3) continue;

        auto& word = columns[1];
        status = U_ZERO_ERROR;
        ut = utext_openUTF8(ut, word.c_str(), -1, &status);
        if (U_FAILURE(status) || !validate_word(ut)) continue;

        auto score = static_cast<fl::nlp::score_t>(std::stoi(columns[2]));

        dict.insert(word).absolute_score = score;
    }

    utext_close(ut);
}

bool validate_is_word_relevant(const nlohmann::json& word_data) noexcept {
    if (word_data.contains("senses")) {
        auto senses = word_data["senses"].get<std::vector<nlohmann::json>>();
        if (!senses.empty()) {
            for (auto& sense : senses) {
                if (sense.contains("tags")) {
                    auto tags = sense["tags"].get<std::vector<fl::u8str>>();
                    auto has_disallowed_tags = std::find_if(tags.begin(), tags.end(), [](fl::u8str& tag) {
                                                   return tag == "misspelling" || tag == "obsolete";
                                               }) != tags.end();
                    if (!has_disallowed_tags) {
                        // If at least one sense does not have a disallowed tag we see the word as relevant
                        return true;
                    }
                } else {
                    // If a sense has no tags it means it is relevant
                    return true;
                }
            }

            // None of the senses didn't include the disallowed tags
            return false;
        }
    }

    // We assume is relevant in all other cases
    return true;
}

bool check_is_word_vulgar(const nlohmann::json& word_data) noexcept {
    if (word_data.contains("senses")) {
        auto senses = word_data["senses"].get<std::vector<nlohmann::json>>();
        for (auto& sense : senses) {
            if (sense.contains("tags")) {
                auto tags = sense["tags"].get<std::vector<fl::u8str>>();
                auto has_vulgar_tag = std::find_if(tags.begin(), tags.end(), [](fl::u8str& tag) {
                                          return tag == "vulgar";
                                      }) != tags.end();
                if (has_vulgar_tag) {
                    return true;
                }
            }
        }
    }

    // We assume it is not vulgar in all other cases
    return false;
}

void read_wiktextract_data_into_dictionary(
    const std::filesystem::path& wiktextract_json_path,
    fl::nlp::mutable_dictionary& dict
) noexcept {
    UText* ut = nullptr;
    UErrorCode status;

    std::ifstream wiktextract_json_file;
    wiktextract_json_file.open(wiktextract_json_path, std::ios::in);
    std::string line;
    nlohmann::json json_data;

    while (std::getline(wiktextract_json_file, line)) {
        json_data = nlohmann::json::parse(line);

        if (json_data.contains("word")) {
            if (!validate_is_word_relevant(json_data)) continue;
            auto word = json_data["word"].get<fl::u8str>();
            status = U_ZERO_ERROR;
            ut = utext_openUTF8(ut, word.c_str(), -1, &status);
            if (U_FAILURE(status) || !validate_word(ut)) continue;
            auto& properties = dict.insert(word);
            // TODO: reevaluate this as it does not catch plurals of vulgar words and also misses some other offensive
            // words
            if (check_is_word_vulgar(json_data)) {
                properties.is_possibly_offensive = true;
            }
        }
    }

    utext_close(ut);
}

} // namespace fl::nlp::preprocessing

#endif
