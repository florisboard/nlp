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

class ImmutableDictionaryError : public std::runtime_error {
  public:
    ImmutableDictionaryError() : std::runtime_error("Trying to mutate dictionary that is not mutable!") {};
    ~ImmutableDictionaryError() = default;
};

class DictionarySerializationError : public std::runtime_error {
  public:
    DictionarySerializationError(const std::filesystem::path& path, const size_t line_num, const char* msg)
        : std::runtime_error(msg), path(path), line_num(line_num) {};
    ~DictionarySerializationError() = default;

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

static const fl::u8char FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE = 'p';
static const fl::u8char FLDIC_FLAG_IS_HIDDEN_BY_USER = 'h';

struct DictionaryHeader {
    fl::u8str schema = FLDIC_SCHEMA_V0_DRAFT1;
    fl::u8str name;
    std::vector<icu::Locale> locales; // in serialization use BCP 47 tags!
    fl::u8str generated_by;

    size_t readFrom(std::basic_istream<fl::u8char>& istream) noexcept;

    size_t writeTo(std::basic_ostream<fl::u8char>& ostream) const noexcept;

    void reset() noexcept;
};

class Dictionary {
    friend class DictionarySession;

  public:
    Dictionary();
    Dictionary(const std::filesystem::path& path);
    Dictionary(const std::filesystem::path& src_path, const std::filesystem::path& dst_path);
    ~Dictionary();

  public:
    const NgramProperties& viewNgramProperties(fl::u8str& word1) const;
    const NgramProperties& viewNgramProperties(fl::u8str& word1, fl::u8str& word2) const;
    const NgramProperties& viewNgramProperties(fl::u8str& word1, fl::u8str& word2, fl::u8str& word3) const;

    std::filesystem::path src_path;
    std::filesystem::path dst_path;

  protected:
    DictionaryHeader header;
    TrieNode root_node;
    std::map<fl::u8str, fl::u8str> shortcuts;

    score_t max_unigram_score = 1;
    score_t max_bigram_score = 1;
    score_t max_trigram_score = 1;

    void deserialize(std::basic_istream<fl::u8char>& istream);
    void serialize(std::basic_ostream<fl::u8char>& ostream);

  public:
    bool contains(const fl::u8str& word) const noexcept;

  private:
    void trieWriteNgramsTo(std::basic_ostream<fl::u8char>& ostream, TrieNode* base_node, uint8_t ngram_level) const;

    // Using const char* on purpose as this method will almost always be called with string literals
    void throwFatalDeseralizationError(size_t line_num, const char* msg) const;
};

class MutableDictionary : public Dictionary {
  public:
    MutableDictionary();
    MutableDictionary(const std::filesystem::path& path);
    MutableDictionary(const std::filesystem::path& src_path, const std::filesystem::path& dst_path);
    ~MutableDictionary();

    bool adjustScoresIfNecessary() noexcept;

    NgramProperties& insert(const fl::u8str& word1) noexcept;
    void insert(const fl::u8str& word1, const fl::u8str& word2) noexcept;
    void insert(const fl::u8str& word1, const fl::u8str& word2, const fl::u8str& word3) noexcept;

    void remove(const fl::u8str& word1) noexcept;
    void remove(const fl::u8str& word1, const fl::u8str& word2) noexcept;
    void remove(const fl::u8str& word1, const fl::u8str& word2, const fl::u8str& word3) noexcept;

    void persist();

  private:
    static const score_t SCORE_ADJUSTMENT_THRESHOLD = SCORE_MAX - 128;
};

} // namespace fl::nlp

#endif
