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
#include <mutex>
#include <shared_mutex>

export module fl.nlp.core.latin:dictionary;

import :algorithms;
export import :definitions;
import :entry_properties;
import fl.nlp.core.common;
import fl.string;
import fl.utils;

namespace fl::nlp {

export const auto FLDIC_SECTION_WORDS = "[words]";
export const auto FLDIC_SECTION_NGRAMS = "[ngrams]";
export const auto FLDIC_SECTION_SHORTCUTS = "[shortcuts]";

export const char FLDIC_FLAG_IS_POSSIBLY_OFFENSIVE = 'p';
export const char FLDIC_FLAG_IS_HIDDEN_BY_USER = 'h';

export enum class LatinDictionarySection {
    UNSPECIFIED,
    WORDS,
    NGRAMS,
    SHORTCUTS,
};

export class LatinDictionary : public Dictionary {
  public:
    LatinDictId dict_id_;
    // all operations on data_->first need to acquire lock on data_->second please.
    std::shared_ptr<std::pair<LatinTrieNode, std::shared_mutex>> data_;

    std::map<EntryType, ScoreT> total_scores_;
    std::map<EntryType, ScoreT> vocab_sizes_;
    std::map<EntryType, ScoreT> global_penalties_;

    LatinDictionary() = delete;
    LatinDictionary(LatinDictId id, std::shared_ptr<std::pair<LatinTrieNode, std::shared_mutex>> shared_data) : dict_id_(id), data_(shared_data) {};
    LatinDictionary(LatinDictId id) : dict_id_(id), data_(std::make_shared<std::pair<LatinTrieNode, std::shared_mutex>>()) {};
    LatinDictionary(const LatinDictionary&) = delete;
    LatinDictionary(LatinDictionary&&) = default;
    virtual ~LatinDictionary() = default;

    LatinDictionary& operator=(const LatinDictionary&) = delete;
    LatinDictionary& operator=(LatinDictionary&&) = default;

    inline LatinTrieNode* insertNgram(std::span<const fl::str::UniString> ngram) noexcept {
        std::scoped_lock<std::shared_mutex> lock(data_->second);
        return algorithms::insertNgram(&(data_->first), dict_id_, ngram);
    }

    inline void forEachEntryReadSafe(algorithms::EntryAction& action) {
        std::shared_lock<std::shared_mutex> lock(data_->second);
        algorithms::forEachEntry(&(data_->first), dict_id_, action);
    }

    inline void forEachWordReadSafe(algorithms::WordAction& action) {
        std::shared_lock<std::shared_mutex> lock(data_->second);
        algorithms::forEachWord(&(data_->first), dict_id_, action);
    }

    inline void forEachNgramReadSafe(algorithms::NgramAction& action) {
        std::shared_lock<std::shared_mutex> lock(data_->second);
        algorithms::forEachNgram(&(data_->first), dict_id_, action);
    }

    inline void forEachNgramReadSafe(int32_t ngram_size, algorithms::NgramAction& action) {
        std::shared_lock<std::shared_mutex> lock(data_->second);
        algorithms::forEachNgram(&(data_->first), dict_id_, ngram_size, action);
    }

    inline void forEachNgramReadSafe(int32_t min_ngram_size, int32_t max_ngram_size, algorithms::NgramAction& action) {
        std::shared_lock<std::shared_mutex> lock(data_->second);
        algorithms::forEachNgram(&(data_->first), dict_id_, min_ngram_size, max_ngram_size, action);
    }

    inline void forEachShortcutReadSafe(algorithms::ShortcutAction& action) {
        std::shared_lock<std::shared_mutex> lock(data_->second);
        algorithms::forEachShortcut(&(data_->first), dict_id_, action);
    }

