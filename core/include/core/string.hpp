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

#ifndef __FLORISNLP_CORE_STRING_H__
#define __FLORISNLP_CORE_STRING_H__

#include <string>
#include <vector>

namespace fl {

using u8char = char;
using u8str = std::basic_string<u8char>;
using u8str_view = std::basic_string_view<u8char>;

// TODO: Rename chstr to something more unique??
using u8uchar = unsigned char;
using u8chstr = fl::u8str;
using u8chstr_vec = std::vector<fl::u8chstr>;

namespace str {

void lowercase(u8str& str) noexcept;

void titlecase(u8str& str) noexcept;

void uppercase(u8str& str) noexcept;

void trim(u8str& src) noexcept;

void split(const u8str& src, const u8str& delim, std::vector<u8str>& dst) noexcept;

void split(const u8str& src, u8char delim, std::vector<u8str>& dst) noexcept;

} // namespace str

namespace chstr {

void vec_to_str(const u8chstr_vec& vec, u8str& str) noexcept;

void str_to_vec(const u8str& str, u8chstr_vec& vec, const fl::u8str& locale_tag = "") noexcept;

bool compare(const u8chstr_vec& a, const u8chstr_vec& b) noexcept;

} // namespace chstr

} // namespace fl

#endif
