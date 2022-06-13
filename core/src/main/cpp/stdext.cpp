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

#include "stdext.hpp"

auto stdext::str_split(const std::string &s, const std::string &delim, std::vector<std::string> &result) noexcept -> void {
    if (s.empty()) return;
    result.clear();
    std::size_t start = 0;
    std::size_t end;
    while ((end = s.find(delim, start)) != std::string::npos) {
        result.push_back(s.substr(start, end));
        start = end + delim.length();
    }
    result.push_back(s.substr(start));
}

auto stdext::str_split(const std::string &s, const std::string &delim) noexcept -> std::vector<std::string> {
    auto result = std::vector<std::string>();
    stdext::str_split(s, delim, result);
    return result;
}
