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

#ifndef __FLORISNLP_CORE_NLP_DICTIONARY_H__
#define __FLORISNLP_CORE_NLP_DICTIONARY_H__

#include "icuext/string.hpp"
#include "nlp/common.hpp"
#include "nlp/trie.hpp"

#include <unicode/locid.h>

#include <filesystem>
#include <istream>
#include <locale>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>

namespace floris::nlp {

class immutable_dictionary_error : public std::runtime_error {
  public:
    immutable_dictionary_error() : std::runtime_error("Trying to mutate dictionary that is not mutable!") {};
    ~immutable_dictionary_error() = default;
};

class dictionary_serialization_error : public std::runtime_error {
  public:
    dictionary_serialization_error(const std::filesystem::path& path, const size_t line_num, const char* msg)
        : std::runtime_error(msg), path(path), line_num(line_num) {};
    ~dictionary_serialization_error() = default;

  private:
    const std::filesystem::path path;
    const size_t line_num;
};

// Atm the schema URL is only used as a long version string, however for the future it enables us to define and support
// different schemas.
static const icuext::u8str FLDIC_SCHEMA_V0_DRAFT1 = "https://florisboard.org/schemas/fldic/v0~draft1/dictionary.txt";

static const icuext::u8char FLDIC_ASSIGNMENT = '=';
static const icuext::u8char FLDIC_NEWLINE = '\n';
static const icuext::u8char FLDIC_LIST_SEPARATOR = ',';
static const icuext::u8char FLDIC_SEPARATOR = '\t';

static const icuext::u8str FLDIC_HEADER_SCHEMA = "schema";
static const icuext::u8str FLDIC_HEADER_NAME = "name";
static const icuext::u8str FLDIC_HEADER_LOCALES = "locales";
static const icuext::u8str FLDIC_HEADER_GENERATED_BY = "generated_by";

static const icuext::u8str FLDIC_SECTION_WORDS = "[words]";
static const icuext::u8str FLDIC_SECTION_SHORTCUTS = "[shortcuts]";

static const icuext::u8char FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE = 'o';
static const icuext::u8char FLDIC_FLAG_IS_HIDDEN_BY_USER = 'h';

struct dictionary_header {
    icuext::u8str schema = FLDIC_SCHEMA_V0_DRAFT1;
    icuext::u8str name;
    std::vector<icu::Locale> locales; // in serialization use BCP 47 tags!
    icuext::u8str generated_by;

    size_t read_from(std::basic_istream<icuext::u8char>& istream) noexcept;

    size_t write_to(std::basic_ostream<icuext::u8char>& ostream) const noexcept;

    void reset() noexcept;
};

class dictionary {
  public:
    dictionary(const std::filesystem::path& path);
    dictionary(const std::filesystem::path& src_path, const std::filesystem::path& dst_path);
    ~dictionary();

  public:
    const ngram_properties& view_ngram_properties(icuext::u8str& word1) const;
    const ngram_properties& view_ngram_properties(icuext::u8str& word1, icuext::u8str& word2) const;
    const ngram_properties& view_ngram_properties(icuext::u8str& word1, icuext::u8str& word2, icuext::u8str& word3)
        const;

  protected:
    std::filesystem::path src_path;
    std::filesystem::path dst_path;

    dictionary_header header;
    basic_trie_node root_node;
    std::map<icuext::u8str, icuext::u8str> shortcuts;

    score_t max_unigram_score;
    score_t max_bigram_score;
    score_t max_trigram_score;

    void deserialize(std::basic_istream<icuext::u8char>& istream);
    void serialize(std::basic_ostream<icuext::u8char>& ostream);

  private:
    void trie_write_ngrams_to(
        std::basic_ostream<icuext::u8char>& ostream,
        basic_trie_node* base_node,
        uint8_t ngram_level
    ) const;

    // Using const char* on purpose as this method will almost always be called with string literals
    void throw_fatal_deseralization_error(size_t line_num, const char* msg) const;
};

class mutable_dictionary : public dictionary {
  public:
    mutable_dictionary(const std::filesystem::path& path);
    mutable_dictionary(const std::filesystem::path& src_path, const std::filesystem::path& dst_path);
    ~mutable_dictionary();

    bool adjust_scores_if_necessary() noexcept;

    void insert(const icuext::u8str& word1, const ngram_properties& properties) noexcept;
    void insert(const icuext::u8str& word1, const icuext::u8str& word2, const ngram_properties& properties) noexcept;
    void insert(
        const icuext::u8str& word1,
        const icuext::u8str& word2,
        const icuext::u8str& word3,
        const ngram_properties& properties
    ) noexcept;

    void remove(const icuext::u8str& word1) noexcept;
    void remove(const icuext::u8str& word1, const icuext::u8str& word2) noexcept;
    void remove(const icuext::u8str& word1, const icuext::u8str& word2, const icuext::u8str& word3) noexcept;

    void persist();

  private:
    static const score_t SCORE_ADJUSTMENT_THRESHOLD = SCORE_MAX - 128;
};

} // namespace floris::nlp

#endif
