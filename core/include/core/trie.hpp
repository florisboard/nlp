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

struct NgramProperties {
    score_t absolute_score : 24 = 0;
    bool is_possibly_offensive : 1 = false;
    bool is_hidden_by_user : 1 = false;
};

template<class ValueT>
requires std::is_copy_assignable_v<ValueT> && std::is_move_assignable_v<ValueT>
class BasicTrieNode {
  private:
    using NodeT = BasicTrieNode<ValueT>;

  public:
    BasicTrieNode() {};
    BasicTrieNode(const NodeT&) = delete;
    BasicTrieNode(NodeT&&) = delete;
    ~BasicTrieNode() = default;

    ValueT properties;
    bool is_terminal = false;
    std::map<fl::u8chstr, std::unique_ptr<NodeT>> children;

    void forEach(std::function<void(const fl::u8chstr_vec&, NodeT*)> action) noexcept {
        fl::u8chstr_vec empty_prefix;
        forEach(empty_prefix, action);
    }

    void forEach(const fl::u8chstr_vec& prefix, std::function<void(const fl::u8chstr_vec&, NodeT*)> action) noexcept {
        auto new_prefix = prefix;
        new_prefix.push_back(fl::u8str()); // Placeholder
        if (is_terminal) {
            action(prefix, this);
        }
        for (auto& [chstr, child_node] : children) {
            if (!isCtrlChar(chstr)) {
                new_prefix[new_prefix.size() - 1] = chstr;
                child_node->forEach(new_prefix, action);
            }
        }
    }

    NodeT* insert(const fl::u8chstr_vec& key) noexcept {
        auto node = resolveKeyOrCreate(key);
        node->properties = properties;
        return node;
    }

    const NodeT* resolveKey(const fl::u8chstr_vec& key) const noexcept {
        const NodeT* node = this;
        for (auto& chstr : key) {
            node = node->getChild(chstr);
            if (node == nullptr) return nullptr;
        }
        return node->is_terminal ? node : nullptr;
    }

    NodeT* resolveKey(const fl::u8chstr_vec& key) noexcept {
        NodeT* node = this;
        for (auto& chstr : key) {
            node = node->getChild(chstr);
            if (node == nullptr) return nullptr;
        }
        return node->is_terminal ? node : nullptr;
    }

    NodeT* resolveKeyOrCreate(const fl::u8chstr_vec& key) noexcept {
        NodeT* node = this;
        for (auto& chstr : key) {
            node = node->getChildOrCreate(chstr);
        }
        if (!node->is_terminal) {
            node->is_terminal = true;
        }
        return node;
    }

    const NodeT* subsequentWords() const noexcept {
        return _subsequent_words.get();
    }

    NodeT* subsequentWords() noexcept {
        return _subsequent_words.get();
    }

    NodeT* subsequentWordsOrCreate() noexcept {
        if (_subsequent_words == nullptr) {
            _subsequent_words = std::make_unique<NodeT>();
        }
        return _subsequent_words.get();
    }

  private:
    std::unique_ptr<NodeT> _subsequent_words = nullptr;

    constexpr bool isCtrlChar(const fl::u8chstr& chstr) {
        return false;
        // auto uch = static_cast<fl::u8uchar>(ch);
        // return uch < 0x20;
    }

    const NodeT* getChild(const fl::u8chstr& chstr) const noexcept {
        if (auto res = children.find(chstr); res != children.end()) {
            return (*res).second.get();
        } else {
            return nullptr;
        }
    }

    NodeT* getChild(const fl::u8chstr& chstr) noexcept {
        if (auto res = children.find(chstr); res != children.end()) {
            return (*res).second.get();
        } else {
            return nullptr;
        }
    }

    NodeT* getChildOrCreate(const fl::u8chstr& chstr) noexcept {
        auto ret_node = getChild(chstr);
        if (ret_node == nullptr) {
            auto node = std::make_unique<NodeT>();
            ret_node = node.get();
            children[chstr] = std::move(node);
        }
        return ret_node;
    }
};

using TrieNode = BasicTrieNode<NgramProperties>;

} // namespace fl::nlp

#endif
