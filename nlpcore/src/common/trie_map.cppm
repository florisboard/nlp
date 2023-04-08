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

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <type_traits>

export module fl.nlp.core.common:trie_map;

namespace fl::nlp {

export template<typename KeyT, typename ValueT, typename ValueIdT = std::uint8_t>
requires std::is_default_constructible_v<ValueT> && std::is_integral_v<ValueIdT>
struct TrieNode {
  private:
    using NodeT = TrieNode<KeyT, ValueT, ValueIdT>;
    using NodeActionT = const std::function<void(std::span<const KeyT>, NodeT*)>;

  public:
    std::map<ValueIdT, ValueT> values;
    std::map<KeyT, std::unique_ptr<NodeT>> children;

    TrieNode() = default;
    TrieNode(const TrieNode&) = delete;
    TrieNode(TrieNode&&) noexcept = default;
    ~TrieNode() = default;

    TrieNode& operator=(const TrieNode&) = delete;
    TrieNode& operator=(TrieNode&&) noexcept = default;

    [[nodiscard]]
    inline NodeT* find(const KeyT& key) {
        return children.at(key).get();
    }

    [[nodiscard]]
    NodeT* find(std::span<const KeyT> key_span) {
        auto node = this;
        for (const auto& key : key_span) {
            node = node->find(key);
        }
        return node;
    }

    [[nodiscard]]
    inline NodeT* findOrNull(const KeyT& key) noexcept {
        auto it = children.find(key);
        if (it != children.end()) {
            return it->second.get();
        } else {
            return nullptr;
        }
    }

    [[nodiscard]]
    NodeT* findOrNull(std::span<const KeyT> key_span) noexcept {
        auto node = this;
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
        auto it = children.find(key);
        if (it != children.end()) {
            return it->second.get();
        } else {
            auto new_child = std::make_unique<NodeT>();
            auto ret_pointer = new_child.get();
            children.insert({key, std::move(new_child)});
            return ret_pointer;
        }
    }

    [[nodiscard]]
    NodeT* findOrCreate(std::span<const KeyT> key_span) {
        auto node = this;
        for (const auto& key : key_span) {
            node = node->findOrCreate(key);
        }
        return node;
    }

    [[nodiscard]]
    inline ValueT* value(ValueIdT id) {
        return &values.at(id);
    }

    [[nodiscard]]
    inline ValueT* valueOrNull(ValueIdT id) noexcept {
        auto it = values.find(id);
        if (it != values.end()) {
            return &(it->second);
        } else {
            return nullptr;
        }
    }

    [[nodiscard]]
    inline ValueT* valueOrCreate(ValueIdT id) {
        return &values[id];
    }

    [[nodiscard]]
    inline bool isEndNode() const noexcept {
        return !values.empty();
    }

    [[nodiscard]]
    inline bool isEndNode(ValueIdT id) const noexcept {
        return values.find(id) != values.end();
    }

    void forEach(std::span<const KeyT> termination_tokens, NodeActionT& action) noexcept {
        std::vector<KeyT> word_cache;
        forEach(word_cache, 0, termination_tokens, action);
    }

    void forEach(NodeActionT& action) noexcept {
        std::vector<KeyT> word_cache;
        forEach(word_cache, 0, {}, action);
    }

  private:
    void forEach(
        std::vector<KeyT>& word_cache,
        size_t insert_index,
        std::span<const KeyT> termination_tokens,
        NodeActionT& action
    ) const noexcept {
        for (auto it = children.begin(); it != children.end(); it++) {
            word_cache.resize(insert_index + 1);
            auto& key = it->first;
            if (std::find(termination_tokens.begin(), termination_tokens.end(), key) != termination_tokens.end()) {
                continue;
            }
            auto* child_node = it->second.get();
            word_cache[insert_index] = key;
            if (child_node->isEndNode()) {
                action(word_cache, child_node);
            }
            child_node->forEach(word_cache, insert_index + 1, termination_tokens, action);
        }
    }
};

} // namespace fl::nlp
