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

#include <unicode/locid.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <vector>

export module fl.nlp.core.latin:dictionary;

import :entry_properties;
import fl.nlp.string;
import fl.nlp.core.common;
import fl.nlp.utils;

namespace fl::nlp {

export using LatinTrieNode = TrieNode<fl::str::UniChar, EntryProperties>;
export using LatinDictId = std::uint8_t;

export const auto FLDIC_SECTION_WORDS = "[words]";
export const auto FLDIC_SECTION_NGRAMS = "[ngrams]";
export const auto FLDIC_SECTION_SHORTCUTS = "[shortcuts]";

export const char FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE = 'p';
export const char FLDIC_FLAG_IS_HIDDEN_BY_USER = 'h';

export const char FLDIC_TOKEN_ID_START_OF_SENTENCE = 0x02; // Start of text ASCII CTRL char
export const char FLDIC_TOKEN_ID_NGRAM_SEPARATOR = 0x1E;   // Record separator ASCII CTRL char
export const char FLDIC_TOKEN_ID_LIMIT = 0x20;             // Space char
export const fl::str::UniChar FLDIC_TOKEN_START_OF_SENTENCE = {1, FLDIC_TOKEN_ID_START_OF_SENTENCE};
export const fl::str::UniChar FLDIC_TOKEN_NGRAM_SEPARATOR = {1, FLDIC_TOKEN_ID_NGRAM_SEPARATOR};
export const std::vector<fl::str::UniChar> FLDIC_SEARCH_TERMINATION_TOKENS = {FLDIC_TOKEN_NGRAM_SEPARATOR};

export enum class LatinDictionarySection {
    UNSPECIFIED,
    WORDS,
    NGRAMS,
    SHORTCUTS,
};

export class LatinDictionary : public Dictionary {
  private:
    using WordActionT =
        const std::function<void(std::span<const fl::str::UniChar>, LatinTrieNode*, WordEntryProperties*)>;
    using NgramActionT = const std::function<
        void(std::span<const fl::str::UniString>, EntryType, LatinTrieNode*, NgramEntryProperties*)>;
    using ShortcutActionT =
        const std::function<void(std::span<const fl::str::UniChar>, LatinTrieNode*, ShortcutEntryProperties*)>;
    using EntryActionT =
        const std::function<void(std::span<const fl::str::UniString>, int32_t, LatinTrieNode*, EntryProperties*)>;

  public:
    std::size_t dict_id_;
    std::shared_ptr<LatinTrieNode> data_;

    std::map<EntryType, int32_t> total_scores_;
    std::map<EntryType, int32_t> vocab_sizes_;
    std::map<EntryType, int32_t> global_penalties_;

    LatinDictionary() = delete;
    LatinDictionary(LatinDictId id, std::shared_ptr<LatinTrieNode> shared_data) : dict_id_(id), data_(shared_data) {};
    LatinDictionary(LatinDictId id) : dict_id_(id), data_(std::make_shared<LatinTrieNode>()) {};
    LatinDictionary(const LatinDictionary&) = delete;
    LatinDictionary(LatinDictionary&&) = default;
    virtual ~LatinDictionary() = default;

    LatinDictionary& operator=(const LatinDictionary&) = delete;
    LatinDictionary& operator=(LatinDictionary&&) = default;

    TrieNode<fl::str::UniChar, EntryProperties>* insertNgram(std::span<const fl::str::UniString> ngram) {
        auto ngram_node = data_.get();
        for (int i = 0; i < ngram.size(); i++) {
            auto& word = ngram[i];
            auto word_node = ngram_node->findOrCreate(word);
            auto word_value = word_node->valueOrCreate(dict_id_);
            if (i + 1 != ngram.size()) {
                ngram_node = word_node->findOrCreate(FLDIC_TOKEN_NGRAM_SEPARATOR);
            } else {
                word_value->ngramPropertiesOrCreate();
                return word_node;
            }
        }
        return nullptr;
    }

    inline void forEachEntry(EntryActionT& action) {
        std::vector<fl::str::UniString> buffer;
        forEachEntryInternal(-1, -1, 1, data_.get(), buffer, action);
    }

    inline void forEachWord(WordActionT& action) {
        data_->forEach(FLDIC_SEARCH_TERMINATION_TOKENS, [&](auto word, auto* node) {
            auto* value = node->valueOrNull(dict_id_);
            if (value == nullptr) return;
            auto* properties = value->wordPropertiesOrNull();
            if (properties == nullptr) return;
            action(word, node, properties);
        });
    }

    inline void forEachNgram(NgramActionT& action) {
        forEachNgram(-1, -1, action);
    }

