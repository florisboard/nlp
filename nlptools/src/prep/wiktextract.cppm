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

#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>
#include <unicode/uchar.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <vector>

export module fl.nlp.tools.prep:wiktextract;

import fl.nlp.core.latin;
import fl.nlp.string;
import fl.nlp.tools.common;

namespace fl::nlp::tools {

const std::string ARG_SRC_PATH = "--src";
const std::string ARG_DST_PATH = "--dst";
const std::string ARG_CONFIG_PATH = "--config";
const std::string ARG_FILTER_NAME = "--filter";
const std::string ARG_FILTER_NAME_DEFAULT_VALUE = "root";
const std::string ARG_STATS_PATH = "--stats";
const std::string ARG_STATS_PATH_DEFAULT_VALUE = "";

const uint8_t MERGING_MAX_DEPTH = 0;
const uint8_t MERGING_MAX_DEPTH_WITH_FO = 2;

struct FilterRule {
    std::vector<std::regex> words;
    std::vector<std::string> tags;
    std::vector<std::string> categories;

    [[nodiscard]]
    bool matches(const std::string& _word, const std::vector<std::string>& _tags,
                 const std::vector<std::string>& _categories) const noexcept {
        for (auto& word : words) {
            if (std::regex_match(_word, word)) return true;
        }
        for (auto& tag : tags) {
            for (auto& _tag : _tags) {
                if (_tag == tag) return true;
            }
        }
        for (auto& category : categories) {
            for (auto& _category : _categories) {
                if (_category == category) return true;
            }
        }
        return false;
    }
};

struct Filter {
    std::string name;
    FilterRule excluded;
    FilterRule offensive;
};

static const Filter FALLBACK_FILTER = {"fallback"};

struct WiktextractConfig {
    std::vector<std::string> project_specific_words;
    std::vector<Filter> filters;

    [[nodiscard]]
    const Filter& getFilter(const std::string& filter_name) const noexcept {
        for (auto& filter : filters) {
            if (filter.name == filter_name) {
                return filter;
            }
        }
        for (auto& filter : filters) {
            if (filter.name == ARG_FILTER_NAME_DEFAULT_VALUE) {
                return filter;
            }
        }
        return FALLBACK_FILTER;
    }
};

struct WordEvaluator {
    std::vector<std::string> form_ofs;
    short exclusion_count = 0;
    short offensive_count = 0;
    short normal_count = 0;

    void reset() {
        exclusion_count = 0;
        offensive_count = 0;
        normal_count = 0;
    }

    [[nodiscard]]
    bool isWordExcluded() const noexcept {
        return exclusion_count >= offensive_count && exclusion_count >= normal_count;
    }

    [[nodiscard]]
    bool isWordOffensive() const noexcept {
        return offensive_count >= normal_count;
    }
};

class WiktextractPreprocessor {
  public:
    WiktextractConfig config;
    fl::nlp::LatinDictionary dict;

    WiktextractPreprocessor() = default;
    WiktextractPreprocessor(const WiktextractPreprocessor&) = delete;
    WiktextractPreprocessor(WiktextractPreprocessor&&) = delete;
    ~WiktextractPreprocessor() = default;

    WiktextractPreprocessor& operator=(const WiktextractPreprocessor&) = delete;
    WiktextractPreprocessor& operator=(WiktextractPreprocessor&&) = delete;

  private:
    std::map<std::string, std::map<std::string, WordEvaluator>> parsed_data;

    // Statistics
    using StatsCounterMapT = std::map<std::string, uint64_t>;
    uint64_t total_raw_words = 0;
    uint64_t total_raw_senses = 0;
    uint64_t total_words_excluded = 0;
    uint64_t total_words_offensive = 0;
    uint64_t total_words_normal = 0;
    StatsCounterMapT pos_stats;
    StatsCounterMapT tag_stats;
    StatsCounterMapT category_stats;
    std::chrono::seconds parse_duration_s;

    void insertProjectSpecificWords() noexcept {
        fl::str::UniString uni_word;
        for (auto& word : config.project_specific_words) {
            fl::str::toUniString(word, uni_word);
            auto node = dict.words.getOrCreate(uni_word);
            node->value.absolute_score++;
        }
    }

