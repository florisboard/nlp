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

#include <iostream>
#include "google/Ngram.hpp"

using namespace nlp::preprocessing;
using namespace nlp::preprocessing::google;

static const std::string TOTALCOUNTS_FILE_NAME = "totalcounts-1";

int main(int argc, char **argv) {
    if (argc <= 1) {
        std::cout << "Please provide a path" << "\n";
        return 1;
    }
    std::string path(argv[1]);
    path.append("/");
    path.append(TOTALCOUNTS_FILE_NAME);
    try {
        auto total_counts = NgramTotalCounts::parse_from_file(path);
        std::cout << total_counts.dump();
    } catch (std::runtime_error e) {
        std::cout << e.what() << "\n";
        return 2;
    }
    return 0;
}