    inline void forEachNgram(int32_t desired_ngram_size, NgramActionT& action) {
        forEachNgram(desired_ngram_size, desired_ngram_size, action);
    }

    inline void forEachNgram(int32_t desired_ngram_size_min, int32_t desired_ngram_size_max, NgramActionT& action) {
        std::vector<fl::str::UniString> buffer;
        forEachEntryInternal(
            desired_ngram_size_min, desired_ngram_size_max, 1, data_.get(), buffer,
            [&](auto ngram, auto ngram_size, auto* node, auto* value) {
                auto properties = value->ngramPropertiesOrNull();
                if (properties == nullptr) return;
                action(ngram, EntryType::ngram(ngram_size), node, properties);
            }
        );
    }

    inline void forEachShortcut(ShortcutActionT& action) {
        data_->forEach(FLDIC_SEARCH_TERMINATION_TOKENS, [&](auto word, auto* node) {
            auto* value = node->valueOrNull(dict_id_);
            if (value == nullptr) return;
            auto* properties = value->shortcutPropertiesOrNull();
            if (properties == nullptr) return;
            action(word, node, properties);
        });
    }

    void recalculateAllFrequencyScores() noexcept {
        std::map<EntryType, int32_t> new_total_scores;
        std::map<EntryType, int32_t> new_vocab_sizes;

        forEachEntry([&](auto ngram, auto ngram_size, auto* node, auto* value) {
            if (ngram_size == 1) {
                // Word
                if (auto properties = value->wordPropertiesOrNull(); properties != nullptr) {
                    auto type = EntryType::word();
                    properties->absolute_score = std::max(0, properties->absolute_score - global_penalties_[type]);
                    new_total_scores[type] += properties->absolute_score;
                    new_vocab_sizes[type]++;
                }
                // Shortcut
                if (auto properties = value->shortcutPropertiesOrNull(); properties != nullptr) {
                    auto type = EntryType::shortcut();
                    properties->absolute_score = std::max(0, properties->absolute_score - global_penalties_[type]);
                    new_total_scores[type] += properties->absolute_score;
                    new_vocab_sizes[type]++;
                }
            } else {
                // Ngram
                if (auto properties = value->ngramPropertiesOrNull(); properties != nullptr) {
                    auto type = EntryType::ngram(ngram_size);
                    properties->absolute_score = std::max(0, properties->absolute_score - global_penalties_[type]);
                    new_total_scores[type] += properties->absolute_score;
                    new_vocab_sizes[type]++;
                }
            }
        });

        total_scores_ = new_total_scores;
        vocab_sizes_ = new_vocab_sizes;
        global_penalties_.clear();
    }

    void recalculateFrequencyScores(EntryType type) noexcept {
        const int32_t penalty = global_penalties_[type];
        int32_t total_score = 0;
        int32_t vocab_size = 0;

        if (type.isWord()) {
            forEachWord([&](auto word, auto* node, auto* properties) {
                properties->absolute_score = std::max(0, properties->absolute_score - penalty);
                total_score += properties->absolute_score;
                vocab_size++;
            });
        } else if (type.isNgram()) {
            forEachNgram(type.ngramSize(), [&](auto ngram, auto type, auto* node, auto* properties) {
                properties->absolute_score = std::max(0, properties->absolute_score - penalty);
                total_score += properties->absolute_score;
                vocab_size++;
            });
        } else if (type.isShortcut()) {
            forEachShortcut([&](auto shortcut, auto* node, auto* properties) {
                properties->absolute_score = std::max(0, properties->absolute_score - penalty);
                total_score += properties->absolute_score;
                vocab_size++;
            });
        }

        total_scores_[type] = total_score;
        vocab_sizes_[type] = vocab_size;
        global_penalties_[type] = 0;
    }

    [[nodiscard]]
    double calculateFrequency(EntryType type, int32_t score, int32_t k_offset) const noexcept {
        const int32_t N = fl::utils::findOrDefault(total_scores_, type, 0);
        const int32_t V = fl::utils::findOrDefault(vocab_sizes_, type, 0);
        return static_cast<double>(score + k_offset) / static_cast<double>(N + k_offset * V);
    }

