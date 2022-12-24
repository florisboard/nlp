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

#ifndef __FLORISNLP_CORE_COMMON_H__
#define __FLORISNLP_CORE_COMMON_H__

#include "core/string.hpp"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace fl::nlp {

// Even though we use an unsigned integer we reserve the signed bit and only use the positive number range
// available from int32_8. This is to ensure compatibility with Java's integers.
using score_t = uint32_t;
static const score_t SCORE_MIN = 0;
static const score_t SCORE_MAX = 0x00FFFFFFu;

using freq_t = double;
static const freq_t FREQ_MIN = 0.0;
static const freq_t FREQ_MAX = 1.0;

// ----- SuggestionRequestFlags ----- //

class SuggestionRequestFlags {
  public:
    static const int32_t M_MAX_SUGGESTION_COUNT = 0x00FF;
    static const int32_t F_ALLOW_POSSIBLY_OFFENSIVE = 0x0100;
    static const int32_t F_IS_PRIVATE_SESSION = 0x0200;

  private:
    int32_t __flags;

  public:
    SuggestionRequestFlags(int32_t flags) : __flags(flags) {};
    ~SuggestionRequestFlags() = default;

    int32_t maxSuggestionCount() const noexcept {
        return (__flags & M_MAX_SUGGESTION_COUNT);
    }
    bool allowPossiblyOffensive() const noexcept {
        return (__flags & F_ALLOW_POSSIBLY_OFFENSIVE) != 0;
    }
    bool isPrivateSession() const noexcept {
        return (__flags & F_IS_PRIVATE_SESSION) != 0;
    }

    operator int32_t() const noexcept {
        return __flags;
    }
};

// ----- SuggestionCandidate ----- //

static const double SUGGESTION_CANDIDATE_MIN_CONFIDENCE = 0.0;
// Everything above 0.9 to 1.0 is reserved for special suggestions such as contacts, clipboard, etc., which is not
// handled in the native implementation
static const double SUGGESTION_CANDIDATE_MAX_CONFIDENCE = 0.9;

struct SuggestionCandidate {
    const fl::u8str text = "";
    const fl::u8str secondary_text = "";
    const int edit_distance;
    const double confidence = SUGGESTION_CANDIDATE_MIN_CONFIDENCE;
    const bool is_eligible_for_auto_commit = false;
    const bool is_eligible_for_user_removal = true;
};

// ----- SpellingResult ----- //

static const int32_t RESULT_UNSPECIFIED = 0x0000;
static const int32_t RESULT_ATTR_IN_THE_DICTIONARY = 0x0001;
static const int32_t RESULT_ATTR_LOOKS_LIKE_TYPO = 0x0002;
static const int32_t RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS = 0x0004;
static const int32_t RESULT_ATTR_LOOKS_LIKE_GRAMMAR_ERROR = 0x0008;
static const int32_t RESULT_ATTR_DONT_SHOW_UI_FOR_SUGGESTIONS = 0x0010;

class SpellingResult {
  public:
    int32_t suggestion_attributes;
    std::vector<fl::u8str> suggestions;

  public:
    SpellingResult() : suggestion_attributes(RESULT_UNSPECIFIED) {}
    SpellingResult(const int32_t suggestion_attributes) : suggestion_attributes(suggestion_attributes) {}
    SpellingResult(const int32_t suggestion_attributes, const std::vector<fl::u8str>& suggestions)
        : suggestion_attributes(suggestion_attributes), suggestions(suggestions) {}
    ~SpellingResult() = default;

    static SpellingResult unspecified() noexcept {
        return SpellingResult(RESULT_UNSPECIFIED);
    }

    static SpellingResult validWord() noexcept {
        return SpellingResult(RESULT_ATTR_IN_THE_DICTIONARY);
    }

    static SpellingResult typo(const std::vector<fl::u8str>& suggestions,
                               bool is_high_confidence_result = false) noexcept {
        auto attributes =
            RESULT_ATTR_LOOKS_LIKE_TYPO | (is_high_confidence_result ? RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS : 0);
        return SpellingResult(attributes, suggestions);
    }

    static SpellingResult grammarError(const std::vector<fl::u8str>& suggestions,
                                       bool is_high_confidence_result = false) noexcept {
        auto attributes = RESULT_ATTR_LOOKS_LIKE_GRAMMAR_ERROR |
                          (is_high_confidence_result ? RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS : 0);
        return SpellingResult(attributes, suggestions);
    }
};

} // namespace fl::nlp

#endif