    void recalculateAllFrequencyScores() noexcept {
        std::map<EntryType, ScoreT> new_total_scores;
        std::map<EntryType, ScoreT> new_vocab_sizes;

        forEachEntryReadSafe([&](auto ngram, auto ngram_size, auto* node, auto* value) {
            if (ngram_size == 1) {
                // Word
                if (auto properties = value->wordPropertiesOrNull(); properties != nullptr) {
                    auto type = EntryType::word();
                    properties->absolute_score = std::max(static_cast<ScoreT>(0), properties->absolute_score - global_penalties_[type]);
                    new_total_scores[type] += properties->absolute_score;
                    new_vocab_sizes[type]++;
                }
                // Shortcut
                if (auto properties = value->shortcutPropertiesOrNull(); properties != nullptr) {
                    auto type = EntryType::shortcut();
                    properties->absolute_score = std::max(static_cast<ScoreT>(0), properties->absolute_score - global_penalties_[type]);
                    new_total_scores[type] += properties->absolute_score;
                    new_vocab_sizes[type]++;
                }
            } else {
                // Ngram
                if (auto properties = value->ngramPropertiesOrNull(); properties != nullptr) {
                    auto type = EntryType::ngram(ngram_size);
                    properties->absolute_score = std::max(static_cast<ScoreT>(0), properties->absolute_score - global_penalties_[type]);
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
        const ScoreT penalty = global_penalties_[type];
        ScoreT total_score = 0;
        ScoreT vocab_size = 0;

        if (type.isWord()) {
            forEachWordReadSafe([&](auto word, auto* node, auto* properties) {
                properties->absolute_score = std::max(static_cast<ScoreT>(0), properties->absolute_score - penalty);
                total_score += properties->absolute_score;
                vocab_size++;
            });
        } else if (type.isNgram()) {
            forEachNgramReadSafe(type.ngramSize(), [&](auto ngram, auto type, auto* node, auto* properties) {
                properties->absolute_score = std::max(static_cast<ScoreT>(0), properties->absolute_score - penalty);
                total_score += properties->absolute_score;
                vocab_size++;
            });
        } else if (type.isShortcut()) {
            forEachShortcutReadSafe([&](auto shortcut, auto* node, auto* properties) {
                properties->absolute_score = std::max(static_cast<ScoreT>(0), properties->absolute_score - penalty);
                total_score += properties->absolute_score;
                vocab_size++;
            });
        }

        total_scores_[type] = total_score;
        vocab_sizes_[type] = vocab_size;
        global_penalties_[type] = 0;
    }

    [[nodiscard]]
    double calculateFrequency(EntryType type, ScoreT score, ScoreT k_offset) const noexcept {
        // TODO: this currently computes P(n-gram). We should instead compute P(n-gram | (n-1)-gram).
        const ScoreT N = fl::utils::findOrDefault(total_scores_, type, static_cast<ScoreT>(0));
        const ScoreT V = fl::utils::findOrDefault(vocab_sizes_, type, static_cast<ScoreT>(0));
        return static_cast<double>(score + k_offset) / std::max(static_cast<double>(N + k_offset * V), 1.0);
    }

  private:
    void deserializeContent(std::istream& istream) override {
        auto section = LatinDictionarySection::UNSPECIFIED;
        std::string line;
        std::vector<std::string> line_components;
        fl::str::UniString word;
        std::vector<std::string> id_components;
        std::vector<fl::str::UniString> ngram;
        std::map<WordIdT, fl::str::UniString> id_to_words_map;
        WordIdT current_word_id = 1;
        total_scores_.clear();
        vocab_sizes_.clear();

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
                std::scoped_lock<std::shared_mutex> lock(data_->second);
                auto node = data_->first.findOrCreate(word);
                auto properties = node->valueOrCreate(dict_id_)->wordPropertiesOrCreate();
                // Parse score
                properties->absolute_score = std::stoll(line_components[1]);
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
                // compute frequency scores
                auto type = EntryType::word();
                total_scores_[type] += properties->absolute_score;
                vocab_sizes_[type]++;
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
                properties->absolute_score = std::stoll(line_components[1]);
                auto type = EntryType::ngram(line_components.size());
                total_scores_[type] += properties->absolute_score;
                vocab_sizes_[type]++;
            } else if (section == LatinDictionarySection::SHORTCUTS) {
                fl::str::split(line, FLDIC_SEPARATOR, line_components);
                if (line_components.size() < 2) {
                    throw std::runtime_error("Invalid line!");
                }
                fl::str::toUniString(line_components[0], word);
                std::scoped_lock<std::shared_mutex> lock(data_->second);
                auto node = data_->first.findOrCreate(word);
                auto properties = node->valueOrCreate(dict_id_)->shortcutPropertiesOrCreate();
                properties->absolute_score = 1;
                properties->shortcut_phrase = line_components[1];
                auto type = EntryType::shortcut();
                total_scores_[type] += properties->absolute_score;
                vocab_sizes_[type]++;
            }
        }

        // This is already done as we go.
        // recalculateAllFrequencyScores();
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
        std::shared_lock<std::shared_mutex> lock(data_->second);
        data_->first.forEach(
            LATIN_SEARCH_TERMINATION_TOKENS,
            [&](std::span<const fl::str::UniChar> uni_word, LatinTrieNode* node) {
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
        std::shared_lock<std::shared_mutex> lock(data_->second);
        serializeNgrams(ostream, ngram, 1, &(data_->first));
    }

    void serializeNgrams(
        std::ostream& ostream,
        std::vector<WordIdT>& ngram,
        int32_t current_ngram_level,
        LatinTrieNode* current_root_node
    ) noexcept {
        current_root_node->forEach(
            LATIN_SEARCH_TERMINATION_TOKENS,
            [&ostream, &ngram, &current_ngram_level, this](auto uni_word, LatinTrieNode* node) {
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

                auto next_ngram_root_node = node->findOrNull(LATIN_TOKEN_NGRAM_SEPARATOR);
                if (next_ngram_root_node != nullptr) {
                    serializeNgrams(ostream, ngram, current_ngram_level + 1, next_ngram_root_node);
                }
            }
        );
    }

    void serializeShortcuts(std::ostream& ostream) noexcept {
        ostream << FLDIC_NEWLINE << FLDIC_SECTION_SHORTCUTS << FLDIC_NEWLINE;
        std::string raw_shortcut;
        std::shared_lock<std::shared_mutex> lock(data_->second);
        algorithms::forEachShortcut(&(data_->first), dict_id_, [&](auto shortcut, auto* node, auto* properties) {
            fl::str::toStdString(shortcut, raw_shortcut);
            ostream << raw_shortcut << FLDIC_SEPARATOR << properties->shortcut_phrase << FLDIC_NEWLINE;
        });
    }

  public:
    [[nodiscard]]
    int32_t getWordId(std::span<const fl::str::UniChar> uni_word) const {
        if (isSpecialToken(uni_word)) {
            return convertSpecialTokenToId(uni_word);
        } else {
            std::shared_lock<std::shared_mutex> lock(data_->second);
            auto value = data_->first.find(uni_word)->valueOrNull(dict_id_);
            if (value == nullptr) return -1;
            auto properties = value->wordPropertiesOrNull();
            return properties == nullptr ? -1 : properties->internal_id;
        }
    }
};

} // namespace fl::nlp
