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

#include <functional>
#include <span>
#include <string>
#include <vector>

export module fl.nlp.core.latin:algorithms;

import :definitions;
import :entry_properties;
import fl.nlp.core.common;
import fl.string;

namespace fl::nlp::algorithms {

inline namespace ranges {

export using WordAction =
    const std::function<void(std::span<const fl::str::UniChar>, LatinTrieNode*, WordEntryProperties*)>;
export using NgramAction =
    const std::function<void(std::span<const fl::str::UniString>, EntryType, LatinTrieNode*, NgramEntryProperties*)>;
export using ShortcutAction =
    const std::function<void(std::span<const fl::str::UniChar>, LatinTrieNode*, ShortcutEntryProperties*)>;
export using EntryAction =
    const std::function<void(std::span<const fl::str::UniString>, int32_t, LatinTrieNode*, EntryProperties*)>;

void forEachEntryInternal(
    LatinTrieNode* current_ngram_node,
    LatinDictId id,
    int32_t min_ngram_size,
    int32_t max_ngram_size,
    int32_t current_ngram_size,
    std::vector<fl::str::UniString>& buffer,
    EntryAction& action
) {
    if (min_ngram_size == 0 || max_ngram_size == 0) return;
    current_ngram_node->forEach(LATIN_SEARCH_TERMINATION_TOKENS, [&](auto word_span, auto* word_node) {
        auto* value = word_node->valueOrNull(id);
        if (value == nullptr) return;
        buffer.resize(current_ngram_size);
        buffer[current_ngram_size - 1].assign(word_span.begin(), word_span.end());
        if (min_ngram_size < 0 || max_ngram_size < 0 ||
            min_ngram_size <= current_ngram_size && current_ngram_size <= max_ngram_size) {
            action(buffer, current_ngram_size, word_node, value);
            if (current_ngram_size == max_ngram_size) return;
        }
        auto* next_ngram_node = word_node->findOrNull(LATIN_TOKEN_NGRAM_SEPARATOR);
        if (next_ngram_node == nullptr) return;
        forEachEntryInternal(
            next_ngram_node, id, min_ngram_size, max_ngram_size, current_ngram_size + 1, buffer, action
        );
    });
}

export inline void forEachEntry(LatinTrieNode* data, LatinDictId id, EntryAction& action) {
    std::vector<fl::str::UniString> buffer;
    forEachEntryInternal(data, id, -1, -1, 1, buffer, action);
}

} // namespace ranges

inline namespace words {

export inline void forEachWord(LatinTrieNode* data, LatinDictId id, WordAction& action) {
    data->forEach(LATIN_SEARCH_TERMINATION_TOKENS, [&](auto word, auto* node) {
        auto* value = node->valueOrNull(id);
        if (value == nullptr) return;
        auto* properties = value->wordPropertiesOrNull();
        if (properties == nullptr) return;
        action(word, node, properties);
    });
}

} // namespace words

inline namespace ngrams {

export inline void forEachNgram(
    LatinTrieNode* data, LatinDictId id, int32_t min_ngram_size, int32_t max_ngram_size, NgramAction& action
) {
    std::vector<fl::str::UniString> buffer;
    forEachEntryInternal(
        data, id, min_ngram_size, max_ngram_size, 1, buffer,
        [&](auto ngram, auto ngram_size, auto* node, auto* value) {
            auto properties = value->ngramPropertiesOrNull();
            if (properties == nullptr) return;
            action(ngram, EntryType::ngram(ngram_size), node, properties);
        }
    );
}

export inline void forEachNgram(LatinTrieNode* data, LatinDictId id, NgramAction& action) {
    forEachNgram(data, id, -1, -1, action);
}

export inline void forEachNgram(LatinTrieNode* data, LatinDictId id, int32_t desired_ngram_size, NgramAction& action) {
    forEachNgram(data, id, desired_ngram_size, desired_ngram_size, action);
}

export LatinTrieNode* insertNgram(
    LatinTrieNode* data, LatinDictId id, std::span<const fl::str::UniString> ngram
) noexcept {
    auto* ngram_node = data;
    for (int i = 0; i < ngram.size(); i++) {
        auto& word = ngram[i];
        auto* word_node = ngram_node->findOrCreate(word);
        auto* word_value = word_node->valueOrCreate(id);
        if (i + 1 != ngram.size()) {
            ngram_node = word_node->findOrCreate(LATIN_TOKEN_NGRAM_SEPARATOR);
        } else {
            word_value->ngramPropertiesOrCreate();
            return word_node;
        }
    }
    return nullptr;
}

export LatinTrieNode* findNgramOrNull(
    LatinTrieNode* data, LatinDictId id, std::span<const fl::str::UniString> ngram
) noexcept {
    auto* ngram_node = data;
    for (int i = 0; i < ngram.size(); i++) {
        auto& word = ngram[i];
        auto* word_node = ngram_node->findOrNull(word);
        if (word_node == nullptr) return nullptr;
        if (i + 1 != ngram.size()) {
            ngram_node = word_node->findOrNull(LATIN_TOKEN_NGRAM_SEPARATOR);
            if (ngram_node == nullptr) return nullptr;
        } else if (id < 0 || word_node->valueOrNull(id) != nullptr) {
            return word_node;
        }
    }
    return nullptr;
}

export LatinTrieNode* findNgram(LatinTrieNode* data, LatinDictId id, std::span<const fl::str::UniString> ngram) {
    auto* ngram_node = findNgramOrNull(data, id, ngram);
    if (ngram_node == nullptr) {
        throw std::out_of_range("No such ngram exists");
    }
    return ngram_node;
}

} // namespace ngrams

inline namespace shortcuts {

export inline void forEachShortcut(LatinTrieNode* data, LatinDictId id, ShortcutAction& action) {
    data->forEach(LATIN_SEARCH_TERMINATION_TOKENS, [&](auto word, auto* node) {
        auto* value = node->valueOrNull(id);
        if (value == nullptr) return;
        auto* properties = value->shortcutPropertiesOrNull();
        if (properties == nullptr) return;
        action(word, node, properties);
    });
}

} // namespace shortcuts

inline namespace other {

export fl::str::UniString wordAt(const LatinTrieNode* end_node) {
    fl::str::UniString word;
    if (end_node == nullptr) {
        return word;
    }

    auto* node = end_node;
    while (node->parent_ != nullptr) {
        if (isSpecialToken(node->key_)) {
            break;
        }
        word.insert(word.begin(), node->key_);
        node = node->parent_;
    }
    return word;
}

export void writeSuggestionResults(
    const TransientSuggestionResults<LatinTrieNode>& transient_results,
    SuggestionResults& results,
    const SuggestionRequestFlags& flags
) {
    std::string raw_text;
    for (const auto& candidate : transient_results.get()) {
        auto text = wordAt(candidate->node_);
        fl::str::toStdString(text, raw_text);
        // TODO: evaluate if it is better if this post-casemapping is better done in Kotlin
        auto iss_start = flags.inputShiftStateStart();
        auto iss_curr = flags.inputShiftStateCurrent();
        bool is_caps = iss_curr == InputShiftState::CAPS_LOCK;
        bool is_shifted = iss_start != InputShiftState::UNSHIFTED;
        if (is_caps) {
            fl::str::uppercase(raw_text);
        } else if (is_shifted) {
            // TODO: is titlecase correct here?? (edge cases like custom words that look like this: LaTeX or ePaper)
            fl::str::titlecase(raw_text);
        }
        auto result = std::make_unique<SuggestionCandidate>(
            raw_text, "", candidate->confidence_, candidate->is_eligible_for_auto_commit_,
            candidate->is_eligible_for_user_removal_
        );
        results.push_back(std::move(result));
    }
}

} // namespace other

} // namespace fl::nlp::algorithms
