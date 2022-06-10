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

#ifndef _FLORISNLP_PREPROCESSING_GOOGLE_NGRAM
#define _FLORISNLP_PREPROCESSING_GOOGLE_NGRAM

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <filesystem>

namespace nlp::preprocessing {
    static const std::string TOTALCOUNTS_FILE_NAME = "totalcounts-1";

    using NgramYear = std::string;
    using NgramCount = uint64_t;

    struct GoogleNgramYearlyCounts {
        static const GoogleNgramYearlyCounts DEFAULT;

        const NgramCount matches = 0;
        const NgramCount pages = 0;
        const NgramCount volumes = 0;
    };

    class GoogleNgramTotalCounts {
        private:
            using TotalCountsMap = std::map<NgramYear, GoogleNgramYearlyCounts>;
            TotalCountsMap total_counts_map;

        public:
            GoogleNgramTotalCounts() : total_counts_map(TotalCountsMap()) { };
            ~GoogleNgramTotalCounts() = default;

            auto load(const std::filesystem::path &path) -> void;

            auto get_counts_of_year(const NgramYear &year) const noexcept -> GoogleNgramYearlyCounts;

            auto set_counts_of_year(const NgramYear year, const GoogleNgramYearlyCounts counts) noexcept -> void;

            auto dump() const noexcept -> std::string;
    };

    class GoogleNgramDatabase {
        private:
            struct Partition {
                std::map<std::string, double> data;
                double max_weight = 0;
            };

            using Database = std::map<std::string, uint16_t>;
            Database database;
            GoogleNgramTotalCounts total_counts;

            auto load_partition(const std::filesystem::path &path) const -> Partition;

            auto normalize_and_insert_partitions(const std::vector<Partition> &partitions) -> void;

        public:
            GoogleNgramDatabase() : database(Database()), total_counts(GoogleNgramTotalCounts()) { };
            ~GoogleNgramDatabase() = default;

            auto load(const std::filesystem::path &path) -> void;

            auto set_word(std::string word, double weight) noexcept -> void;

            auto dump() const noexcept -> std::string;

            auto dump(std::basic_ostream<char> &out) const noexcept -> void;
    };
}

#endif // _FLORISNLP_PREPROCESSING_GOOGLE_NGRAM
