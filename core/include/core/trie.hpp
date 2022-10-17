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

#ifndef __FLORISNLP_CORE_TRIE_H__
#define __FLORISNLP_CORE_TRIE_H__

#include "core/common.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace fl::nlp {

struct ngram_properties {
    score_t absolute_score = 0;
    bool is_possibly_offensive = false;
    bool is_hidden_by_user = false;
};

template<typename V>
class alphabet_array {
  private:
    static const std::size_t IDX_OFFSET = 32; // 0x00-0x1F control characters are not supported to save memory
    static const std::size_t SIZE = 256 - IDX_OFFSET;

    using PtrT = std::unique_ptr<V>;

  public:
    explicit alphabet_array() : data() {
        std::fill_n(data, SIZE, nullptr);
    };
    ~alphabet_array() = default;

    constexpr const PtrT& operator[](char ch) const {
        return at(to_index(ch));
    }

    constexpr PtrT& operator[](char ch) {
        return at(to_index(ch));
    }

    constexpr const PtrT& at(std::size_t idx) const {
        return idx >= 0 && idx < SIZE ? data[idx] : throw std::out_of_range("");
    }

    constexpr PtrT& at(std::size_t idx) {
        return idx >= 0 && idx < SIZE ? data[idx] : throw std::out_of_range("");
    }

    constexpr std::size_t size() const noexcept {
        return SIZE;
    }

  private:
    PtrT data[SIZE];

    static constexpr bool validate_uchar(unsigned char uch) noexcept {
        return uch >= 0x20 && uch <= 0xFF;
    }

  public:
    static constexpr std::size_t to_index(char ch) {
        auto uch = static_cast<unsigned char>(ch);
        return validate_uchar(uch) ? (static_cast<std::size_t>(uch) - IDX_OFFSET) : throw std::out_of_range("");
    }

    static constexpr char to_char(std::size_t idx) {
        auto uch = static_cast<unsigned char>(idx + IDX_OFFSET);
        return validate_uchar(uch) ? static_cast<char>(uch) : throw std::out_of_range("");
    }
};

class basic_trie_node {
  private:
    using StrT = std::string;
    using NodeT = basic_trie_node;
    using ValueT = ngram_properties;

  public:
    basic_trie_node() {};
    basic_trie_node(const NodeT&) = delete;
    basic_trie_node(NodeT&&) = delete;
    ~basic_trie_node() = default;

    ValueT properties;
    bool is_terminal = false;
    alphabet_array<NodeT> children;

    void for_each(std::function<void(const StrT&, NodeT*)> action) noexcept {
        StrT empty_prefix;
        for_each(empty_prefix, action);
    }

    void for_each(const StrT& prefix, std::function<void(const StrT&, NodeT*)> action) noexcept {
        StrT new_prefix = prefix;
        new_prefix += ' '; // Placeholder
        if (is_terminal) {
            action(prefix, this);
        }
        for (std::size_t n = 0; n < children.size(); n++) {
            auto ch = children.to_char(n);
            if (!is_ctrl_char(ch) && children.at(n) != nullptr) {
                new_prefix[new_prefix.size() - 1] = ch;
                children.at(n)->for_each(new_prefix, action);
            }
        }
    }

    NodeT* insert(const StrT& key, const ValueT& properties) noexcept {
        auto node = resolve_key_or_create(key);
        node->properties = properties;
        return node;
    }

    NodeT* insert(const StrT& key, ValueT&& properties) noexcept {
        auto node = resolve_key_or_create(key);
        node->properties = std::move(properties);
        return node;
    }

    NodeT* subsequent_words_or_null() noexcept {
        return subsequent_words.get();
    }

    NodeT* subsequent_words_or_create() noexcept {
        if (subsequent_words != nullptr) {
            subsequent_words = std::make_unique<NodeT>();
        }
        return subsequent_words.get();
    }

    void fuzzy_search(const StrT& word, int max_cost, std::function<void(const StrT&, const NodeT*, int)> on_result)
        const {
        std::vector<std::vector<int>> distances;
        std::string prefix_buffer;
        fuzzy_search_recursive(word, prefix_buffer, 0, distances, max_cost, on_result);
    }

  private:
    std::unique_ptr<NodeT> subsequent_words = nullptr;

    constexpr bool is_ctrl_char(char ch) {
        return ch < 0x20;
    }

    const NodeT* get_child_or_null_const(char ch) const noexcept {
        return children[ch].get();
    }

    NodeT* get_child_or_null(char ch) noexcept {
        return children[ch].get();
    }

    NodeT* get_child_or_create(char ch) noexcept {
        auto ret_node = get_child_or_null(ch);
        if (ret_node == nullptr) {
            auto node = std::make_unique<NodeT>();
            ret_node = node.get();
            children[ch] = std::move(node);
        }
        return ret_node;
    }

  public:
    const NodeT* resolve_key_or_null_const(const StrT& key) const noexcept {
        const NodeT* node = this;
        for (char ch : key) {
            node = node->get_child_or_null_const(ch);
            if (node == nullptr) return nullptr;
        }
        return node;
    }

    NodeT* resolve_key_or_null(const StrT& key) noexcept {
        NodeT* node = this;
        for (char ch : key) {
            node = node->get_child_or_null(ch);
            if (node == nullptr) return nullptr;
        }
        return node;
    }

    NodeT* resolve_key_or_create(const StrT& key) noexcept {
        NodeT* node = this;
        for (char ch : key) {
            node = node->get_child_or_create(ch);
        }
        if (!node->is_terminal) {
            node->is_terminal = true;
        }
        return node;
    }

  private:
    void fuzzy_search_recursive(
        const StrT& word,
        StrT& prefix_buffer,
        int prefix_length,
        std::vector<std::vector<int>>& distances,
        int max_cost,
        std::function<void(const StrT&, const NodeT*, int)> on_result
    ) const {
        while (prefix_length >= distances.size()) {
            distances.push_back(std::vector(word.length() + 1, 0));
        }

        if (prefix_length == 0) {
            for (int i = 0; i <= word.length(); i++) {
                distances[0][i] = i;
            }
        } else {
            distances[prefix_length][0] = prefix_length;
            for (int i = 1; i <= word.length(); i++) {
                int substitution_cost;
                if (prefix_buffer[prefix_length - 1] == word[i - 1]) {
                    substitution_cost = 0;
                } else {
                    substitution_cost = 1;
                }
                distances[prefix_length][i] = std::min(
                    std::min(distances[prefix_length - 1][i] + 1, distances[prefix_length][i - 1] + 1),
                    distances[prefix_length - 1][i - 1] + substitution_cost
                );
            }
        }

        auto cost = distances[prefix_length][word.length()];
        if (prefix_length > word.length() && cost > max_cost) {
            return;
        } else if (cost <= max_cost && is_terminal) {
            on_result(prefix_buffer.substr(0, prefix_length), this, cost);
        }

        if (prefix_length == prefix_buffer.length()) {
            prefix_buffer.push_back('?');
        }
        for (std::size_t n = 0; n < children.size(); n++) {
            if (children.at(n) != nullptr) {
                prefix_buffer[prefix_length] = children.to_char(n);
                children.at(n)->fuzzy_search_recursive(
                    word, prefix_buffer, prefix_length + 1, distances, max_cost, on_result
                );
            }
        }
    }
};

} // namespace fl::nlp

#endif
