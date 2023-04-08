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
#include <vector>

export module fl.nlp.utils;

namespace fl::utils {

template<typename T>
using CombiningCallback = const std::function<void(const std::vector<T>)>;

template<typename T>
void forEachCombinationInternal(
    const std::vector<const std::vector<T>*>& vectors,
    std::vector<T>& current_combination,
    int current_vector_index,
    CombiningCallback<T>& callback
) {
    if (current_vector_index == vectors.size()) {
        callback(current_combination);
        return;
    }

    for (const auto& elem : *vectors[current_vector_index]) {
        current_combination[current_vector_index] = elem;
        forEachCombinationInternal(vectors, current_combination, current_vector_index + 1, callback);
    }
}

export template<typename T>
void forEachCombination(const std::vector<const std::vector<T>*>& vectors, CombiningCallback<T>& callback) {
    std::vector<T> current_combination(vectors.size(), "");
    forEachCombinationInternal<T>(vectors, current_combination, 0, callback);
}

export template<typename K, typename V>
V& findOrDefault(std::map<K, V>& map, const K& key, V& default_value) noexcept {
    auto it = map.find(key);
    return it != map.end() ? it->second : default_value;
}

export template<typename K, typename V>
const V& findOrDefault(const std::map<K, V>& map, const K& key, const V& default_value) noexcept {
    auto it = map.find(key);
    return it != map.end() ? it->second : default_value;
}

} // namespace fl::utils
