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

#ifndef __FLORISNLP_ICUEXT_USTRING_H__
#define __FLORISNLP_ICUEXT_USTRING_H__

#include <unicode/locid.h>
#include <unicode/umachine.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/utypes.h>

#include <string>

namespace icuext {

using u8char = char;
using u8str = std::basic_string<u8char>;
using u8str_view = std::basic_string_view<u8char>;

using u16char = UChar;
using u16str = std::basic_string<u16char>;
using u16str_view = std::basic_string_view<u16char>;

using u32char = UChar32;
using u32str = std::basic_string<u32char>;
using u32str_view = std::basic_string_view<u32char>;

using unistr = icu::UnicodeString;
using locale = icu::Locale;

namespace str {

void to_unistr(const u8str& src, unistr& dst) noexcept;
void to_unistr(const u16str& src, unistr& dst) noexcept;
void to_unistr(const u32str& src, unistr& dst) noexcept;
unistr to_unistr(const u8str& src) noexcept;
unistr to_unistr(const u16str& src) noexcept;
unistr to_unistr(const u32str& src) noexcept;

void to_u8str(const unistr& src, u8str& dst) noexcept;
void to_u8str(const u16str& src, u8str& dst) noexcept;
void to_u8str(const u32str& src, u8str& dst) noexcept;
u8str to_u8str(const unistr& src) noexcept;
u8str to_u8str(const u16str& src) noexcept;
u8str to_u8str(const u32str& src) noexcept;

void to_u16str(const unistr& src, u16str& dst) noexcept;
void to_u16str(const u8str& src, u16str& dst) noexcept;
void to_u16str(const u32str& src, u16str& dst) noexcept;
u16str to_u16str(const unistr& src) noexcept;
u16str to_u16str(const u8str& src) noexcept;
u16str to_u16str(const u32str& src) noexcept;

void to_u32str(const unistr& src, u32str& dst) noexcept;
void to_u32str(const u8str& src, u32str& dst) noexcept;
void to_u32str(const u16str& src, u32str& dst) noexcept;
u32str to_u32str(const unistr& src) noexcept;
u32str to_u32str(const u8str& src) noexcept;
u32str to_u32str(const u16str& src) noexcept;

u8str to_uppercase(const u8str& str) noexcept;
u16str to_uppercase(const u16str& str) noexcept;
u32str to_uppercase(const u32str& str) noexcept;
u8str to_uppercase(const u8str& str, const locale& locale) noexcept;
u16str to_uppercase(const u16str& str, const locale& locale) noexcept;
u32str to_uppercase(const u32str& str, const locale& locale) noexcept;

u8str to_lowercase(const u8str& str) noexcept;
u16str to_lowercase(const u16str& str) noexcept;
u32str to_lowercase(const u32str& str) noexcept;
u8str to_lowercase(const u8str& str, const locale& locale) noexcept;
u16str to_lowercase(const u16str& str, const locale& locale) noexcept;
u32str to_lowercase(const u32str& str, const locale& locale) noexcept;

void uppercase(u8str& str) noexcept;
void uppercase(u16str& str) noexcept;
void uppercase(u32str& str) noexcept;
void uppercase(u8str& str, const locale& locale) noexcept;
void uppercase(u16str& str, const locale& locale) noexcept;
void uppercase(u32str& str, const locale& locale) noexcept;
void lowercase(u8str& str) noexcept;
void lowercase(u16str& str) noexcept;
void lowercase(u32str& str) noexcept;
void lowercase(u8str& str, const locale& locale) noexcept;
void lowercase(u16str& str, const locale& locale) noexcept;
void lowercase(u32str& str, const locale& locale) noexcept;

u8str trimmed(const u8str& src) noexcept;
u16str trimmed(const u16str& src) noexcept;
u32str trimmed(const u32str& src) noexcept;

void trim(u8str& src) noexcept;
void trim(u16str& src) noexcept;
void trim(u32str& src) noexcept;

} // namespace str

} // namespace icuext

#endif