  private:
    void forEachEntryInternal(
        int32_t desired_ngram_size_min,
        int32_t desired_ngram_size_max,
        int32_t current_ngram_size,
        LatinTrieNode* current_ngram_node,
        std::vector<fl::str::UniString>& buffer,
        EntryActionT& action
    ) {
        if (desired_ngram_size_min == 0 || desired_ngram_size_max == 0) return;
        current_ngram_node->forEach(FLDIC_SEARCH_TERMINATION_TOKENS, [&](auto word_span, auto* word_node) {
            auto* value = word_node->valueOrNull(dict_id_);
            if (value == nullptr) return;
            buffer.resize(current_ngram_size);
            buffer[current_ngram_size - 1].assign(word_span.begin(), word_span.end());
            if (desired_ngram_size_min < 0 || desired_ngram_size_max < 0 ||
                desired_ngram_size_min <= current_ngram_size && current_ngram_size <= desired_ngram_size_max) {
                action(buffer, current_ngram_size, word_node, value);
                if (current_ngram_size == desired_ngram_size_max) return;
            }
            auto* next_ngram_node = word_node->findOrNull(FLDIC_TOKEN_NGRAM_SEPARATOR);
            if (next_ngram_node == nullptr) return;
            forEachEntryInternal(
                desired_ngram_size_min, desired_ngram_size_max, current_ngram_size + 1, next_ngram_node, buffer, action
            );
        });
    }

    void deserializeContent(std::istream& istream) override {
        auto section = LatinDictionarySection::UNSPECIFIED;
        std::string line;
        std::vector<std::string> line_components;
        fl::str::UniString word;
        std::vector<std::string> id_components;
        std::vector<fl::str::UniString> ngram;
        std::map<WordIdT, fl::str::UniString> id_to_words_map;
        WordIdT current_word_id = 1;

        while (std::getline(istream, line)) {
            fl::str::trim(line);
            if (line.empty() || line.starts_with(FLDIC_LINE_COMMENT)) continue;

            if (line.starts_with('[')) {
                // Section header
                if (line == FLDIC_SECTION_WORDS) {
                    section = LatinDictionarySection::WORDS;
                } else if (line == FLDIC_SECTION_NGRAMS) {
                    section = LatinDictionarySection::NGRAMS;
                } else if (line == FLDIC_SECTION_SHORTCUTS) {
                    section = LatinDictionarySection::SHORTCUTS;
                } else {
                    throw std::runtime_error("Encountered invalid section name!");
                }
                continue;
            }

            if (section == LatinDictionarySection::WORDS) {
                fl::str::split(line, FLDIC_SEPARATOR, line_components);
                if (line_components.size() < 2) {
                    throw std::runtime_error("Invalid line!");
                }
                fl::str::toUniString(line_components[0], word);
                auto node = data_->findOrCreate(word);
                auto properties = node->valueOrCreate(dict_id_)->wordPropertiesOrCreate();
                // Parse score
                properties->absolute_score = std::stoi(line_components[1]);
                // Parse flags
                if (line_components.size() > 2) {
                    for (const auto& flag : line_components[2]) {
                        if (flag == FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE) {
                            properties->is_possibly_offensive = true;
                        } else if (flag == FLDIC_FLAG_IS_HIDDEN_BY_USER) {
                            properties->is_hidden_by_user = true;
                        }
                    }
                }
                // Assign word to word ID map
                id_to_words_map[current_word_id++] = std::move(word);
            } else if (section == LatinDictionarySection::NGRAMS) {
                fl::str::split(line, FLDIC_SEPARATOR, line_components);
                if (line_components.size() < 2) {
                    throw std::runtime_error("Invalid line!");
                }
                fl::str::split(line_components[0], FLDIC_LIST_SEPARATOR, id_components);
                ngram.resize(id_components.size());
                for (int i = 0; i < id_components.size(); i++) {
                    auto id = std::stoi(id_components[i]);
                    word = isSpecialId(id) ? convertSpecialIdToToken(id) : id_to_words_map[id];
                    ngram[i] = std::move(word);
                }
                auto node = insertNgram(ngram);
                auto properties = node->value(dict_id_)->ngramProperties();
                properties->absolute_score = std::stoi(line_components[1]);
            } else if (section == LatinDictionarySection::SHORTCUTS) {
                // throw std::runtime_error("TODO: implement shortcuts");
            }
        }

        // TODO: do this directly when reading the words/ngrams and avoid this heavy op
        recalculateAllFrequencyScores();
    }

    void serializeContent(std::ostream& ostream) override {
        serializeWords(ostream);
        serializeNgrams(ostream);
        serializeShortcuts(ostream);
    }

