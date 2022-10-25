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

class dictionary_session {
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
    key_proximity_map key_proximity_mapping;

    dictionary_session() = default;
    dictionary_session(const dictionary_session&) = delete;
    dictionary_session(dictionary_session&&) = delete;
    ~dictionary_session() = default;

    void load_base_dictionary(const std::filesystem::path& dict_path);

    void load_user_dictionary(const std::filesystem::path& dict_path);

    spelling_result spell(
        fl::u8str& word,
        const std::vector<fl::u8str>& prev_words,
        const std::vector<fl::u8str>& next_words,
        suggestion_request_flags& flags
    );

    void suggest(
        fl::u8str& word,
        const std::vector<fl::u8str>& prev_words,
        suggestion_request_flags& flags,
        std::vector<std::unique_ptr<suggestion_candidate>>& results
    );

  private:
    std::vector<std::unique_ptr<dictionary>> _base_dictionaries;
    std::unique_ptr<mutable_dictionary> _user_dictionary = nullptr;

    enum class fuzzy_search_type {
        proximity,
        proximity_without_self,
        proximity_or_prefix,
    };

    class fuzzy_search_state {
      public:
        const dictionary_session& session;
        const fuzzy_search_type type;
        const int max_distance;
        const suggestion_request_flags flags;
        std::vector<fl::u8str> word_chars;
        std::vector<fl::u8str> word_chars_opposite_case;
        std::vector<fl::u8str> prefix_chars;
        std::vector<std::vector<int>> distances;
        std::function<void(fl::u8str&&, const trie_node*, int)> on_result;

        fuzzy_search_state(
            const dictionary_session& session,
            const fuzzy_search_type type,
            const int max_distance,
            const suggestion_request_flags& flags,
            const fl::u8str& word
        );
        fuzzy_search_state(const fuzzy_search_state&) = delete;
        fuzzy_search_state(fuzzy_search_state&&) = delete;
        ~fuzzy_search_state() = default;

        void set_prefix_chstr_at(std::size_t prefix_index, const fl::u8str& chstr) noexcept;

        int edit_distance_at(std::size_t prefix_index) const noexcept;

        fl::u8str prefix_str_at(std::size_t prefix_index) const noexcept;

        bool is_dead_end_at(std::size_t prefix_index) const noexcept;

      private:
        void init_word_chars(const fl::u8str& word) noexcept;

        void ensure_capacity_for(std::size_t prefix_index) noexcept;
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
    void fuzzy_search_recursive_dld(const trie_node* node, fuzzy_search_state& state, int prefix_index) const noexcept;

    void fuzzy_search(
        const trie_node* root_node,
        fuzzy_search_type type,
        int max_distance,
        const suggestion_request_flags& flags,
        const fl::u8str& word,
        std::function<void(fl::u8str&&, const trie_node*, int)> on_result
    ) const noexcept;
};

} // namespace fl::nlp

#endif
