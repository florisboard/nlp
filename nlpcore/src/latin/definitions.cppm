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

#include <cstdint>
#include <vector>

export module fl.nlp.core.latin:definitions;

import :entry_properties;
import fl.nlp.string;
import fl.nlp.core.common;

namespace fl::nlp {

export using LatinDictId = std::int8_t;
export using LatinTrieNode = TrieNode<fl::str::UniChar, EntryProperties, LatinDictId>;

export const char LATIN_TOKEN_ID_START_OF_SENTENCE = 0x02; // Start of text ASCII CTRL char
export const char LATIN_TOKEN_ID_NGRAM_SEPARATOR = 0x1E;   // Record separator ASCII CTRL char
export const char LATIN_TOKEN_ID_LIMIT = 0x20;             // Space char
export const fl::str::UniChar LATIN_TOKEN_START_OF_SENTENCE = {1, LATIN_TOKEN_ID_START_OF_SENTENCE};
export const fl::str::UniChar LATIN_TOKEN_NGRAM_SEPARATOR = {1, LATIN_TOKEN_ID_NGRAM_SEPARATOR};
export const std::vector<fl::str::UniChar> LATIN_SEARCH_TERMINATION_TOKENS = {LATIN_TOKEN_NGRAM_SEPARATOR};

export inline bool isSpecialToken(const fl::str::UniChar& token) noexcept {
    return token.size() == 1 && token[0] < LATIN_TOKEN_ID_LIMIT;
}

export inline bool isSpecialToken(std::span<const fl::str::UniChar> token) noexcept {
    return token.size() == 1 && isSpecialToken(token[0]);
}

export inline bool isSpecialId(int32_t id) noexcept {
    return id < 0;
}

export inline int32_t convertSpecialTokenToId(std::span<const fl::str::UniChar> token) noexcept {
    return (-1) * static_cast<int32_t>(token[0][0]);
}

export inline fl::str::UniString convertSpecialIdToToken(int32_t id) noexcept {
    return {std::string(1, static_cast<char>((-1) * id))};
}

} // namespace fl::nlp
