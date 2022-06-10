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

#ifndef _FLORISNLP_CORE_STDLIBEXTENSIONS
#define _FLORISNLP_CORE_STDLIBEXTENSIONS

#include <string>
#include <vector>
#include <map>

/**
 * Extension namespace of `std`, which provides helper functions for actions that are not easy to achieve using only
 * stblib functions.
 */
namespace stdext {
    /**
     * Splits given string `s` with given delimiter `delim`, and pushes the resulting parts into `result`. If `s` is
     * empty, this function does nothing.
     *
     * @param s The string which should be split up.
     * @param delim The delimiter, which is used to split `s`. Can be any value. Undefined behavior if delimiter is
     *  empty.
     * @param result The vector in which the resulting splitted parts are stored.
     */
    auto str_split(const std::string &s, const std::string &delim, std::vector<std::string> &result) noexcept -> void;

    /**
     * Splits given string `s` with given delimiter `delim`, and returns the parts as a newly allocated vector. If `s`
     * is empty, this function returns an empty vector.
     *
     * @param s The string which should be split up.
     * @param delim The delimiter, which is used to split `s`. Can be any value. Undefined behavior if delimiter is
     *  empty.
     *
     * @return A newly allocated vector in which the resulting splitted parts are stored. Is empty if `s` is empty.
     */
    auto str_split(const std::string &s, const std::string &delim) noexcept -> std::vector<std::string>;

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
    auto map_get_or_default(const std::map<K, V> &map, const K &key, const V &def_value) noexcept -> V {
        auto pair = map.find(key);
        return pair != map.end() ? pair->second : def_value;
    }
}

#endif // _FLORISNLP_CORE_STDLIBEXTENSIONS
