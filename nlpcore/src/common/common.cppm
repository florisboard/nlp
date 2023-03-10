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
#include <string>
#include <vector>

export module fl.nlp.core.common;

export import :dictionary;
export import :key_proximity_checker;
export import :suggestion_request_flags;
export import :trie_map;

import fl.nlp.string;

namespace fl::nlp {

// ----- SuggestionCandidate ----- //

export constexpr double SUGGESTION_CANDIDATE_MIN_CONFIDENCE = 0.0;
// Everything above 0.9 to 1.0 is reserved for special suggestions such as contacts, clipboard, etc., which is not
// handled in the native implementation
export constexpr double SUGGESTION_CANDIDATE_MAX_CONFIDENCE = 0.9;

export struct SuggestionCandidate {
    const std::string text;
    const std::string secondary_text;
    const int edit_distance;
    const double confidence = SUGGESTION_CANDIDATE_MIN_CONFIDENCE;
    const bool is_eligible_for_auto_commit = false;
    const bool is_eligible_for_user_removal = true;
};

// ----- SpellingResult ----- //

export constexpr int32_t RESULT_UNSPECIFIED = 0x0000;
export constexpr int32_t RESULT_ATTR_IN_THE_DICTIONARY = 0x0001;
export constexpr int32_t RESULT_ATTR_LOOKS_LIKE_TYPO = 0x0002;
export constexpr int32_t RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS = 0x0004;
export constexpr int32_t RESULT_ATTR_LOOKS_LIKE_GRAMMAR_ERROR = 0x0008;
export constexpr int32_t RESULT_ATTR_DONT_SHOW_UI_FOR_SUGGESTIONS = 0x0010;

export class SpellingResult {
  public:
    int32_t suggestion_attributes;
    std::vector<std::string> suggestions;

  public:
    SpellingResult() : suggestion_attributes(RESULT_UNSPECIFIED) {}
    explicit SpellingResult(const int32_t suggestion_attributes) : suggestion_attributes(suggestion_attributes) {}
    SpellingResult(const int32_t suggestion_attributes, const std::vector<std::string>& suggestions)
        : suggestion_attributes(suggestion_attributes), suggestions(suggestions) {}
    ~SpellingResult() = default;

    static SpellingResult unspecified() noexcept {
        return SpellingResult(RESULT_UNSPECIFIED);
    }

    static SpellingResult validWord() noexcept {
        return SpellingResult(RESULT_ATTR_IN_THE_DICTIONARY);
    }

    static SpellingResult
    typo(const std::vector<std::string>& suggestions, bool is_high_confidence_result = false) noexcept {
        auto attributes =
            RESULT_ATTR_LOOKS_LIKE_TYPO | (is_high_confidence_result ? RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS : 0);
        return {attributes, suggestions};
    }

    static SpellingResult
    grammarError(const std::vector<std::string>& suggestions, bool is_high_confidence_result = false) noexcept {
        auto attributes = RESULT_ATTR_LOOKS_LIKE_GRAMMAR_ERROR |
                          (is_high_confidence_result ? RESULT_ATTR_HAS_RECOMMENDED_SUGGESTIONS : 0);
        return {attributes, suggestions};
    }
};

} // namespace fl::nlp
