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

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

namespace fl::nlp::tools {

static const fl::u8str FLAG_INDICATOR = "--";
static const fl::u8str FLAG_SRC_PATH_LONG = "--src";
static const fl::u8str FLAG_DST_PATH_LONG = "--dst";
static const fl::u8str FLAG_STATS_PATH_LONG = "--stats";

static const uint8_t MERGING_MAX_DEPTH = 0;
static const uint8_t MERGING_MAX_DEPTH_WITH_FO = 2;

static const auto EXCLUSION_FILTER = [](const fl::u8str& tc) {
    return tc == "obsolete" || tc == "misspelling" || tc == "morpheme";
};
static const auto OFFENSIVE_FILTER = [](const fl::u8str& tc) {
    return tc == "offensive" || tc == "vulgar" || tc == "English swear words" || tc == "English ethnic slurs" ||
           tc == "Masturbation" || tc == "English religious slurs";
};

struct word_evaluator {
    std::vector<fl::u8str> form_ofs;
    short exclusion_count = 0;
    short offensive_count = 0;
    short normal_count = 0;

    void reset() {
        exclusion_count = 0;
        offensive_count = 0;
        normal_count = 0;
    }

    bool is_word_excluded() const noexcept {
        return exclusion_count >= offensive_count && exclusion_count >= normal_count;
    }

    bool is_word_offensive() const noexcept {
        return offensive_count >= normal_count;
    }
};

class wiktextract_preprocessor {
  public:
    fl::nlp::mutable_dictionary dict;

    wiktextract_preprocessor() = default;
    ~wiktextract_preprocessor() = default;

  private:
    std::map<fl::u8str, std::map<fl::u8str, word_evaluator>> parsed_data;

    // Statistics
    using stats_counter_map = std::map<fl::u8str, uint64_t>;
    uint64_t total_raw_words = 0;
    uint64_t total_raw_senses = 0;
    uint64_t total_words_excluded = 0;
    uint64_t total_words_offensive = 0;
    uint64_t total_words_normal = 0;
    stats_counter_map pos_stats;
    stats_counter_map tag_stats;
    stats_counter_map category_stats;
    std::chrono::seconds parse_duration_s;

    void insert_project_specific_words() noexcept {
        dict.insert("FlorisBoard");
        dict.insert("Smartbar");
    }

    bool validate_word(UText* ut) {
        UChar32 cp;
        std::size_t i;
        while ((cp = utext_next32(ut)) != U_SENTINEL) {
            if (i == 0 && cp == '-') return false;
            if (!u_isalpha(cp) && cp != '\'' && cp != '-') return false;
            i++;
        }
        return true;
    }

    bool check_is_excluded(const std::vector<fl::u8str>& tags) const noexcept {
        return std::find_if(tags.begin(), tags.end(), EXCLUSION_FILTER) != tags.end();
    }

    bool check_is_offensive(const std::vector<fl::u8str>& tags,
                            const std::vector<fl::u8str>& category_names) const noexcept {
        return std::find_if(tags.begin(), tags.end(), OFFENSIVE_FILTER) != tags.end() ||
               std::find_if(category_names.begin(), category_names.end(), OFFENSIVE_FILTER) != category_names.end();
    }

    void merge_evaluator_counts(word_evaluator& target_evaluator, const word_evaluator& pos_evaluator,
                                const fl::u8str& pos, uint8_t max_depth, uint8_t depth = 0) const noexcept {
        target_evaluator.exclusion_count += (depth + 1) * pos_evaluator.exclusion_count;
        target_evaluator.offensive_count += (depth + 1) * pos_evaluator.offensive_count;
        target_evaluator.normal_count += (depth + 1) * pos_evaluator.normal_count;
        if (depth >= max_depth) return;
        for (auto& form_of : pos_evaluator.form_ofs) {
            if (parsed_data.contains(form_of)) {
                auto& pos_map = parsed_data.at(form_of);
                if (pos_map.contains(pos)) {
                    merge_evaluator_counts(target_evaluator, pos_map.at(pos), pos, max_depth, depth + 1);
                }
            }
        }
    }

