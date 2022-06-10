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

namespace nlp::preprocessing::google {
    using NgramYear = std::string;
    using NgramCount = uint64_t;

    struct NgramYearlyCounts {
        static const NgramYearlyCounts DEFAULT;

        const NgramCount matches = 0;
        const NgramCount pages = 0;
        const NgramCount volumes = 0;
    };

    class NgramTotalCounts {
        private:
            using TotalCountsMap = std::map<NgramYear, NgramYearlyCounts>;
            TotalCountsMap total_counts_map;

        public:
            static auto parse_from_file(const std::string &path) -> NgramTotalCounts;

            NgramTotalCounts() : total_counts_map(TotalCountsMap()) { };
            ~NgramTotalCounts() = default;

            auto get_counts_of_year(const NgramYear year) const noexcept -> NgramYearlyCounts;

            auto set_counts_of_year(const NgramYear year, const NgramYearlyCounts counts) noexcept -> void;

            auto dump() const noexcept -> std::string;
    };
}

#endif // _FLORISNLP_PREPROCESSING_GOOGLE_NGRAM
