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

#include <unicode/brkiter.h>
#include <unicode/locid.h>

#include <functional>

export module fl.icuext:brkiter;

namespace fl::icuext {

export struct BreakIteratorCache {
    std::unique_ptr<icu::BreakIterator> sentence_iterator = nullptr;
    std::unique_ptr<icu::BreakIterator> word_iterator = nullptr;
    std::unique_ptr<icu::BreakIterator> character_iterator = nullptr;

    inline icu::BreakIterator* sentence() const noexcept {
        return sentence_iterator.get();
    }

    inline icu::BreakIterator* word() const noexcept {
        return word_iterator.get();
    }

    inline icu::BreakIterator* character() const noexcept {
        return character_iterator.get();
    }

    void init(const icu::Locale& locale, UErrorCode& status) noexcept {
        reset();

        sentence_iterator = //
            std::unique_ptr<icu::BreakIterator>(icu::BreakIterator::createSentenceInstance(locale, status));
        word_iterator = //
            std::unique_ptr<icu::BreakIterator>(icu::BreakIterator::createWordInstance(locale, status));
        character_iterator = //
            std::unique_ptr<icu::BreakIterator>(icu::BreakIterator::createCharacterInstance(locale, status));
    }

    void reset() noexcept {
        sentence_iterator.reset();
        word_iterator.reset();
        character_iterator.reset();
    }
};

export void forEach(icu::BreakIterator* bi, const std::function<void(int32_t, int32_t)>& action) {
    int32_t current = bi->first();
    int32_t next;
    while ((next = bi->next()) != icu::BreakIterator::DONE) {
        action(current, next);
        current = next;
    }
}

} // namespace fl::icuext
