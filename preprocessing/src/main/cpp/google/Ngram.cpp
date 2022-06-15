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

#include "Ngram.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "stdext/map.hpp"
#include "stdext/string.hpp"

using namespace nlp::preprocessing;

static const char YEAR_DATA_DELIM = '\t';
static const std::string_view YEAR_DELIM { "," };
static const std::string_view DATABASE_DELIM { "\t" };

const GoogleNgramYearlyCounts GoogleNgramYearlyCounts::DEFAULT = GoogleNgramYearlyCounts();

auto GoogleNgramTotalCounts::load(const std::filesystem::path& path) -> void {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File '" + path.string() + "' not found!");
    }
    if (std::filesystem::is_directory(path)) {
        throw std::runtime_error("File '" + path.string() + "' is a directory!");
    }

    std::ifstream total_counts_file(path);
    if (!total_counts_file.is_open()) {
        throw std::runtime_error("An unknown error occurred");
    }
    std::string year_data_str;
    std::vector<std::string_view> year_data_vec;
    while (std::getline(total_counts_file, year_data_str, YEAR_DATA_DELIM)) {
        if (year_data_str.empty()) continue;
        year_data_vec.clear();
        stdext::string::split(year_data_vec, year_data_str, YEAR_DELIM);
        if (year_data_vec.size() != 4) continue;
        auto year = stdext::string::to_number<uint16_t>(year_data_vec[0]);
        GoogleNgramYearlyCounts year_data = {
            .matches = stdext::string::to_number<uint64_t>(year_data_vec[1]),
            .pages = stdext::string::to_number<uint64_t>(year_data_vec[2]),
            .volumes = stdext::string::to_number<uint64_t>(year_data_vec[3]),
        };
        set_counts_of_year(year, year_data);
    }
    total_counts_file.close();
}

auto GoogleNgramTotalCounts::get_counts_of_year(const uint16_t& year) const noexcept -> GoogleNgramYearlyCounts {
    return stdext::map::get_or_default(total_counts_map, year, GoogleNgramYearlyCounts::DEFAULT);
}

auto GoogleNgramTotalCounts::set_counts_of_year(const uint16_t year, const GoogleNgramYearlyCounts counts) noexcept
    -> void {
    total_counts_map.insert(std::make_pair(year, counts));
}

auto GoogleNgramTotalCounts::dump() const noexcept -> std::string {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

auto GoogleNgramTotalCounts::dump(std::basic_ostream<char>& out) const noexcept -> void {
    out << "GoogleNgramTotalCounts {\n";
    for (auto& [year, counts] : total_counts_map) {
        out << year << " -> { ";
        out << "matches = " << counts.matches << ", ";
        out << "pages = " << counts.pages << ", ";
        out << "volumes = " << counts.volumes << " }\n";
    }
    out << "}\n";
}

// -------- GoogleNgramDatabase --------

auto GoogleUnigramDatabase::load(const std::filesystem::path& path) -> void {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Directory '" + path.string() + "' not found!");
    }
    if (!std::filesystem::is_directory(path)) {
        throw std::runtime_error("Directory '" + path.string() + "' is a file!");
    }

    // Load total counts (or throw)
    total_counts.load(path / TOTALCOUNTS_FILE_NAME);

    // Load all partitions
    // TODO: placeholder partition 20 is used; add support for multi-threading and processing of all partitions
    auto time_start = std::chrono::high_resolution_clock::now();
    auto partition = load_partition(path / "1-00019-of-00024");
    auto time_stop = std::chrono::high_resolution_clock::now();
    auto time_duration = std::chrono::duration_cast<std::chrono::seconds>(time_stop - time_start);
    std::cout << "Load partition "
              << "1-00019-of-00024"
              << " took " << time_duration.count() << "s\n";
    std::vector<Partition> partitions { partition };

    // Insert them
    normalize_and_insert_partitions(partitions);
}

