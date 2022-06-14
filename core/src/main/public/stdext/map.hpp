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

#ifndef _FLORISNLP_CORE_STDEXT_MAP
#define _FLORISNLP_CORE_STDEXT_MAP

#include <charconv>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace stdext::map {

/**
 * Get the value of `key` from `map`, or return `def_value` if the `key` does not exist. Unlike the `[]` operator,
 * this method does not modify the `map` in any way.
 *
 * @param map The map on which this method should operate.
 * @param key The key for which the value should be returned.
 * @param def_value The default value to return, in case the key does not exist in the map.
 *
 * @return The value of `key` or `def_value`.
 */
template<typename K, typename V>
auto get_or_default(const std::map<K, V> &map, const K &key, const V &def_value) noexcept -> V {
    auto pair = map.find(key);
    return pair != map.end() ? pair->second : def_value;
}

} // namespace stdext::map

#endif // _FLORISNLP_CORE_STDEXT_MAP
