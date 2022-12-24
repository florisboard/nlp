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

#ifndef __FLORISNLP_CORE_DICTIONARY_SESSION_H__
#define __FLORISNLP_CORE_DICTIONARY_SESSION_H__

#include "core/common.hpp"
#include "core/dictionary.hpp"
#include "core/key_proximity_map.hpp"
#include "core/string.hpp"

#include <unicode/utext.h>

#include <filesystem>
#include <string>

namespace fl::nlp {

class DictionarySession {
  public:
    static const int MAX_COST = 6;
    static const int COST_IS_EQUAL = 0;
    static const int COST_IS_OPPOSITE_CASE = 1;
    static const int COST_INSERT = 2;
    static const int COST_DELETE = 2;
    static const int COST_SUBSTITUTE_DEFAULT = 2;
    static const int COST_SUBSTITUTE_IN_PROXIMITY = 1;
    static const int COST_TRANSPOSE = 1;
    static const int PENALTY_DEFAULT = 0;
    static const int PENALTY_START_OF_STR = 2;

    fl::u8str locale_tag = "en_us";
    KeyProximityMap key_proximity_mapping;

    DictionarySession() = default;
    DictionarySession(const DictionarySession&) = delete;
    DictionarySession(DictionarySession&&) = delete;
    ~DictionarySession() = default;

    void loadBaseDictionary(const std::filesystem::path& dict_path);

    void loadUserDictionary(const std::filesystem::path& dict_path);

    SpellingResult spell(fl::u8str& word, const std::vector<fl::u8str>& prev_words,
                         const std::vector<fl::u8str>& next_words, SuggestionRequestFlags& flags);

    void suggest(fl::u8str& word, const std::vector<fl::u8str>& prev_words, SuggestionRequestFlags& flags,
                 std::vector<std::unique_ptr<SuggestionCandidate>>& results);

  private:
    std::vector<std::unique_ptr<Dictionary>> _base_dictionaries;
    std::unique_ptr<MutableDictionary> _user_dictionary = nullptr;

    enum class FuzzySearchType {
        Proximity,
        ProximityWithoutSelf,
        ProximityOrPrefix,
    };

    class FuzzySearchState {
      public:
        const DictionarySession& session;
        const FuzzySearchType type;
        const int max_distance;
        const SuggestionRequestFlags flags;
        fl::u8chstr_vec word_chars;
        fl::u8chstr_vec word_chars_opposite_case;
        fl::u8chstr_vec prefix_chars;
        std::vector<std::vector<int>> distances;
        std::function<void(fl::u8str&&, const TrieNode*, int)> on_result;

        FuzzySearchState(const DictionarySession& session, const FuzzySearchType type, const int max_distance,
                         const SuggestionRequestFlags& flags, const fl::u8str& word);
        FuzzySearchState(const FuzzySearchState&) = delete;
        FuzzySearchState(FuzzySearchState&&) = delete;
        ~FuzzySearchState() = default;

        void setPrefixChstrAt(std::size_t prefix_index, const fl::u8chstr& chstr) noexcept;

        int editDistanceAt(std::size_t prefix_index) const noexcept;

        fl::u8str prefixStrAt(std::size_t prefix_index) const noexcept;

        bool isDeadEndAt(std::size_t prefix_index) const noexcept;

      private:
        void initWordChars(const fl::u8str& word) noexcept;

        void ensureCapacityFor(std::size_t prefix_index) noexcept;
    };

    /**
     * UTF-8 aware fuzzy search algorithm searching a trie and returning all words within a certain given distance.
     *
     * This algorithm utilizes the
     * [Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance) as a bas
     * distance metric, together with a penalty system for unusual operations.
     *
     * TODO: This algorithm is not fully UTF-8 aware yet. Has severe issues outside ASCII in the prefix and chstr area
     */
    void fuzzySearchRecursiveDld(const TrieNode* node, FuzzySearchState& state, int prefix_index) const noexcept;

    void fuzzySearch(const TrieNode* root_node, FuzzySearchType type, int max_distance,
                     const SuggestionRequestFlags& flags, const fl::u8str& word,
                     std::function<void(fl::u8str&&, const TrieNode*, int)> on_result) const noexcept;
};

} // namespace fl::nlp

#endif