    bool validateWord(UText* ut) {
        UChar32 cp;
        while ((cp = utext_next32(ut)) != U_SENTINEL) {
            if (!u_isalpha(cp) && cp != '\'' && cp != '-') return false;
        }
        return true;
    }

    void mergeEvaluatorCounts(WordEvaluator& target_evaluator, const WordEvaluator& pos_evaluator,
                              const std::string& pos, short max_depth, short depth = 0) const noexcept {
        target_evaluator.exclusion_count += (depth + 1) * pos_evaluator.exclusion_count;
        target_evaluator.offensive_count += (depth + 1) * pos_evaluator.offensive_count;
        target_evaluator.normal_count += (depth + 1) * pos_evaluator.normal_count;
        if (depth >= max_depth) return;
        for (auto& form_of : pos_evaluator.form_ofs) {
            if (parsed_data.contains(form_of)) {
                auto& pos_map = parsed_data.at(form_of);
                if (pos_map.contains(pos)) {
                    mergeEvaluatorCounts(target_evaluator, pos_map.at(pos), pos, max_depth, depth + 1);
                }
            }
        }
    }

    void readWiktextractJsonFile(const std::filesystem::path& wiktextract_json_path, const std::string& filter_name) {
        std::ifstream wiktextract_json_file(wiktextract_json_path, std::ios::in);
        std::string line;
        std::string word;
        std::string pos;
        std::vector<nlohmann::json> senses;
        std::vector<std::string> tags;
        std::vector<nlohmann::json> categories;
        std::vector<std::string> category_names;
        std::string form_of;
        nlohmann::json json_data;
        Filter filter = config.getFilter(filter_name);

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
                        auto category_name = category["name"].get<std::string>();
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

                if (filter.excluded.matches(word, tags, category_names)) {
                    word_data.exclusion_count++;
                } else if (filter.offensive.matches(word, tags, category_names)) {
                    word_data.offensive_count++;
                } else {
                    word_data.normal_count++;
                }
            }
        }
    }

  public:
    void loadConfig(const std::string& config_path) {
        std::ifstream config_file(config_path);
        nlohmann::json config_json = nlohmann::json::parse(config_file);
        config_json["projectSpecificWords"].get_to(config.project_specific_words);
        auto filter_json_list = config_json["filters"];
        for (auto& filter_json : filter_json_list) {
            Filter filter;
            filter_json["name"].get_to(filter.name);
            filter_json["excluded"]["words"].get_to(filter.excluded.words);
            filter_json["excluded"]["tags"].get_to(filter.excluded.tags);
            filter_json["excluded"]["categories"].get_to(filter.excluded.categories);
            filter_json["offensive"]["words"].get_to(filter.offensive.words);
            filter_json["offensive"]["tags"].get_to(filter.offensive.tags);
            filter_json["offensive"]["categories"].get_to(filter.offensive.categories);
            config.filters.push_back(std::move(filter));
        }
    }

    void readWiktextractDataIntoDictionary(const std::filesystem::path& wiktextract_json_path,
                                           const std::string& filter_name) {
        auto parse_start_time = std::chrono::high_resolution_clock::now();

        readWiktextractJsonFile(wiktextract_json_path, filter_name);

        UText* ut = nullptr;
        UErrorCode status;

        // Insertion into dictionary
        WordEvaluator evaluator;
        WordEvaluator evaluator_with_fo;
        for (auto& [word, pos_map] : parsed_data) {
            evaluator.reset();
            evaluator_with_fo.reset();

            for (auto& [pos, pos_evaluator] : pos_map) {
                mergeEvaluatorCounts(evaluator, pos_evaluator, pos, MERGING_MAX_DEPTH);
                mergeEvaluatorCounts(evaluator_with_fo, pos_evaluator, pos, MERGING_MAX_DEPTH_WITH_FO);
            }

            if (evaluator.isWordExcluded() || evaluator_with_fo.isWordExcluded()) {
                total_words_excluded++;
                continue;
            } else {
                status = U_ZERO_ERROR;
                ut = utext_openUTF8(ut, word.c_str(), word.size(), &status);
                if (U_FAILURE(status) || !validateWord(ut)) {
                    total_words_excluded++;
                    continue;
                } else if (evaluator_with_fo.isWordOffensive()) {
                    total_words_offensive++;
                    fl::str::UniString uni_word;
                    fl::str::toUniString(word, uni_word);
                    auto node = dict.words.getOrCreate(uni_word);
                    node->value.absolute_score += evaluator_with_fo.offensive_count;
                    node->value.is_possibly_offensive = true;
                } else {
                    total_words_normal++;
                    fl::str::UniString uni_word;
                    fl::str::toUniString(word, uni_word);
                    auto node = dict.words.getOrCreate(uni_word);
                    node->value.absolute_score += evaluator_with_fo.normal_count;
                }
            }
        }
        insertProjectSpecificWords();

        utext_close(ut);

        auto parse_end_time = std::chrono::high_resolution_clock::now();
        parse_duration_s = std::chrono::duration_cast<std::chrono::seconds>(parse_end_time - parse_start_time);
    }

    void persistDictionary(const std::string& dst_path) {
        dict.file_path = dst_path;
        dict.persistToDisk();
    }

    void persistStats(const std::string& stats_path) const {
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

int handlePrepWiktextractAction(argparse::ArgumentParser& arg_parser) noexcept {
    std::string src_path = arg_parser.get(ARG_SRC_PATH);
    std::string dst_path = arg_parser.get(ARG_DST_PATH);
    std::string config_path = arg_parser.get(ARG_CONFIG_PATH);
    std::string filter_name = arg_parser.get(ARG_FILTER_NAME);
    std::string stats_path = arg_parser.get(ARG_STATS_PATH);

    fl::str::trim(src_path);
    fl::str::trim(dst_path);
    fl::str::trim(config_path);
    fl::str::trim(filter_name);
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
    if (config_path.empty()) {
        std::cerr << "Fatal: No config path specified! Aborting.\n";
        return 1;
    } else if (!std::filesystem::exists(config_path)) {
        std::cerr << "Fatal: Given config path '" << config_path << "' does not exist! Aborting.\n";
        return 1;
    }
    if (filter_name.empty()) {
        std::cerr << "Fatal: No Filter name specified! Aborting.\n";
        return 1;
    }

    WiktextractPreprocessor preprocessor;
    preprocessor.loadConfig(config_path);
    preprocessor.readWiktextractDataIntoDictionary(src_path, filter_name);
    preprocessor.persistDictionary(dst_path);
    preprocessor.persistStats(stats_path);

    return 0;
}

export class PrepWiktextractActionConfig : public ActionConfig {
  public:
    PrepWiktextractActionConfig() : ActionConfig("prep-wiktextract") {};

    void initArgumentConfig(argparse::ArgumentParser& arg_parser) override {
        arg_parser.add_description("Preprocessing tool which assists in creating FlorisBoard dictionaries "
                                   "(fldic files) using wiktextract json archives from https://kaikki.org/");
        arg_parser.add_argument(ARG_SRC_PATH)
            .required()
            .metavar("PATH")
            .help("The source path pointing to a wiktextract json file");
        arg_parser.add_argument(ARG_DST_PATH)
            .required()
            .metavar("PATH")
            .help("Path of the resulting fldic file; must be writable and must not point to a directory. If a file "
                  "with this name already exists, it will be overwritten");
        arg_parser.add_argument(ARG_CONFIG_PATH)
            .required()
            .metavar("PATH")
            .help("Path of the config file to use for parsing words");
        arg_parser.add_argument(ARG_FILTER_NAME)
            .default_value(ARG_FILTER_NAME_DEFAULT_VALUE)
            .metavar("NAME")
            .help("Specify a specific filter to use from the given config");
        arg_parser.add_argument(ARG_STATS_PATH)
            .default_value(ARG_STATS_PATH_DEFAULT_VALUE)
            .metavar("PATH")
            .help("Path where the resulting statistics from parsing will be written. If empty, no statistics file will "
                  "be written.");
    }

    int runAction(argparse::ArgumentParser& arg_parser) override {
        return handlePrepWiktextractAction(arg_parser);
    }
};

} // namespace fl::nlp::tools
