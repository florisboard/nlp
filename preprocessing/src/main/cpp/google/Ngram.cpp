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
#include "stdext.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <filesystem>

using namespace nlp::preprocessing::google;

static const char YEAR_DATA_DELIM = '\t';
static const std::string YEAR_DELIM = ",";

const NgramYearlyCounts NgramYearlyCounts::DEFAULT = NgramYearlyCounts();

auto NgramTotalCounts::parse_from_file(const std::string &path) -> NgramTotalCounts {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File '" + path + "' not found!");
    }
    if (std::filesystem::is_directory(path)) {
        throw std::runtime_error("File '" + path + "' is a directory!");
    }

    std::ifstream total_counts_file(path);
    if (total_counts_file.is_open()) {
        auto total_counts = NgramTotalCounts();
        std::string year_data_str;
        std::vector<std::string> year_data_vec;
        while (std::getline(total_counts_file, year_data_str, YEAR_DATA_DELIM)) {
            if (year_data_str.empty()) continue;
            year_data_vec.clear();
            stdext::str_split(year_data_str, YEAR_DELIM, year_data_vec);
            if (year_data_vec.size() != 4) continue;
            NgramYear year = year_data_vec[0];
            NgramYearlyCounts year_data = {
                .matches = std::strtoull(year_data_vec[1].c_str(), nullptr, 10),
                .pages = std::strtoull(year_data_vec[2].c_str(), nullptr, 10),
                .volumes = std::strtoull(year_data_vec[3].c_str(), nullptr, 10),
            };
            total_counts.set_counts_of_year(year, year_data);
        }
        total_counts_file.close();
        return total_counts;
    }

    throw std::runtime_error("An unknown error occurred");
}

auto NgramTotalCounts::get_counts_of_year(const NgramYear year) const noexcept -> NgramYearlyCounts {
    return stdext::map_get_or_default(total_counts_map, year, NgramYearlyCounts::DEFAULT);
}

auto NgramTotalCounts::set_counts_of_year(const NgramYear year, const NgramYearlyCounts counts) noexcept -> void {
    total_counts_map.insert(std::make_pair(year, counts));
}

auto NgramTotalCounts::dump() const noexcept -> std::string {
    std::stringstream ss;
    ss << "NgramTotalCounts {\n";
    for (auto [year, counts] : total_counts_map) {
        ss << "  " << year << " -> { ";
        ss << "matches = " << counts.matches << ", ";
        ss << "pages = " << counts.pages << ", ";
        ss << "volumes = " << counts.volumes << " }\n";
    }
    ss << "}\n";
    return ss.str();
}
