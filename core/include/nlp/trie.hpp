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

#ifndef __FLORISNLP_CORE_NLP_TRIE_H__
#define __FLORISNLP_CORE_NLP_TRIE_H__

#include "nlp/common.hpp"

#include <array>
#include <memory>
#include <string>
#include <type_traits>

namespace floris::nlp {

struct ngram_properties {
    score_t absolute_score = 0;
    bool is_possibly_offensive = false;
    bool is_hidden_by_user = false;
};

struct basic_trie_node {
  private:
    using CharT = char;
    using UCharT = size_t;
    using StrT = std::basic_string<CharT>;
    using NodeT = basic_trie_node;
    using ValueT = ngram_properties;

    static const UCharT SEPARATOR_ID = 0x1F;
    static const size_t ALPHABET_SIZE = 256;

  public:
    basic_trie_node() {};
    basic_trie_node(const NodeT&) = delete;
    basic_trie_node(NodeT&&) = delete;
    ~basic_trie_node() = default;

    ValueT properties;
    bool is_terminal = false;
    std::array<std::unique_ptr<NodeT>, ALPHABET_SIZE> children = { nullptr };

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
        for (UCharT uch = 0; uch < ALPHABET_SIZE; uch++) {
            if (!is_ctrl_char(uch) && children[uch] != nullptr) {
                new_prefix[new_prefix.size() - 1] = static_cast<CharT>(uch);
                children[uch]->for_each(new_prefix, action);
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
        return get_child_or_null(SEPARATOR_ID);
    }

    NodeT* subsequent_words_or_create() noexcept {
        return get_child_or_create(SEPARATOR_ID);
    }

  private:
    constexpr bool is_ctrl_char(UCharT uch) {
        return uch < 0x20;
    }

    constexpr bool is_ctrl_char(CharT ch) {
        return ch < 0x20;
    }

    NodeT* get_child_or_null(UCharT ch) noexcept {
        return children[ch].get();
    }

    NodeT* get_child_or_create(UCharT ch) noexcept {
        auto ret_node = get_child_or_null(ch);
        if (ret_node == nullptr) {
            auto node = std::make_unique<NodeT>();
            ret_node = node.get();
            children[ch] = std::move(node);
        }
        return ret_node;
    }

    NodeT* resolve_key_or_create(const StrT& key) noexcept {
        NodeT* node = this;
        for (CharT ch : key) {
            node = node->get_child_or_create(static_cast<UCharT>(ch));
        }
        if (!node->is_terminal) {
            node->is_terminal = true;
        }
        return node;
    }
};

} // namespace floris::nlp

#endif