auto GoogleUnigramDatabase::load_partition(const std::filesystem::path& partition_path) const -> Partition {
    if (!std::filesystem::exists(partition_path)) {
        throw std::runtime_error("File '" + partition_path.string() + "' not found!");
    }
    if (std::filesystem::is_directory(partition_path)) {
        throw std::runtime_error("File '" + partition_path.string() + "' is a directory!");
    }

    std::ifstream partition_file(partition_path);
    if (!partition_file.is_open()) {
        throw std::runtime_error("An unknown error (partition file) occurred");
    }
    std::ofstream partition_log(get_log_path(partition_path), std::ios::out | std::ios::trunc);
    if (!partition_log.is_open()) {
        throw std::runtime_error("An unknown error (partition log file) occurred");
    }
    auto partition = Partition { .name = partition_path.filename() };
    std::string line_str;
    std::string cleaned_word;
    std::vector<std::string_view> line_vec;
    std::vector<std::string_view> token_vec;
    while (std::getline(partition_file, line_str)) {
        if (line_str.empty()) continue;
        stdext::string::split(line_vec, line_str, DATABASE_DELIM);
        if (line_vec.size() < 1) continue;
        partition.entry_count++;
        std::string_view original_word(line_vec[0]);
        if (!check_and_clean_raw_word(original_word, cleaned_word, partition_log)) {
            partition.skip_count++;
            continue;
        }
        size_t n = 0;
        double weight_sum = 0.0;
        size_t weight_count = 0;
        for (auto& token_str : line_vec) {
            if (n++ == 0 || token_str.empty()) continue;
            stdext::string::split(token_vec, token_str, YEAR_DELIM);
            if (token_vec.size() != 3) continue;
            auto year = stdext::string::to_number<uint16_t>(token_vec[0]);
            auto matches = stdext::string::to_number<uint64_t>(token_vec[1]);
            auto yearlyData = total_counts.get_counts_of_year(year);
            if (yearlyData.matches <= 0) continue;
            weight_sum += (double)matches / (double)yearlyData.matches;
            weight_count++;
        }
        auto unigram = Partition::Unigram {
            .word = cleaned_word,
            .weight = weight_count > 0 ? (weight_sum / static_cast<double>(weight_count)) : 0,
        };
        partition_log << unigram.weight << "\n";
        partition.data.push_back(unigram);
        if (unigram.weight > partition.max_weight) {
            partition.max_weight = unigram.weight;
        }
    }
    partition_file.close();
    return partition;
}

auto GoogleUnigramDatabase::get_log_path(const std::filesystem::path& partition_path) const noexcept
    -> std::filesystem::path {
    auto log_filename = LOG_FILENAME_PREFIX + partition_path.filename().string() + LOG_FILENAME_SUFFIX;
    return partition_path.parent_path() / log_filename;
}

auto GoogleUnigramDatabase::check_and_clean_raw_word(const std::string_view& original,
                                                     std::string& cleaned,
                                                     std::basic_ostream<char>& log) const noexcept -> bool {
    // Check if URL
    if (original.starts_with("https://") || original.starts_with("http://") || original.starts_with("www.")) {
        log << "skip(url)\t" << original << "\n";
        return false;
    }

    // Check if Email
    if (original.find("@") != std::string::npos) {
        log << "skip(email)\t" << original << "\n";
        return false;
    }

    // Check if number (by _NUM tag)
    if (original.ends_with("_NUM")) {
        log << "skip(numtag)\t" << original << "\n";
        return false;
    }

    // Do cleaning operation
    cleaned.assign(original);
    if (!std::regex_match(cleaned, WORD_VALIDATION_REGEX)) {
        log << "skip(invalid)\t" << original << "\n";
        return false;
    }

    // At this point we assume it is a valid word
    log << "take\t" << original << "\t" << cleaned << "\t"; // leave open so outer logic can append weight
    return true;
}

auto GoogleUnigramDatabase::normalize_and_insert_partitions(const std::vector<Partition>& partitions) -> void {
    // Find maximum weight in all partitions
    double max_weight = 0.0;
    for (auto& partition : partitions) {
        if (partition.max_weight > max_weight) {
            max_weight = partition.max_weight;
        }
    }

    // Normalize and insert
    // TODO: reevaluate normed weight calculation and if all 0 are really 0 (maybe spellcheck here??)
    for (auto& partition : partitions) {
        for (auto& [word, weight] : partition.data) {
            uint16_t norm_weight = std::round(UINT16_MAX * (weight / max_weight));
            norm_weight += stdext::map::get_or_default(database, word, (uint16_t)0);
            if (norm_weight > 0) {
                database.insert_or_assign(word, norm_weight);
            }
        }
    }
}

auto GoogleUnigramDatabase::set_word(std::string word, double data) noexcept -> void {
    database.insert(std::make_pair(word, data));
}

auto GoogleUnigramDatabase::dump() const noexcept -> std::string {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

auto GoogleUnigramDatabase::dump(std::basic_ostream<char>& out) const noexcept -> void {
    total_counts.dump(out);
    out << "\nGoogleNgramDatabase {\n";
    for (auto& [word, weight] : database) {
        out << word << " -> " << weight << "\n";
    }
    out << "}\n";
}
