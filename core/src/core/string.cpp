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

#include "core/string.hpp"

#include <unicode/uchar.h>
#include <unicode/utypes.h>

#include <functional>

void apply_casemap(
    fl::u8str& str,
    UCaseMap* cached_csm,
    std::function<int32_t(UCaseMap*, char*, int32_t, const char*, int32_t, UErrorCode*)> casemapper
) noexcept {
    if (str.empty()) return;

    // Set up case mapper
    UErrorCode status = U_ZERO_ERROR;
    UCaseMap* csm = cached_csm != nullptr ? cached_csm : ucasemap_open("", U_FOLD_CASE_DEFAULT, &status);
    if (csm == nullptr || U_FAILURE(status)) return;

    // Calculate length of dst
    size_t dst_length = casemapper(csm, nullptr, 0, str.c_str(), -1, &status);
    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR) return; // Ignoring buffer overflow error on purpose
    status = U_ZERO_ERROR;

    // Apply case mapping
    char* dst_buffer = new char[dst_length + 1] { 0 };
    size_t dst_length_actually_written = casemapper(csm, dst_buffer, dst_length + 1, str.c_str(), -1, &status);
    if (dst_length_actually_written != dst_length || U_FAILURE(status)) return;
    str.assign(dst_buffer);

    // Clean up
    if (cached_csm == nullptr) ucasemap_close(csm);
    delete[] dst_buffer;
}

void fl::str::lowercase(u8str& str, UCaseMap* cached_csm) noexcept {
    apply_casemap(str, cached_csm, ucasemap_utf8ToLower);
}

void fl::str::titlecase(u8str& str, UCaseMap* cached_csm) noexcept {
    apply_casemap(str, cached_csm, ucasemap_utf8ToTitle);
}

void fl::str::uppercase(u8str& str, UCaseMap* cached_csm) noexcept {
    apply_casemap(str, cached_csm, ucasemap_utf8ToUpper);
}

bool is_whitespace(fl::u8char c) noexcept {
    return u_isWhitespace(c);
}

void fl::str::trim(fl::u8str& src) noexcept {
    src.erase(std::find_if_not(src.rbegin(), src.rend(), is_whitespace).base(), src.end());
    src.erase(src.begin(), std::find_if_not(src.begin(), src.end(), is_whitespace));
}

void fl::str::split(const fl::u8str& src, const fl::u8str& delim, std::vector<fl::u8str>& dst) noexcept {
    dst.clear();
    size_t last = 0;
    size_t next = 0;
    while ((next = src.find(delim, last)) != fl::u8str::npos) {
        dst.push_back(src.substr(last, next - last));
        last = next + 1;
    }
    dst.push_back(src.substr(last));
}

void fl::str::split(const fl::u8str& src, fl::u8char delim, std::vector<fl::u8str>& dst) noexcept {
    dst.clear();
    size_t last = 0;
    size_t next = 0;
    while ((next = src.find(delim, last)) != fl::u8str::npos) {
        dst.push_back(src.substr(last, next - last));
        last = next + 1;
    }
    dst.push_back(src.substr(last));
}
