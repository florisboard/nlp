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

#include "google/Ngram.hpp"
#include "wortschatz_corpora/preprocessor.hpp"

#include <fstream>
#include <iostream>

using namespace nlp::preprocessing;

const std::filesystem::path TEST_FLDIC_SRC_PATH = "data/test_in.fldic";
const std::filesystem::path TEST_FLDIC_DST_PATH = "data/test_out.fldic";
const std::filesystem::path TEST_WORD_LIST =
    "data/.wortschatz_corpora/eng/eng_news_2020_300K/eng_news_2020_300K-words.txt";

int main(int argc, char** argv) {
    floris::nlp::mutable_dictionary dict;
    dict.dst_path = TEST_FLDIC_DST_PATH;
    fl::nlp::preprocessing::read_corpora_into_dictionary(TEST_WORD_LIST, dict);
    dict.persist();
    return 0;
    // if (argc <= 1) {
    //     std::cout << "Please provide a path"
    //               << "\n";
    //     return 1;
    // }
    // std::string path(argv[1]);
    // try {
    //     auto database = GoogleUnigramDatabase();
    //     database.load(path);
    //     std::string out_path = path + std::string("/db_dump.log");
    //     std::ofstream norm_dump(out_path);
    //     if (norm_dump.is_open()) {
    //         database.dump(norm_dump);
    //         norm_dump.close();
    //     }
    // } catch (std::runtime_error e) {
    //     std::cout << e.what() << "\n";
    //     return 2;
    // }
    // return 0;
}
