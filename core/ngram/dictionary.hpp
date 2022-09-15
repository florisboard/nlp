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

#ifndef _FLORISNLP_CORE_NGRAM_DICTIONARY
#define _FLORISNLP_CORE_NGRAM_DICTIONARY

#include <filesystem>
#include <locale>
#include <string>
#include <tsl/htrie_map.h>

namespace floris::nlp {

enum DictionaryType {
    Unspecified = 0,
    ReadOnlyDictionary = 1,
    MutableDictionary = 2,
};

struct HtrieNodeProperties {
    score_t weighted_score;
    bool is_possibly_offensive : 1;
    bool is_hidden_by_user : 1;

    uint8_t ngramLevel;
    tsl::htrie_map<ucodepoint_t, HtrieNodeProperties> ngramChildren;
};

class Dictionary {
  protected:
    tsl::htrie_map<ucodepoint_t, HtrieNodeProperties> ngram_data;
    score_t score_offset;
    bool is_open;

  public:
    const DictionaryType type;
    const std::vector<std::locale> locales;
    const score_t learn_factor;
    const score_t decay_factor;

  protected:
    Dictionary(const DictionaryType type,
               const std::vector<std::locale>& locales,
               const score_t learn_factor,
               const score_t decay_factor)
        : type(type), locales(locales), learn_factor(learn_factor), decay_factor(decay_factor), score_offset(0u) {};
    ~Dictionary();

  public:
    virtual SpellingResult&
    spell(ustring_t word, int max_suggestion_count, bool allow_possibly_offensive, bool is_private_session);

    virtual void suggest();

    virtual bool close();

    bool is_mutable() const noexcept {
        return type == DictionaryType::MutableDictionary;
    }

    std::vector<ustring_t> get_list_of_words() const noexcept {
        std::vector<ustring_t> ret_list(ngram_data.size());
        for (auto it = ngram_data.begin(); it != ngram_data.end(); ++it) {
            ret_list.push_back(it.key());
        }
        return ret_list;
    }

    double get_frequency_for_word(ustring_t& word) const noexcept {
        try {
            auto& word_properties = ngram_data.at(word);
            return static_cast<double>(word_properties.weighted_score) / SCORE_MAX;
        } catch (std::out_of_range e) {
            return 0.0;
        }
    }
};

} // namespace floris::nlp

#endif
