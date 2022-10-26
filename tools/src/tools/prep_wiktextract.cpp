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

#include "tools/prep_wiktextract.hpp"

#include "core/dictionary.hpp"

#include <nlohmann/json.hpp>
#include <unicode/uchar.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

#include <fstream>
#include <iostream>
#include <map>

namespace fl::nlp::tools {

using stats_counter_map = std::map<fl::u8str, uint64_t>;

static const fl::u8str FLAG_INDICATOR = "--";
static const fl::u8str FLAG_SRC_PATH_LONG = "--src";
static const fl::u8str FLAG_DST_PATH_LONG = "--dst";

void insert_project_specific_words(fl::nlp::mutable_dictionary& dict) noexcept {
    dict.insert("FlorisBoard");
    dict.insert("Smartbar");
}

bool validate_word(UText* ut) {
    UChar32 cp;
    while ((cp = utext_next32(ut)) != U_SENTINEL) {
        if (!u_isalpha(cp) && cp != '\'' && cp != '-') return false;
    }
    return true;
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
                auto has_vulgar_tag = std::find_if(tags.begin(), tags.end(),
                                                   [](fl::u8str& tag) { return tag == "vulgar"; }) != tags.end();
                if (has_vulgar_tag) {
                    return true;
                }
            }
        }
    }

    // We assume it is not vulgar in all other cases
    return false;
}

void read_wiktextract_data_into_dictionary(const std::filesystem::path& wiktextract_json_path,
                                           fl::nlp::mutable_dictionary& dict) noexcept {
    UText* ut = nullptr;
    UErrorCode status;

    std::ifstream wiktextract_json_file(wiktextract_json_path, std::ios::in);
    fl::u8str line;
    nlohmann::json json_data;

    // Stats
    uint64_t total_words;
    uint64_t total_senses;
    stats_counter_map pos_stats;
    stats_counter_map tag_stats;
    stats_counter_map category_stats;

    while (std::getline(wiktextract_json_file, line)) {
        json_data = nlohmann::json::parse(line);

        if (json_data.contains("word")) {
            total_words++;
            pos_stats[json_data["pos"].get<fl::u8str>()]++;
            if (json_data.contains("senses")) {
                auto senses = json_data["senses"].get<std::vector<nlohmann::json>>();
                for (auto& sense : senses) {
                    total_senses++;
                    if (sense.contains("tags")) {
                        auto tags = sense["tags"].get<std::vector<fl::u8str>>();
                        for (auto& tag : tags) {
                            tag_stats[tag]++;
                        }
                    }
                    if (sense.contains("categories")) {
                        auto categories = sense["categories"].get<std::vector<nlohmann::json>>();
                        for (auto& category : categories) {
                            category_stats[category["name"].get<fl::u8str>()]++;
                        }
                    }
                }
            }

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

    std::ofstream out("data/stats.json");
    nlohmann::json json_stats;
    json_stats["total_words"] = total_words;
    json_stats["total_senses"] = total_senses;
    json_stats["pos_stats"] = pos_stats;
    json_stats["tag_stats"] = tag_stats;
    json_stats["category_stats"] = category_stats;
    out << std::setw(2) << json_stats;
}

int handle_prep_wiktextract_action(const std::vector<fl::u8str>& flags) noexcept {
    fl::u8str src_path;
    fl::u8str dst_path;

    for (std::size_t i = 0; i < flags.size();) {
        auto& flag = flags[i];
        if (flag == FLAG_SRC_PATH_LONG) {
            if (i + 1 < flags.size() && !flags[i + 1].starts_with(FLAG_INDICATOR)) {
                src_path = flags[i + 1];
                i += 2;
            } else {
                std::cerr << "Fatal: Using source path flag without corresponsing path! Aborting.\n";
                return 1;
            }
        } else if (flag == FLAG_DST_PATH_LONG) {
            if (i + 1 < flags.size() && !flags[i + 1].starts_with(FLAG_INDICATOR)) {
                dst_path = flags[i + 1];
                i += 2;
            } else {
                std::cerr << "Fatal: Using destination path flag without corresponsing path! Aborting.\n";
                return 1;
            }
        } else {
            std::cerr << "Warning: Unknown flag '" << flag << "'. Ignoring.\n";
            i++;
        }
    }

    fl::str::trim(src_path);
    fl::str::trim(dst_path);
    if (src_path.empty()) {
        std::cerr << "Fatal: No source path specified! Aborting.\n";
        return 1;
    } else if (!std::filesystem::exists(src_path)) {
        std::cerr << "Fatal: Given source path '" << src_path << "' does not exist! Aborting.\n";
        return 1;
    }
    if (dst_path.empty()) {
        std::cerr << "Fatal: No destination path specified! Aborting.\n";
        return 1;
    }

    fl::nlp::mutable_dictionary dict;
    dict.dst_path = dst_path;
    insert_project_specific_words(dict);
    read_wiktextract_data_into_dictionary(src_path, dict);
    dict.persist();

    return 0;
}

} // namespace fl::nlp::tools