  public:
    void read_wiktextract_data_into_dictionary(const std::filesystem::path& wiktextract_json_path) {
        UText* ut = nullptr;
        UErrorCode status;

        std::ifstream wiktextract_json_file(wiktextract_json_path, std::ios::in);
        fl::u8str line;
        fl::u8str word;
        fl::u8str pos;
        std::vector<nlohmann::json> senses;
        std::vector<fl::u8str> tags;
        std::vector<nlohmann::json> categories;
        std::vector<fl::u8str> category_names;
        fl::u8str form_of;
        nlohmann::json json_data;

        auto parse_start_time = std::chrono::high_resolution_clock::now();

        while (std::getline(wiktextract_json_file, line)) {
            json_data = nlohmann::json::parse(line);
            if (!json_data.contains("word") || !json_data.contains("pos") || !json_data.contains("senses")) continue;

            json_data["word"].get_to(word);
            json_data["pos"].get_to(pos);
            json_data["senses"].get_to(senses);
            auto& word_data = parsed_data[word][pos];

            total_raw_words++;
            pos_stats[pos]++;

            for (auto& sense : senses) {
                total_raw_senses++;
                if (sense.contains("tags")) {
                    sense["tags"].get_to(tags);
                    for (auto& tag : tags) {
                        tag_stats[tag]++;
                    }
                } else {
                    tags.clear();
                }
                category_names.clear();
                if (sense.contains("categories")) {
                    sense["categories"].get_to(categories);
                    for (auto& category : categories) {
                        auto category_name = category["name"].get<fl::u8str>();
                        category_stats[category_name]++;
                        category_names.push_back(std::move(category_name));
                    }
                } else {
                    categories.clear();
                }
                if (sense.contains("form_of")) {
                    sense["form_of"][0]["word"].get_to(form_of);
                    word_data.form_ofs.push_back(form_of);
                } else if (sense.contains("alt_of")) {
                    sense["alt_of"][0]["word"].get_to(form_of);
                    word_data.form_ofs.push_back(form_of);
                }

                if (check_is_excluded(tags)) {
                    word_data.exclusion_count++;
                } else if (check_is_offensive(tags, category_names)) {
                    word_data.offensive_count++;
                } else {
                    word_data.normal_count++;
                }
            }
        }

        // Insertion into dictionary
        word_evaluator evaluator;
        word_evaluator evaluator_with_fo;
        for (auto it = parsed_data.cbegin(); it != parsed_data.cend(); it++) {
            auto& [word, pos_map] = *it;
            evaluator.reset();
            evaluator_with_fo.reset();

            for (auto& [pos2, pos_evaluator] : pos_map) {
                merge_evaluator_counts(evaluator, pos_evaluator, pos2, MERGING_MAX_DEPTH);
                merge_evaluator_counts(evaluator_with_fo, pos_evaluator, pos2, MERGING_MAX_DEPTH_WITH_FO);
            }

            if (evaluator.is_word_excluded() || evaluator_with_fo.is_word_excluded()) {
                total_words_excluded++;
                continue;
            } else {
                status = U_ZERO_ERROR;
                ut = utext_openUTF8(ut, word.c_str(), word.size(), &status);
                if (U_FAILURE(status) || !validate_word(ut)) {
                    total_words_excluded++;
                    continue;
                } else if (evaluator_with_fo.is_word_offensive()) {
                    total_words_offensive++;
                    auto& properties = dict.insert(word);
                    properties.absolute_score += evaluator_with_fo.offensive_count;
                    properties.is_possibly_offensive = true;
                } else {
                    total_words_normal++;
                    auto& properties = dict.insert(word);
                    properties.absolute_score += evaluator_with_fo.normal_count;
                }
            }
        }

        utext_close(ut);

        auto parse_end_time = std::chrono::high_resolution_clock::now();
        parse_duration_s = std::chrono::duration_cast<std::chrono::seconds>(parse_end_time - parse_start_time);
    }

    void persist_dictionary(const fl::u8str& dst_path) {
        dict.dst_path = dst_path;
        dict.persist();
    }

    void persist_stats(const fl::u8str& stats_path) const {
        if (stats_path.empty()) return;
        std::ofstream out(stats_path);
        nlohmann::json json_stats;
        json_stats["_parse_duration_in_seconds"] = parse_duration_s.count();
        json_stats["_total_raw_words"] = total_raw_words;
        json_stats["_total_raw_senses"] = total_raw_senses;
        json_stats["_total_words_excluded"] = total_words_excluded;
        json_stats["_total_words_offensive"] = total_words_offensive;
        json_stats["_total_words_normal"] = total_words_normal;
        json_stats["pos_stats"] = pos_stats;
        json_stats["tag_stats"] = tag_stats;
        json_stats["category_stats"] = category_stats;
        out << std::setw(2) << json_stats;
    }
};

int handle_prep_wiktextract_action(const std::vector<fl::u8str>& flags) noexcept {
    fl::u8str src_path;
    fl::u8str dst_path;
    fl::u8str stats_path;

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
        } else if (flag == FLAG_STATS_PATH_LONG) {
            if (i + 1 < flags.size() && !flags[i + 1].starts_with(FLAG_INDICATOR)) {
                stats_path = flags[i + 1];
                i += 2;
            } else {
                std::cerr << "Fatal: Using statistics path flag without corresponsing path! Aborting.\n";
                return 1;
            }
        } else {
            std::cerr << "Warning: Unknown flag '" << flag << "'. Ignoring.\n";
            i++;
        }
    }

    fl::str::trim(src_path);
    fl::str::trim(dst_path);
    fl::str::trim(stats_path);
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

    wiktextract_preprocessor preprocessor;
    preprocessor.read_wiktextract_data_into_dictionary(src_path);
    preprocessor.persist_dictionary(dst_path);
    preprocessor.persist_stats(stats_path);

    return 0;
}

} // namespace fl::nlp::tools
