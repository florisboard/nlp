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
#include <map>
#include <memory>
#include <span>

export module fl.nlp.core.common:trie_map;

namespace fl::nlp {

export template<class KeyT, class ValueT>
struct TrieNode {
  private:
    using NodeT = TrieNode<KeyT, ValueT>;

  public:
    ValueT value;
    bool is_end_node = false;
    std::map<KeyT, std::unique_ptr<NodeT>> children;

    NodeT* getChild(const KeyT& key) {
        return children.at(key).get();
    }

    const NodeT* getChild(const KeyT& key) const {
        return children.at(key).get();
    }

    NodeT* getChildOrNull(const KeyT& key) noexcept {
        auto it = children.find(key);
        if (it != children.end()) {
            return it->second.get();
        } else {
            return nullptr;
        }
    }

    const NodeT* getChildOrNull(const KeyT& key) const noexcept {
        auto it = children.find(key);
        if (it != children.end()) {
            return it->second.get();
        } else {
            return nullptr;
        }
    }

    NodeT* getChildOrCreate(const KeyT& key) {
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

    const NodeT* getChildOrCreate(const KeyT& key) const {
        auto it = children.find(key);
        if (it != children.end()) {
            return it->second.get();
        } else {
            auto new_child = std::make_unique<NodeT>();
            auto ret_pointer = new_child.get();
            children.insert(key, std::move(new_child));
            return ret_pointer;
        }
    }

    void setChild(const KeyT& key, const ValueT& new_value) noexcept {
        children.insert_or_assign(key, new_value);
    }

    void setChild(const KeyT& key, ValueT&& new_value) noexcept {
        children.insert_or_assign(key, new_value);
    }

    void forEach(const std::function<void(const std::span<KeyT>&, ValueT&)>& action) noexcept {
        std::vector<KeyT> word_cache;
        forEach(word_cache, 0, action);
    }

    void forEach(
        std::vector<KeyT>& word_cache,
        size_t insert_index,
        const std::function<void(const std::span<KeyT>&, ValueT&)>& action
    ) noexcept {
        for (auto it = children.begin(); it != children.end(); it++) {
            word_cache.resize(insert_index + 1);
            auto& key = it->first;
            auto* child_node = it->second.get();
            word_cache[insert_index] = key;
            if (child_node->is_end_node) {
                action(word_cache, child_node->value);
            }
            child_node->forEach(word_cache, insert_index + 1, action);
        }
    }
};

export template<class KeyT, class ValueT>
class TrieMap {
  private:
    using KeySpanT = std::span<KeyT>;
    using NodeT = TrieNode<KeyT, ValueT>;

  public:
    NodeT* get(KeySpanT key_span) {
        auto* current_node = &root_node_;
        for (auto& key : key_span) {
            current_node = current_node->getChild(key);
        }
        if (!current_node->is_end_node) {
            throw std::out_of_range("Key exists in map but is not marked as end node!");
        }
        return current_node;
    }

    const NodeT* get(KeySpanT key_span) const {
        auto* current_node = &root_node_;
        for (auto& key : key_span) {
            current_node = current_node->getChild(key);
        }
        if (!current_node->is_end_node) {
            throw std::out_of_range("Key exists in map but is not marked as end node!");
        }
        return current_node;
    }

    NodeT* getOrNull(KeySpanT key_span) noexcept {
        auto* current_node = &root_node_;
        for (auto& key : key_span) {
            current_node = current_node->getChildOrNull(key);
            if (current_node == nullptr) {
                return nullptr;
            }
        }
        if (!current_node->is_end_node) {
            return nullptr;
        }
        return current_node;
    }

    const NodeT* getOrNull(KeySpanT key_span) const noexcept {
        auto* current_node = &root_node_;
        for (auto& key : key_span) {
            current_node = current_node->getChildOrNull(key);
            if (current_node == nullptr) {
                return nullptr;
            }
        }
        if (!current_node->is_end_node) {
            return nullptr;
        }
        return current_node;
    }

    NodeT* getOrCreate(KeySpanT key_span) noexcept {
        auto* current_node = &root_node_;
        for (auto& key : key_span) {
            current_node = current_node->getChildOrCreate(key);
        }
        current_node->is_end_node = true;
        return current_node;
    }

    void set(KeySpanT key_span, const ValueT& new_value) noexcept {
        auto* end_node = getOrCreate(key_span);
        end_node->is_end_node = true;
        end_node->value = new_value;
    }

    void set(KeySpanT key_span, ValueT&& new_value) noexcept {
        auto* end_node = getOrCreate(key_span);
        end_node->value = new_value;
    }

    bool contains(KeySpanT key_span) const noexcept {
        return getOrNull(key_span) != nullptr;
    }

    inline NodeT* rootNode() noexcept {
        return &root_node_;
    }

    inline const NodeT* rootNode() const noexcept {
        return &root_node_;
    }

    void forEach(const std::function<void(const KeySpanT&, const ValueT&)>& action) const noexcept {
        std::vector<KeyT> word_cache;
        rootNode()->forEach(word_cache, 0, action);
    }

    void forEach(const std::function<void(const KeySpanT&, ValueT&)>& action) noexcept {
        std::vector<KeyT> word_cache;
        rootNode()->forEach(word_cache, 0, action);
    }

  private:
    NodeT root_node_;
};

} // namespace fl::nlp
