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

#include "icuext/string.hpp"

#include <unicode/unistr.h>

namespace icuext {

void str::to_unistr(const u8str& src, unistr& dst) noexcept {
    auto tmp = to_unistr(src);
    dst.setTo(tmp);
}
void str::to_unistr(const u16str& src, unistr& dst) noexcept {
    auto tmp = to_unistr(src);
    dst.setTo(tmp);
}
void str::to_unistr(const u32str& src, unistr& dst) noexcept {
    auto tmp = to_unistr(src);
    dst.setTo(tmp);
}
unistr str::to_unistr(const u8str& src) noexcept { return unistr::fromUTF8(src.c_str()); }
unistr str::to_unistr(const u16str& src) noexcept { return unistr(src.c_str()); }
unistr str::to_unistr(const u32str& src) noexcept { return unistr::fromUTF32(src.c_str(), src.length()); }

void str::to_u8str(const unistr& src, u8str& dst) noexcept {
    dst.clear();
    src.toUTF8String(dst);
}
void str::to_u8str(const u16str& src, u8str& dst) noexcept {
    auto tmp = to_unistr(src);
    dst.clear();
    tmp.toUTF8String(dst);
}
void str::to_u8str(const u32str& src, u8str& dst) noexcept {
    auto tmp = to_unistr(src);
    dst.clear();
    tmp.toUTF8String(dst);
}
u8str str::to_u8str(const unistr& src) noexcept {
    auto tmp = u8str();
    to_u8str(src, tmp);
    return tmp;
}
u8str str::to_u8str(const u16str& src) noexcept {
    auto tmp = u8str();
    to_u8str(src, tmp);
    return tmp;
}
u8str str::to_u8str(const u32str& src) noexcept {
    auto tmp = u8str();
    to_u8str(src, tmp);
    return tmp;
}

void str::to_u16str(const unistr& src, u16str& dst) noexcept { dst.assign(src.getBuffer()); }
void str::to_u16str(const u8str& src, u16str& dst) noexcept {
    auto tmp = to_unistr(src);
    dst.assign(tmp.getBuffer());
}
void str::to_u16str(const u32str& src, u16str& dst) noexcept {
    auto tmp = to_unistr(src);
    dst.assign(tmp.getBuffer());
}
u16str str::to_u16str(const unistr& src) noexcept {
    auto tmp = u16str();
    to_u16str(src, tmp);
    return tmp;
}
u16str str::to_u16str(const u8str& src) noexcept {
    auto tmp = u16str();
    to_u16str(src, tmp);
    return tmp;
}
u16str str::to_u16str(const u32str& src) noexcept {
    auto tmp = u16str();
    to_u16str(src, tmp);
    return tmp;
}

void str::to_u32str(const unistr& src, u32str& dst) noexcept {
    dst.clear();
    dst.reserve(src.length());
    UErrorCode _;
    src.toUTF32(dst.data(), dst.length(), _);
}
void str::to_u32str(const u8str& src, u32str& dst) noexcept {
    auto tmp = to_unistr(src);
    dst.clear();
    dst.reserve(tmp.length());
    UErrorCode _;
    tmp.toUTF32(dst.data(), dst.length(), _);
}
void str::to_u32str(const u16str& src, u32str& dst) noexcept {
    auto tmp = to_unistr(src);
    dst.clear();
    dst.reserve(tmp.length());
    UErrorCode _;
    tmp.toUTF32(dst.data(), dst.length(), _);
}
u32str str::to_u32str(const unistr& src) noexcept {
    auto tmp = u32str();
    to_u32str(src, tmp);
    return tmp;
}
u32str str::to_u32str(const u8str& src) noexcept {
    auto tmp = u32str();
    to_u32str(src, tmp);
    return tmp;
}
u32str str::to_u32str(const u16str& src) noexcept {
    auto tmp = u32str();
    to_u32str(src, tmp);
    return tmp;
}

static const locale ROOT_LOCALE = locale::getRoot();

u8str str::to_uppercase(const u8str& str) noexcept { return to_uppercase(str, ROOT_LOCALE); }
u16str str::to_uppercase(const u16str& str) noexcept { return to_uppercase(str, ROOT_LOCALE); }
u32str str::to_uppercase(const u32str& str) noexcept { return to_uppercase(str, ROOT_LOCALE); }
u8str str::to_uppercase(const u8str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toUpper(locale);
    return to_u8str(tmp);
}
u16str str::to_uppercase(const u16str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toUpper(locale);
    return to_u16str(tmp);
}
u32str str::to_uppercase(const u32str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toUpper(locale);
    return to_u32str(tmp);
}

u8str str::to_lowercase(const u8str& str) noexcept { return to_lowercase(str, ROOT_LOCALE); }
u16str str::to_lowercase(const u16str& str) noexcept { return to_lowercase(str, ROOT_LOCALE); }
u32str str::to_lowercase(const u32str& str) noexcept { return to_lowercase(str, ROOT_LOCALE); }
u8str str::to_lowercase(const u8str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toUpper(locale);
    return to_u8str(tmp);
}
u16str str::to_lowercase(const u16str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toUpper(locale);
    return to_u16str(tmp);
}
u32str str::to_lowercase(const u32str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toUpper(locale);
    return to_u32str(tmp);
}

void str::uppercase(u8str& str) noexcept { uppercase(str, ROOT_LOCALE); }
void str::uppercase(u16str& str) noexcept { uppercase(str, ROOT_LOCALE); }
void str::uppercase(u32str& str) noexcept { uppercase(str, ROOT_LOCALE); }
void str::uppercase(u8str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toUpper(locale);
    to_u8str(tmp, str);
}
void str::uppercase(u16str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toUpper(locale);
    to_u16str(tmp, str);
}
void str::uppercase(u32str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toUpper(locale);
    to_u32str(tmp, str);
}

void str::lowercase(u8str& str) noexcept { lowercase(str, ROOT_LOCALE); }
void str::lowercase(u16str& str) noexcept { lowercase(str, ROOT_LOCALE); }
void str::lowercase(u32str& str) noexcept { lowercase(str, ROOT_LOCALE); }
void str::lowercase(u8str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toLower(locale);
    to_u8str(tmp, str);
}
void str::lowercase(u16str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toLower(locale);
    to_u16str(tmp, str);
}
void str::lowercase(u32str& str, const locale& locale) noexcept {
    auto tmp = to_unistr(str);
    tmp.toLower(locale);
    to_u32str(tmp, str);
}

} // namespace icuext
