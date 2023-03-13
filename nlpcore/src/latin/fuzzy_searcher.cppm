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
#include <set>
#include <span>

export module fl.nlp.core.latin:fuzzy_searcher;

import :dictionary;
import :nlp_session_config;
import :nlp_session_state;
import fl.nlp.core.common;
import fl.nlp.string;

namespace fl::nlp {

// TODO: standardize this definition
const fl::str::UniString START_OF_SENTENCE_TOKEN = {std::string(1, 2)};

enum class FuzzySearchType {
    Proximity,
    ProximityWithoutSelf,
    ProximityOrPrefix,
};

class LatinFuzzySearcher {
  private:
    template<class P>
    using OnResultLambda = const std::function<void(fl::str::UniString&, int, P&)>;

  private:
    const LatinNlpSessionConfig* session_config;
    const LatinNlpSessionState* session_state;

  public:
    LatinFuzzySearcher() = delete;
    explicit LatinFuzzySearcher(
        const LatinNlpSessionConfig* session_config_, const LatinNlpSessionState* session_state_
    )
        : session_config(session_config_), session_state(session_state_) {}

    void predictWord(
        std::span<fl::str::UniString> sentence, const SuggestionRequestFlags& flags, SuggestionResults& results
    ) noexcept {
        if (sentence.empty()) {
            return;
        }
        auto max_ngram_level = std::max(1, std::min(flags.maxNgramLevel(), static_cast<int>(sentence.size())));
        for (int ngram_level = 1; ngram_level <= max_ngram_level; ngram_level++) {
            if (ngram_level == 1) {
                // We have a uni-gram and only search for proximate words
                if (sentence.back().empty()) continue;
                std::string word;
                fuzzySearchWord(
                    sentence.back(), FuzzySearchType::ProximityOrPrefix,
                    [&](fl::str::UniString& uni_word, int cost, WordProperties& properties) {
                        fl::str::toStdString(uni_word, word);
                        double confidence = 1.0 - (cost / 6.0);
                        results.insert({word, "", confidence}, flags);
                    }
                );
            } else {
                // We have an n-gram
                auto ngram = sentence.subspan(sentence.size() - ngram_level, ngram_level);
            }
        }
    }

  private:
    void fuzzySearchWord(
        const fl::str::UniString& word, FuzzySearchType type, OnResultLambda<WordProperties>& on_result
    ) noexcept {
        fl::str::UniString prefix;
        fuzzySearchWordRecursiveDld(word, type, prefix, 0, on_result);
    }

    void fuzzySearchWordRecursiveDld(
        const fl::str::UniString& word,
        FuzzySearchType type,
        fl::str::UniString& prefix,
        int prefix_pos,
        OnResultLambda<WordProperties>& on_result
    ) noexcept {
        prefix.resize(prefix_pos);
        auto children = fetchChildrenForPrefix(prefix);

        if (fl::str::startsWith(prefix, word)) {
            auto properties = fetchWordPropertiesForPrefix(prefix);
            on_result(prefix, 0, properties);
        }

        for (auto& child : children) {
            prefix.resize(prefix_pos + 1);
            prefix[prefix_pos] = child;
            fuzzySearchWordRecursiveDld(word, type, prefix, prefix_pos + 1, on_result);
        }
    }

    void fuzzySearchNgram(
        std::span<const fl::str::UniString> ngram, FuzzySearchType type, OnResultLambda<NgramProperties>& on_result
    ) noexcept {
        //
    }

    WordProperties fetchWordPropertiesForPrefix(fl::str::UniString& prefix) noexcept {
        WordProperties combined_properties;
        session_state->forEachDictionary([&](const LatinDictionary& dict) {
            auto node = dict.words.getOrNull(prefix);
            if (node != nullptr) {
                combined_properties.absolute_score = node->value.absolute_score - dict.global_penalty_words;
                combined_properties.is_possibly_offensive =
                    combined_properties.is_possibly_offensive || node->value.is_possibly_offensive;
                combined_properties.is_hidden_by_user =
                    combined_properties.is_hidden_by_user || node->value.is_hidden_by_user;
            }
        });
        return combined_properties;
    }

    std::set<fl::str::UniChar> fetchChildrenForPrefix(fl::str::UniString& prefix) noexcept {
        std::set<fl::str::UniChar> combined_child_keys;
        session_state->forEachDictionary([&](const LatinDictionary& dict) {
            auto node = dict.words.getOrNull(prefix);
            if (node != nullptr) {
                for (auto& [key, _] : node->children) {
                    combined_child_keys.emplace(key);
                }
            }
        });
        return combined_child_keys;
    }
};

} // namespace fl::nlp
