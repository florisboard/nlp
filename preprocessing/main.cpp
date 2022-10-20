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
const std::filesystem::path TEST_FLDIC_DST_PATH = "data/wikt_en.fldic";
const std::filesystem::path TEST_WORD_LIST =
    "data/.wortschatz_corpora/eng/eng_wikipedia_2016_300K/eng_wikipedia_2016_300K-words.txt";
const std::filesystem::path TEST_WIKTEXTRACT_FILE = "data/.wiktextract/kaikki.org-dictionary-English.json";

void insert_project_specific_words(fl::nlp::mutable_dictionary& dict) {
    dict.insert("FlorisBoard", fl::nlp::ngram_properties { 1 });
    dict.insert("Smartbar", fl::nlp::ngram_properties { 1 });
}

int main(int argc, char** argv) {
    fl::nlp::mutable_dictionary dict;
    dict.dst_path = TEST_FLDIC_DST_PATH;
    insert_project_specific_words(dict);
    fl::nlp::preprocessing::read_wiktextract_data_into_dictionary(TEST_WIKTEXTRACT_FILE, dict);
    dict.persist();
    return 0;
}
