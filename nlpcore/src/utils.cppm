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
#include <iterator>
#include <map>
#include <span>
#include <vector>

export module fl.utils;

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
inline V& findOrDefault(std::map<K, V>& map, const K& key, V& default_value) noexcept {
    auto it = map.find(key);
    return it != map.end() ? it->second : default_value;
}

export template<typename K, typename V>
inline const V& findOrDefault(const std::map<K, V>& map, const K& key, const V& default_value) noexcept {
    auto it = map.find(key);
    return it != map.end() ? it->second : default_value;
}

export template<typename T>
bool equal(std::span<const T> span1, std::span<const T> span2) {
    return span1.size() == span2.size() && std::equal(span1.begin(), span1.end(), span2.begin());
}

export template<typename T>
bool equal(std::span<const T> span1, std::span<T> span2) {
    return span1.size() == span2.size() && std::equal(span1.begin(), span1.end(), span2.begin());
}

export template<typename T>
bool equal(std::span<T> span1, std::span<const T> span2) {
    return span1.size() == span2.size() && std::equal(span1.begin(), span1.end(), span2.begin());
}

export template<typename T>
bool equal(std::span<T> span1, std::span<T> span2) {
    return span1.size() == span2.size() && std::equal(span1.begin(), span1.end(), span2.begin());
}

// TODO: remove this once we upgrade Android NDK to r26
export template<typename It>
std::span<const typename std::iterator_traits<It>::value_type> make_span(It begin, It end) {
    using value_type = typename std::iterator_traits<It>::value_type;
    return {&(*begin), static_cast<std::size_t>(std::distance(begin, end))};
}

// TODO: remove this once we upgrade Android NDK to r26
export template<typename It>
std::span<const typename std::iterator_traits<It>::value_type> make_span(It begin, std::size_t count) {
    using value_type = typename std::iterator_traits<It>::value_type;
    return {&(*begin), count};
}

} // namespace fl::utils
