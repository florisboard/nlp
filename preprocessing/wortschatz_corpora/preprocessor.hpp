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

#ifndef __FLORISNLP_WORTSCHATZ_CORPORA_PREPROCESSOR_H__
#define __FLORISNLP_WORTSCHATZ_CORPORA_PREPROCESSOR_H__

#include "icuext/string.hpp"
#include "nlp/dictionary.hpp"

#include <unicode/uchar.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

#include <fstream>
#include <string>

namespace fl::nlp::preprocessing {

// The separator of the Wortschatz Corpora word list file
static const char SEPARATOR = '\t';

bool validate_word(UText* ut) {
    UChar32 cp;
    while ((cp = utext_next32(ut)) != U_SENTINEL) {
        if (!u_isalpha(cp)) return false;
    }
    return true;
}

void read_corpora_into_dictionary(
    const std::filesystem::path& word_list_path,
    floris::nlp::mutable_dictionary& dict
) noexcept {
    UText* ut = nullptr;
    UErrorCode status;

    std::ifstream word_list_file;
    word_list_file.open(word_list_path, std::ios::in);
    std::string line;
    std::vector<std::string> columns;

    while (std::getline(word_list_file, line)) {
        icuext::str::trim(line);
        icuext::str::split(line, SEPARATOR, columns);
        if (columns.size() < 3) continue;

        auto& word = columns[1];
        status = U_ZERO_ERROR;
        ut = utext_openUTF8(ut, word.c_str(), -1, &status);
        if (U_FAILURE(status) || !validate_word(ut)) continue;

        auto score = static_cast<floris::nlp::score_t>(std::stoi(columns[2]));
        auto properties = floris::nlp::ngram_properties { score };

        dict.insert(word, properties);
    }

    utext_close(ut);
}

} // namespace fl::nlp::preprocessing

#endif
