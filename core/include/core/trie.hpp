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
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace fl::nlp {

struct ngram_properties {
    score_t absolute_score : 24 = 0;
    bool is_possibly_offensive : 1 = false;
    bool is_hidden_by_user : 1 = false;
};

template<class ValueT>
requires std::is_copy_assignable_v<ValueT> && std::is_move_assignable_v<ValueT>
class basic_trie_node {
  private:
    using NodeT = basic_trie_node<ValueT>;

  public:
    basic_trie_node() {};
    basic_trie_node(const NodeT&) = delete;
    basic_trie_node(NodeT&&) = delete;
    ~basic_trie_node() = default;

    ValueT properties;
    bool is_terminal = false;
    std::map<fl::u8chstr, std::unique_ptr<NodeT>> children;

    void for_each(std::function<void(const fl::u8chstr_vec&, NodeT*)> action) noexcept {
        fl::u8chstr_vec empty_prefix;
        for_each(empty_prefix, action);
    }

    void for_each(const fl::u8chstr_vec& prefix, std::function<void(const fl::u8chstr_vec&, NodeT*)> action) noexcept {
        auto new_prefix = prefix;
        new_prefix.push_back(fl::u8str()); // Placeholder
        if (is_terminal) {
            action(prefix, this);
        }
        for (auto& [chstr, child_node] : children) {
            if (!is_ctrl_char(chstr)) {
                new_prefix[new_prefix.size() - 1] = chstr;
                child_node->for_each(new_prefix, action);
            }
        }
    }

    NodeT* insert(const fl::u8chstr_vec& key) noexcept {
        auto node = resolve_key_or_create(key);
        node->properties = properties;
        return node;
    }

    const NodeT* resolve_key(const fl::u8chstr_vec& key) const noexcept {
        const NodeT* node = this;
        for (auto& chstr : key) {
            node = node->get_child(chstr);
            if (node == nullptr) return nullptr;
        }
        return node->is_terminal ? node : nullptr;
    }

    NodeT* resolve_key(const fl::u8chstr_vec& key) noexcept {
        NodeT* node = this;
        for (auto& chstr : key) {
            node = node->get_child(chstr);
            if (node == nullptr) return nullptr;
        }
        return node->is_terminal ? node : nullptr;
    }

    NodeT* resolve_key_or_create(const fl::u8chstr_vec& key) noexcept {
        NodeT* node = this;
        for (auto& chstr : key) {
            node = node->get_child_or_create(chstr);
        }
        if (!node->is_terminal) {
            node->is_terminal = true;
        }
        return node;
    }

    const NodeT* subsequent_words() const noexcept { return _subsequent_words.get(); }

    NodeT* subsequent_words() noexcept { return _subsequent_words.get(); }

    NodeT* subsequent_words_or_create() noexcept {
        if (_subsequent_words == nullptr) {
            _subsequent_words = std::make_unique<NodeT>();
        }
        return _subsequent_words.get();
    }

  private:
    std::unique_ptr<NodeT> _subsequent_words = nullptr;

    constexpr bool is_ctrl_char(const fl::u8chstr& chstr) {
        return false;
        // auto uch = static_cast<fl::u8uchar>(ch);
        // return uch < 0x20;
    }

    const NodeT* get_child(const fl::u8chstr& chstr) const noexcept {
        if (auto res = children.find(chstr); res != children.end()) {
            return (*res).second.get();
        } else {
            return nullptr;
        }
    }

    NodeT* get_child(const fl::u8chstr& chstr) noexcept {
        if (auto res = children.find(chstr); res != children.end()) {
            return (*res).second.get();
        } else {
            return nullptr;
        }
    }

    NodeT* get_child_or_create(const fl::u8chstr& chstr) noexcept {
        auto ret_node = get_child(chstr);
        if (ret_node == nullptr) {
            auto node = std::make_unique<NodeT>();
            ret_node = node.get();
            children[chstr] = std::move(node);
        }
        return ret_node;
    }
};

using trie_node = basic_trie_node<ngram_properties>;

} // namespace fl::nlp

#endif