    void serializeWords(std::ostream& ostream) noexcept {
        ostream << FLDIC_NEWLINE << FLDIC_SECTION_WORDS << FLDIC_NEWLINE;
        WordIdT current_word_id = 1;
        std::string word;
        data_->forEach(
            FLDIC_SEARCH_TERMINATION_TOKENS,
            [&](std::span<const fl::str::UniChar> uni_word, TrieNode<fl::str::UniChar, EntryProperties>* node) {
                auto value = node->valueOrNull(dict_id_);
                if (value == nullptr) {
                    return;
                }
                auto word_properties = value->wordPropertiesOrNull();
                if (word_properties == nullptr) {
                    return;
                }
                fl::str::toStdString(uni_word, word);
                auto score = word_properties->absolute_score - global_penalties_[EntryType::word()];
                ostream << word << FLDIC_SEPARATOR << score;
                if (word_properties->is_possibly_offensive || word_properties->is_hidden_by_user) {
                    ostream << FLDIC_SEPARATOR;
                    if (word_properties->is_possibly_offensive) {
                        ostream << FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE;
                    }
                    if (word_properties->is_hidden_by_user) {
                        ostream << FLDIC_FLAG_IS_HIDDEN_BY_USER;
                    }
                }
                ostream << FLDIC_NEWLINE;
                word_properties->internal_id = current_word_id++;
            }
        );
    }

    void serializeNgrams(std::ostream& ostream) noexcept {
        ostream << FLDIC_NEWLINE << FLDIC_SECTION_NGRAMS << FLDIC_NEWLINE;
        std::vector<WordIdT> ngram;
        serializeNgrams(ostream, ngram, 1, data_.get());
    }

    void serializeNgrams(
        std::ostream& ostream,
        std::vector<WordIdT>& ngram,
        int32_t current_ngram_level,
        TrieNode<fl::str::UniChar, EntryProperties>* current_root_node
    ) noexcept {
        current_root_node->forEach(
            FLDIC_SEARCH_TERMINATION_TOKENS,
            [&ostream, &ngram, &current_ngram_level,
             this](auto uni_word, TrieNode<fl::str::UniChar, EntryProperties>* node) {
                ngram.resize(current_ngram_level);
                ngram[current_ngram_level - 1] = getWordId(uni_word);
                auto value = node->valueOrNull(dict_id_);
                if (value != nullptr) {
                    auto ngram_properties = value->ngramPropertiesOrNull();
                    if (ngram_properties != nullptr && current_ngram_level >= 2) {
                        if (!std::all_of(ngram.begin(), ngram.end(), [](auto id) { return id < 0; })) {
                            for (int i = 0; i < ngram.size(); i++) {
                                ostream << ngram[i];
                                if (i + 1 != ngram.size()) {
                                    ostream << FLDIC_LIST_SEPARATOR;
                                }
                            }
                            ostream << FLDIC_SEPARATOR
                                    << ngram_properties->absolute_score -
                                           global_penalties_[EntryType::ngram(current_ngram_level)]
                                    << FLDIC_NEWLINE;
                        }
                    }
                }

                auto next_ngram_root_node = node->findOrNull(FLDIC_TOKEN_NGRAM_SEPARATOR);
                if (next_ngram_root_node != nullptr) {
                    serializeNgrams(ostream, ngram, current_ngram_level + 1, next_ngram_root_node);
                }
            }
        );
    }

    void serializeShortcuts(std::ostream& ostream) noexcept {
        ostream << FLDIC_NEWLINE << FLDIC_SECTION_SHORTCUTS << FLDIC_NEWLINE;
    }

  public:
    [[nodiscard]]
    int32_t getWordId(std::span<const fl::str::UniChar> uni_word) const {
        if (isSpecialToken(uni_word)) {
            return convertSpecialTokenToId(uni_word);
        } else {
            auto value = data_->find(uni_word)->valueOrNull(dict_id_);
            if (value == nullptr) return -1;
            auto properties = value->wordPropertiesOrNull();
            return properties == nullptr ? -1 : properties->internal_id;
        }
    }

    static inline bool isSpecialToken(const fl::str::UniChar& token) noexcept {
        return token.size() == 1 && token[0] < FLDIC_TOKEN_ID_LIMIT;
    }

    static inline bool isSpecialToken(std::span<const fl::str::UniChar> token) noexcept {
        return token.size() == 1 && isSpecialToken(token[0]);
    }

    static inline bool isSpecialId(int32_t id) noexcept {
        return id < 0;
    }

    static inline int32_t convertSpecialTokenToId(std::span<const fl::str::UniChar> token) noexcept {
        return (-1) * static_cast<int32_t>(token[0][0]);
    }

    static inline fl::str::UniString convertSpecialIdToToken(int32_t id) noexcept {
        return {std::string(1, static_cast<char>((-1) * id))};
    }
};

} // namespace fl::nlp
