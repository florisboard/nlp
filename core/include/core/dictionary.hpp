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

#ifndef __FLORISNLP_CORE_DICTIONARY_H__
#define __FLORISNLP_CORE_DICTIONARY_H__

#include "core/common.hpp"
#include "core/trie.hpp"

#include <unicode/locid.h>

#include <filesystem>
#include <istream>
#include <locale>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>

namespace fl::nlp {

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
static const fl::u8str FLDIC_SCHEMA_V0_DRAFT1 = "https://florisboard.org/schemas/fldic/v0~draft1/dictionary.txt";

static const fl::u8char FLDIC_ASSIGNMENT = '=';
static const fl::u8char FLDIC_NEWLINE = '\n';
static const fl::u8char FLDIC_LIST_SEPARATOR = ',';
static const fl::u8char FLDIC_SEPARATOR = '\t';

static const fl::u8str FLDIC_HEADER_SCHEMA = "schema";
static const fl::u8str FLDIC_HEADER_NAME = "name";
static const fl::u8str FLDIC_HEADER_LOCALES = "locales";
static const fl::u8str FLDIC_HEADER_GENERATED_BY = "generated_by";

static const fl::u8str FLDIC_SECTION_WORDS = "[words]";
static const fl::u8str FLDIC_SECTION_SHORTCUTS = "[shortcuts]";

static const fl::u8char FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE = 'o';
static const fl::u8char FLDIC_FLAG_IS_HIDDEN_BY_USER = 'h';

struct dictionary_header {
    fl::u8str schema = FLDIC_SCHEMA_V0_DRAFT1;
    fl::u8str name;
    std::vector<icu::Locale> locales; // in serialization use BCP 47 tags!
    fl::u8str generated_by;

    size_t read_from(std::basic_istream<fl::u8char>& istream) noexcept;

    size_t write_to(std::basic_ostream<fl::u8char>& ostream) const noexcept;

    void reset() noexcept;
};

class dictionary {
    friend class dictionary_session;

  public:
    dictionary();
    dictionary(const std::filesystem::path& path);
    dictionary(const std::filesystem::path& src_path, const std::filesystem::path& dst_path);
    ~dictionary();

  public:
    const ngram_properties& view_ngram_properties(fl::u8str& word1) const;
    const ngram_properties& view_ngram_properties(fl::u8str& word1, fl::u8str& word2) const;
    const ngram_properties& view_ngram_properties(fl::u8str& word1, fl::u8str& word2, fl::u8str& word3) const;

    std::filesystem::path src_path;
    std::filesystem::path dst_path;

  protected:
    dictionary_header header;
    basic_trie_node root_node;
    std::map<fl::u8str, fl::u8str> shortcuts;

    score_t max_unigram_score;
    score_t max_bigram_score;
    score_t max_trigram_score;

    void deserialize(std::basic_istream<fl::u8char>& istream);
    void serialize(std::basic_ostream<fl::u8char>& ostream);

  private:
    void trie_write_ngrams_to(std::basic_ostream<fl::u8char>& ostream, basic_trie_node* base_node, uint8_t ngram_level)
        const;

    // Using const char* on purpose as this method will almost always be called with string literals
    void throw_fatal_deseralization_error(size_t line_num, const char* msg) const;
};

class mutable_dictionary : public dictionary {
  public:
    mutable_dictionary();
    mutable_dictionary(const std::filesystem::path& path);
    mutable_dictionary(const std::filesystem::path& src_path, const std::filesystem::path& dst_path);
    ~mutable_dictionary();

    bool adjust_scores_if_necessary() noexcept;

    void insert(const fl::u8str& word1, const ngram_properties& properties) noexcept;
    void insert(const fl::u8str& word1, const fl::u8str& word2, const ngram_properties& properties) noexcept;
    void insert(
        const fl::u8str& word1,
        const fl::u8str& word2,
        const fl::u8str& word3,
        const ngram_properties& properties
    ) noexcept;

    void remove(const fl::u8str& word1) noexcept;
    void remove(const fl::u8str& word1, const fl::u8str& word2) noexcept;
    void remove(const fl::u8str& word1, const fl::u8str& word2, const fl::u8str& word3) noexcept;

    void persist();

  private:
    static const score_t SCORE_ADJUSTMENT_THRESHOLD = SCORE_MAX - 128;
};

} // namespace fl::nlp

#endif
