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

#ifndef _FLORISNLP_CORE_STDEXT_STRING
#define _FLORISNLP_CORE_STDEXT_STRING

#include <charconv>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace stdext::string {

/**
 * Splits given string with given delimiter and pushes the resulting parts into the `out` vector. If `out` is
 * not empty, it will be cleared before being filled up with matches.
 *
 * @param out The vector in which the resulting splitted part views are stored.
 * @param strv The string which should be split up. May be empty, in which case `out` will also be empty.
 * @param delimv The delimiter, which is used to split `strv`. Undefined behavior if delimiter is empty.
 */
auto split(std::vector<std::string_view>& out, const std::string_view& strv, const std::string_view& delimv) noexcept
    -> void {
    if (!out.empty()) out.clear();
    const auto begin = strv.cbegin();
    const auto end = strv.cend();
    auto it = begin;
    while (it != end) {
        const auto delim_it = std::find_first_of(it, end, delimv.cbegin(), delimv.cend());
        if (it != delim_it) {
            out.emplace_back(strv.substr(std::distance(begin, it), std::distance(it, delim_it)));
        }
        if (delim_it == end) break;
        it = std::next(delim_it);
    }
}

/**
 * Splits given string with given delimiter and pushes the resulting parts into the `out` vector. If `out` is
 * not empty, it will be cleared before being filled up with matches.
 *
 * @param out The vector in which the resulting splitted part views are stored.
 * @param str The string which should be split up. May be empty, in which case `out` will also be empty.
 * @param delim The delimiter, which is used to split `strv`. Undefined behavior if delimiter is empty.
 */
inline auto split(std::vector<std::string_view>& out, const std::string& str, const std::string& delim) noexcept
    -> void {
    const std::string_view strv(str);
    const std::string_view delimv(delim);
    split(out, strv, delimv);
}

/**
 * Splits given string with given delimiter and pushes the resulting parts into the `out` vector. If `out` is
 * not empty, it will be cleared before being filled up with matches.
 *
 * @param out The vector in which the resulting splitted part views are stored.
 * @param str The string which should be split up. May be empty, in which case `out` will also be empty.
 * @param delimv The delimiter, which is used to split `strv`. Undefined behavior if delimiter is empty.
 */
inline auto split(std::vector<std::string_view>& out, const std::string& str, const std::string_view& delimv) noexcept
    -> void {
    const std::string_view strv(str);
    split(out, strv, delimv);
}

/**
 * Splits given string with given delimiter and pushes the resulting parts into the `out` vector. If `out` is
 * not empty, it will be cleared before being filled up with matches.
 *
 * @param out The vector in which the resulting splitted part views are stored.
 * @param strv The string which should be split up. May be empty, in which case `out` will also be empty.
 * @param delim The delimiter, which is used to split `strv`. Undefined behavior if delimiter is empty.
 */
inline auto split(std::vector<std::string_view>& out, const std::string_view& strv, const std::string& delim) noexcept
    -> void {
    const std::string_view delimv(delim);
    split(out, strv, delimv);
}

/**
 * Converts given string to a number fitting into the specified integral type. Unspecified behavior if given
 * type is not of integral type. This method may throw if an error occurs.
 *
 * @param strv The string containing the number.
 * @param base The base of the number in the string. May be `0` for auto selection or any value between `2` and
 *  `36`. Will throw if this value is not within specified boundaries. Defaults to `10`.
 *
 * @return The converted number.
 */
template<typename IntegralType>
auto to_number(const std::string_view& strv, const int base = 10) -> IntegralType {
    if (!(base == 0 || base >= 2 && base <= 36)) {
        throw std::invalid_argument("Parameter base has invalid value (" + std::to_string(base) + ")!");
    }
    IntegralType value;
    auto result = std::from_chars(strv.data(), strv.data() + strv.size(), value, base);
    if (result.ec == std::errc::invalid_argument) {
        throw std::invalid_argument("Given string is not a valid number!");
    } else if (result.ec == std::errc::result_out_of_range) {
        throw std::out_of_range("Given string contains a number which exceeds the integer value limits!");
    }
    return value;
}

/**
 * Converts given string to a number fitting into the specified integral type. Unspecified behavior if given
 * type is not of integral type. This method may throw if an error occurs.
 *
 * @param strv The string containing the number.
 * @param base The base of the number in the string. May be `0` for auto selection or any value between `2` and
 *  `36`. Will throw if this value is not within specified boundaries. Defaults to `10`.
 *
 * @return The converted number.
 */
template<typename IntegralType>
inline auto to_number(const std::string& str, const int base = 10) -> IntegralType {
    std::string_view strv(str);
    return to_number<IntegralType>(strv, base);
}

} // namespace stdext::string

#endif // _FLORISNLP_CORE_STDEXT_STRING
