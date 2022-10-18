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

#include <cstdint>
#include <string>
#include <vector>

namespace fl::nlp {

// Even though we use an unsigned integer we reserve the signed bit and only use the positive number range
// available from int32_8. This is to ensure compatibility with Java's integers.
using score_t = uint32_t;
static const score_t SCORE_MIN = 0;
static const score_t SCORE_MAX = INT32_MAX;

using freq_t = double;
static const freq_t FREQ_MIN = 0.0;
static const freq_t FREQ_MAX = 1.0;

// ----- suggestion_request_flags ----- //

class suggestion_request_flags {
  private:
    static const int32_t M_MAX_SUGGESTION_COUNT = 0x00FF;
    static const int32_t F_ALLOW_POSSIBLY_OFFENSIVE = 0x0100;
    static const int32_t F_IS_PRIVATE_SESSION = 0x0200;

  private:
    const int32_t __flags;

  public:
    suggestion_request_flags(int32_t flags) : __flags(flags) {};
    ~suggestion_request_flags() = default;

    int32_t max_suggestion_count() const noexcept {
        return (__flags & M_MAX_SUGGESTION_COUNT);
    }
    bool allow_possibly_offensive() const noexcept {
        return (__flags & F_ALLOW_POSSIBLY_OFFENSIVE) != 0;
    }
    bool is_private_session() const noexcept {
        return (__flags & F_IS_PRIVATE_SESSION) != 0;
    }

    operator int32_t() const noexcept {
        return __flags;
    }
};

// ----- suggestion_candidate ----- //

static const double SUGGESTION_CANDIDATE_MIN_CONFIDENCE = 0.0;
// Everything above 0.9 to 1.0 is reserved for special suggestions such as contacts, clipboard, etc., which is not
// handled in the native implementation
static const double SUGGESTION_CANDIDATE_MAX_CONFIDENCE = 0.9;

struct suggestion_candidate {
    const fl::u8str text = "";
    const fl::u8str secondary_text = "";
    const int edit_distance;
    const double confidence = SUGGESTION_CANDIDATE_MIN_CONFIDENCE;
    const bool is_eligible_for_auto_commit = false;
    const bool is_eligible_for_user_removal = true;
};

// ----- spelling_result ----- //

static const int32_t RESULT_UNSPECIFIED = 0x0000;
static const int32_t RESULT_ATTR_IN_THE_DICTIONARY = 0x0001;
static const int32_t RESULT_ATTR_LOOKS_LIKE_TYPO = 0x0002;
static const int32_t RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS = 0x0004;
static const int32_t RESULT_ATTR_LOOKS_LIKE_GRAMMAR_ERROR = 0x0008;
static const int32_t RESULT_ATTR_DONT_SHOW_UI_FOR_SUGGESTIONS = 0x0010;

class spelling_result {
  public:
    int32_t suggestion_attributes;
    std::vector<fl::u8str> suggestions;

  public:
    spelling_result() : suggestion_attributes(RESULT_UNSPECIFIED) {}
    spelling_result(const int32_t suggestion_attributes) : suggestion_attributes(suggestion_attributes) {}
    spelling_result(const int32_t suggestion_attributes, const std::vector<fl::u8str>& suggestions)
        : suggestion_attributes(suggestion_attributes), suggestions(suggestions) {}
    ~spelling_result() = default;

    static spelling_result unspecified() noexcept {
        return spelling_result(RESULT_UNSPECIFIED);
    }

    static spelling_result valid_word() noexcept {
        return spelling_result(RESULT_ATTR_IN_THE_DICTIONARY);
    }

    static spelling_result typo(
        const std::vector<fl::u8str>& suggestions,
        bool is_high_confidence_result = false
    ) noexcept {
        auto attributes =
            RESULT_ATTR_LOOKS_LIKE_TYPO | (is_high_confidence_result ? RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS : 0);
        return spelling_result(attributes, suggestions);
    }

    static spelling_result grammar_error(
        const std::vector<fl::u8str>& suggestions,
        bool is_high_confidence_result = false
    ) noexcept {
        auto attributes = RESULT_ATTR_LOOKS_LIKE_GRAMMAR_ERROR |
                          (is_high_confidence_result ? RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS : 0);
        return spelling_result(attributes, suggestions);
    }
};

} // namespace fl::nlp

#endif
