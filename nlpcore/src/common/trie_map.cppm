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

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

export module fl.nlp.core.common:trie_map;

namespace fl::nlp {

export template<typename KeyT, typename ValueT, typename ValueIdT>
requires std::is_default_constructible_v<ValueT> && std::is_integral_v<ValueIdT>
struct TrieNode {
  private:
    using NodeT = TrieNode<KeyT, ValueT, ValueIdT>;
    using NodeActionWithoutKeyT = const std::function<void(NodeT*)>;
    using NodeActionWithKeyT = const std::function<void(std::span<const KeyT>, NodeT*)>;

  public:
    KeyT key_;
    std::map<ValueIdT, ValueT> values_;
    std::vector<std::unique_ptr<NodeT>> children_;
    NodeT* parent_ = nullptr;

    TrieNode() = default;
    TrieNode(const TrieNode&) = delete;
    TrieNode(TrieNode&&) noexcept = default;
    ~TrieNode() = default;

    TrieNode& operator=(const TrieNode&) = delete;
    TrieNode& operator=(TrieNode&&) noexcept = default;

    [[nodiscard]]
    inline NodeT* find(const KeyT& key) {
        auto* child = findOrNull(key);
        if (child != nullptr) {
            return child;
        }
        throw std::out_of_range("No such key");
    }

    [[nodiscard]]
    NodeT* find(std::span<const KeyT> key_span) {
        auto* node = this;
        for (const auto& key : key_span) {
            node = node->find(key);
        }
        return node;
    }

    [[nodiscard]]
    inline NodeT* findOrNull(const KeyT& key) noexcept {
        auto it = std::find_if(children_.begin(), children_.end(), [&](auto& node) { return node->key_ == key; });
        if (it != children_.end()) {
            return it->get();
        } else {
            return nullptr;
        }
    }

    [[nodiscard]]
    NodeT* findOrNull(std::span<const KeyT> key_span) noexcept {
        auto* node = this;
        for (const auto& key : key_span) {
            node = node->findOrNull(key);
            if (node == nullptr) {
                return nullptr;
            }
        }
        return node;
    }

    [[nodiscard]]
    inline NodeT* findOrCreate(const KeyT& key) {
        auto* child = findOrNull(key);
        if (child != nullptr) {
            return child;
        }
        auto new_child = std::make_unique<NodeT>();
        new_child->key_ = key;
        new_child->parent_ = this;
        auto* ret_pointer = new_child.get();
        children_.push_back(std::move(new_child));
        return ret_pointer;
    }

    [[nodiscard]]
    NodeT* findOrCreate(std::span<const KeyT> key_span) {
        auto* node = this;
        for (const auto& key : key_span) {
            node = node->findOrCreate(key);
        }
        return node;
    }

    [[nodiscard]]
    inline ValueT* value(ValueIdT id) {
        return &values_.at(id);
    }

    [[nodiscard]]
    inline ValueT* valueOrNull(ValueIdT id) noexcept {
        auto it = values_.find(id);
        if (it != values_.end()) {
            return &(it->second);
        } else {
            return nullptr;
        }
    }

    [[nodiscard]]
    inline ValueT* valueOrCreate(ValueIdT id) {
        return &values_[id];
    }

    [[nodiscard]]
    inline bool isEndNode() const noexcept {
        return !values_.empty();
    }

    [[nodiscard]]
    inline bool isEndNode(ValueIdT id) const noexcept {
        return values_.find(id) != values_.end();
    }

    void forEach(std::span<const KeyT> termination_tokens, NodeActionWithoutKeyT& action) noexcept {
        std::vector<KeyT> word_cache;
        forEach(word_cache, 0, termination_tokens, action);
    }

    void forEach(std::span<const KeyT> termination_tokens, NodeActionWithKeyT& action) noexcept {
        std::vector<KeyT> word_cache;
        forEach(word_cache, 0, termination_tokens, action);
    }

    void forEach(NodeActionWithoutKeyT& action) noexcept {
        forEach({}, action);
    }

    void forEach(NodeActionWithKeyT& action) noexcept {
        forEach({}, action);
    }

  private:
    void forEach(std::span<const KeyT> termination_tokens, NodeActionWithoutKeyT& action) const noexcept {
        for (const auto& child_node : children_) {
            if (std::find(termination_tokens.begin(), termination_tokens.end(), child_node->key_) !=
                termination_tokens.end()) {
                continue;
            }
            if (child_node->isEndNode()) {
                action(child_node.get());
            }
            child_node->forEach(termination_tokens, action);
        }
    }

    void forEach(
        std::vector<KeyT>& word_cache,
        size_t insert_index,
        std::span<const KeyT> termination_tokens,
        NodeActionWithKeyT& action
    ) const noexcept {
        for (const auto& child_node : children_) {
            word_cache.resize(insert_index + 1);
            if (std::find(termination_tokens.begin(), termination_tokens.end(), child_node->key_) !=
                termination_tokens.end()) {
                continue;
            }
            word_cache[insert_index] = child_node->key_;
            if (child_node->isEndNode()) {
                action(word_cache, child_node.get());
            }
            child_node->forEach(word_cache, insert_index + 1, termination_tokens, action);
        }
    }
};

} // namespace fl::nlp
