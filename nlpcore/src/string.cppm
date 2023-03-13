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

#include <unicode/ubrk.h>
#include <unicode/ucasemap.h>
#include <unicode/uchar.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

#include <span>
#include <sstream>
#include <string>
#include <vector>

export module fl.nlp.string;

namespace fl::str {

export using UniChar = std::basic_string<char>;
export using UniString = std::vector<UniChar>;
export using UniStringSpan = std::span<UniChar>;

export void applyCasemap(
    std::string& str,
    const std::function<int32_t(UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*)>& casemapper
) noexcept {
    if (str.empty()) return;

    // Set up case mapper
    UErrorCode status = U_ZERO_ERROR;
    UCaseMap* csm = ucasemap_open("", U_FOLD_CASE_DEFAULT, &status);
    if (csm == nullptr || U_FAILURE(status)) return;

    // Calculate length of dst
    size_t dst_length = casemapper(csm, nullptr, 0, str.c_str(), -1, &status);
    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR) return; // Ignoring buffer overflow error on purpose
    status = U_ZERO_ERROR;

    // Apply case mapping
    char* dst_buffer = new char[dst_length + 1] {0};
    size_t dst_length_actually_written = casemapper(csm, dst_buffer, dst_length + 1, str.c_str(), -1, &status);
    if (dst_length_actually_written != dst_length || U_FAILURE(status)) return;
    str.assign(dst_buffer);

    // Clean up
    ucasemap_close(csm);
    delete[] dst_buffer;
}

export void lowercase(std::string& str) noexcept {
    applyCasemap(str, ucasemap_utf8ToLower);
}

export void titlecase(std::string& str) noexcept {
    applyCasemap(str, ucasemap_utf8ToTitle);
}

export void uppercase(std::string& str) noexcept {
    applyCasemap(str, ucasemap_utf8ToUpper);
}

export bool is_whitespace(char c) noexcept {
    return u_isWhitespace(c);
}

export void trim(std::string& src) noexcept {
    src.erase(std::find_if_not(src.rbegin(), src.rend(), is_whitespace).base(), src.end());
    src.erase(src.begin(), std::find_if_not(src.begin(), src.end(), is_whitespace));
}

export void split(
    const std::string& src, const std::string& delim, std::vector<std::string>& dst, std::size_t max_split_ops = 0
) noexcept {
    dst.clear();
    size_t last = 0;
    size_t next;
    size_t split_ops = 0;
    while ((next = src.find(delim, last)) != std::string::npos) {
        dst.push_back(src.substr(last, next - last));
        last = next + 1;
        split_ops++;
        if (max_split_ops > 0 && split_ops >= max_split_ops) {
            break;
        }
    }
    dst.push_back(src.substr(last));
}

export void split(
    const std::string& src, char delim, std::vector<std::string>& dst, std::size_t max_split_ops = 0
) noexcept {
    dst.clear();
    size_t last = 0;
    size_t next;
    size_t split_ops = 0;
    while ((next = src.find(delim, last)) != std::string::npos) {
        dst.push_back(src.substr(last, next - last));
        last = next + 1;
        split_ops++;
        if (max_split_ops > 0 && split_ops >= max_split_ops) {
            break;
        }
    }
    dst.push_back(src.substr(last));
}

export std::string repeat(const std::string& str, size_t n) {
    std::stringstream ss;
    for (size_t i = 0; i < n; i++) {
        ss << str;
    }
    return ss.str();
}

export void toUniString(const std::string& str, UniString& uni_str, const std::string& locale_tag = "") noexcept {
    uni_str.clear();

    auto status = U_ZERO_ERROR;
    auto ut = utext_openUTF8(nullptr, str.c_str(), static_cast<int64_t>(str.size()), &status);
    auto ub = ubrk_open(UBRK_CHARACTER, locale_tag.c_str(), nullptr, 0, &status);
    ubrk_setUText(ub, ut, &status);

    if (U_SUCCESS(status)) {
        int32_t prev_n = 0;
        int32_t curr_n;

        while ((curr_n = ubrk_next(ub)) != UBRK_DONE) {
            auto uni_char = str.substr(prev_n, curr_n - prev_n);
            uni_str.push_back(std::move(uni_char));
            prev_n = curr_n;
        }
    }

    ubrk_close(ub);
    utext_close(ut);
}

export void toStdString(std::span<const UniChar> uni_str, std::string& str) noexcept {
    str.clear();
    for (auto& uni_char : uni_str) {
        str.append(uni_char);
    }
}

export bool compare(std::span<const UniChar> a, std::span<const UniChar> b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

export bool startsWith(std::span<const UniChar> str, std::span<const UniChar> prefix) noexcept {
    if (prefix.size() > str.size()) return false;
    for (int i = 0; i < prefix.size(); i++) {
        if (prefix[i] != str[i]) return false;
    }
    return true;
}

} // namespace fl::str
