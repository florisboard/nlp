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

#include <memory>
#include <string>

export module fl.nlp.core.latin:entry_properties;

namespace fl::nlp {

export using WordIdT = int32_t;

export struct EntryType {
  public:
    using ValueT = int32_t;

    ValueT value_;

    inline constexpr bool isWord() const noexcept {
        return value_ == 1;
    }

    inline constexpr bool isNgram() const noexcept {
        return value_ > 1;
    }

    inline constexpr bool isNgram(ValueT size) const noexcept {
        return value_ == size;
    }

    inline constexpr bool isShortcut() const noexcept {
        return value_ == -1;
    }

    inline constexpr ValueT ngramSize() const noexcept {
        return std::max(1, value_);
    }

    bool operator==(const EntryType& other) const noexcept {
        return value_ == other.value_;
    }

  public:
    static inline constexpr EntryType word() noexcept {
        return {1};
    }

    static inline constexpr EntryType ngram(ValueT size) noexcept {
        return {std::max(2, size)};
    }

    static inline constexpr EntryType shortcut() noexcept {
        return {-1};
    }
};

export struct WordEntryProperties {
    WordIdT internal_id = 0;
    int32_t absolute_score = 0;
    bool is_possibly_offensive = false;
    bool is_hidden_by_user = false;
};

export struct NgramEntryProperties {
    int32_t absolute_score = 0;
};

export struct ShortcutEntryProperties {
    std::string shortcut_phrase;
    int32_t absolute_score;
    bool is_possibly_offensive = false;
    bool is_hidden_by_user = false;
};

export struct EntryProperties {
  private:
    std::unique_ptr<WordEntryProperties> word_properties = nullptr;
    std::unique_ptr<NgramEntryProperties> ngram_properties = nullptr;
    std::unique_ptr<ShortcutEntryProperties> shortcut_properties = nullptr;

  public:
    WordEntryProperties* wordProperties() {
        auto p = word_properties.get();
        if (p == nullptr) {
            throw std::runtime_error("Word properties is not initialized");
        }
        return p;
    }

    inline WordEntryProperties* wordPropertiesOrNull() noexcept {
        return word_properties.get();
    }

    WordEntryProperties* wordPropertiesOrCreate() {
        if (word_properties == nullptr) {
            word_properties = std::make_unique<WordEntryProperties>();
        }
        return word_properties.get();
    }

    NgramEntryProperties* ngramProperties() {
        auto p = ngram_properties.get();
        if (p == nullptr) {
            throw std::runtime_error("Ngram properties is not initialized");
        }
        return p;
    }

    inline NgramEntryProperties* ngramPropertiesOrNull() noexcept {
        return ngram_properties.get();
    }

    NgramEntryProperties* ngramPropertiesOrCreate() {
        if (ngram_properties == nullptr) {
            ngram_properties = std::make_unique<NgramEntryProperties>();
        }
        return ngram_properties.get();
    }

    ShortcutEntryProperties* shortcutProperties() {
        auto p = shortcut_properties.get();
        if (p == nullptr) {
            throw std::runtime_error("Shortcut properties is not initialized");
        }
        return p;
    }

    inline ShortcutEntryProperties* shortcutPropertiesOrNull() noexcept {
        return shortcut_properties.get();
    }

    ShortcutEntryProperties* shortcutPropertiesOrCreate() {
        if (shortcut_properties == nullptr) {
            shortcut_properties = std::make_unique<ShortcutEntryProperties>();
        }
        return shortcut_properties.get();
    }
};

} // namespace fl::nlp

namespace std {

export template<>
struct hash<fl::nlp::EntryType> {
    std::size_t operator()(const fl::nlp::EntryType& entry) const noexcept {
        return std::hash<fl::nlp::EntryType::ValueT>()(entry.value_);
    }
};

template<>
struct less<fl::nlp::EntryType> {
    bool operator()(const fl::nlp::EntryType& lhs, const fl::nlp::EntryType& rhs) const noexcept {
        return lhs.value_ < rhs.value_;
    }
};

} // namespace std
