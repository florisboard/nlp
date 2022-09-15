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

#ifndef _FLORISNLP_CORE_NGRAM_SPELLING_RESULT
#define _FLORISNLP_CORE_NGRAM_SPELLING_RESULT

#include <memory>
#include <vector>
#include "ngram.hpp"

namespace floris::nlp {

static const int RESULT_UNSPECIFIED = 0x0000;
static const int RESULT_ATTR_IN_THE_DICTIONARY = 0x0001;
static const int RESULT_ATTR_LOOKS_LIKE_TYPO = 0x0002;
static const int RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS = 0x0004;
static const int RESULT_ATTR_LOOKS_LIKE_GRAMMAR_ERROR = 0x0008;
static const int RESULT_ATTR_DONT_SHOW_UI_FOR_SUGGESTIONS = 0x0010;

class SpellingResult {
  public:
    const int suggestion_attributes;
    const std::shared_ptr<const std::vector<ustring_t>> suggestions;

  public:
    SpellingResult(const int suggestion_attributes, const std::shared_ptr<const std::vector<ustring_t>> suggestions)
        : suggestion_attributes(suggestion_attributes), suggestions(suggestions) {}
    ~SpellingResult() = default;

    static SpellingResult unspecified() noexcept {
        return SpellingResult(RESULT_UNSPECIFIED, nullptr);
    }

    static SpellingResult valid_word() noexcept {
        return SpellingResult(RESULT_ATTR_IN_THE_DICTIONARY, nullptr);
    }

    static SpellingResult typo(const std::shared_ptr<const std::vector<ustring_t>> suggestions,
                               bool is_high_confidence_result = false) noexcept {
        auto attributes =
            RESULT_ATTR_LOOKS_LIKE_TYPO | (is_high_confidence_result ? RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS : 0);
        return SpellingResult(attributes, std::move(suggestions));
    }

    static SpellingResult grammar_error(const std::shared_ptr<const std::vector<ustring_t>> suggestions,
                                        bool is_high_confidence_result = false) noexcept {
        auto attributes = RESULT_ATTR_LOOKS_LIKE_GRAMMAR_ERROR |
                          (is_high_confidence_result ? RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS : 0);
        return SpellingResult(attributes, std::move(suggestions));
    }
};

} // namespace floris::nlp

#endif
