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

#include <unicode/utext.h>

export module fl.icuext:utext;

namespace fl::icuext {

/**
 * Helper class which wraps the ICU C API UText into a modern C++ class with automatic closing on destruct.
 *
 * For the exact documentation of UText and each of its methods see the docs for the UText C API.
 *
 * @see https://unicode-org.github.io/icu-docs/apidoc/dev/icu4c/utext_8h.html
 */
export class Text {
  public:
    Text() : ut(nullptr) {};
    Text(UText* ut_) : ut(ut_) {};
    ~Text() {
        close();
    }

    Text(const Text&) = delete;
    Text(Text&& other) noexcept : ut(other.ut) {
        other.ut = nullptr;
    }

    Text& operator=(const Text&) = delete;
    Text& operator=(Text&& other) noexcept {
        if (this != &other) {
            close();
            ut = other.ut;
            other.ut = nullptr;
        }
        return *this;
    }

    void openUTF8(const std::string& str, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return;

        ut = utext_openUTF8(ut, str.c_str(), str.size(), &status);
    }

    void openUTF8(const char* str, int64_t length, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return;

        ut = utext_openUTF8(ut, str, length, &status);
    }

    void openCharacterIterator(icu::CharacterIterator& ci, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return;

        ut = utext_openCharacterIterator(ut, &ci, &status);
    }

    void openReplaceable(icu::Replaceable& rep, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return;

        ut = utext_openReplaceable(ut, &rep, &status);
    }

    void openUChars(const UChar* str, int64_t length, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return;

        ut = utext_openUChars(ut, str, length, &status);
    }

    void openUnicodeString(icu::UnicodeString& str, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return;

        ut = utext_openUnicodeString(ut, &str, &status);
    }

    void openConstUnicodeString(const icu::UnicodeString& str, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return;

        ut = utext_openConstUnicodeString(ut, &str, &status);
    }

    void close() {
        utext_close(ut);
        ut = nullptr;
    }

    UChar32 char32At(int64_t native_index) {
        return utext_char32At(ut, native_index);
    }

    Text clone(bool deep, bool read_only, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return {};

        UText* ut_clone = utext_clone(nullptr, ut, deep, read_only, &status);
        if (ut_clone == nullptr) {
            status = U_UNSUPPORTED_ERROR;
        }
        return {ut_clone};
    }

    void copy(int64_t native_start, int64_t native_limit, int64_t dest_index, bool move, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return;

        utext_copy(ut, native_start, native_limit, dest_index, move, &status);
    }

    UChar32 current32() {
        return utext_current32(ut);
    }

    int32_t extract(int64_t native_start, int64_t native_limit, UChar* dest, int32_t dest_capacity,
                    UErrorCode& status) {
        if (status != U_ZERO_ERROR) return 0;

        return utext_extract(ut, native_start, native_limit, dest, dest_capacity, &status);
    }

    void freeze() {
        return utext_freeze(ut);
    }

    int64_t getNativeIndex() {
        return utext_getNativeIndex(ut);
    }

    int64_t getPreviousNativeIndex() {
        return utext_getPreviousNativeIndex(ut);
    }

    bool hasMetaData() {
        return utext_hasMetaData(ut);
    }

    bool isLengthExpensive() {
        return utext_isLengthExpensive(ut);
    }

    bool isWritable() {
        return utext_isWritable(ut);
    }

    bool moveIndex32(int32_t delta) {
        return utext_moveIndex32(ut, delta);
    }

    int64_t nativeLength() {
        return utext_nativeLength(ut);
    }

    UChar32 next32() {
        return utext_next32(ut);
    }

    UChar32 next32From(int64_t native_index) {
        return utext_next32From(ut, native_index);
    }

    UChar32 previous32() {
        return utext_previous32(ut);
    }

    UChar32 previous32From(int64_t native_index) {
        return utext_previous32From(ut, native_index);
    }

    UChar32 replace(int64_t native_start, int64_t native_limit, const UChar* replacement_text,
                    int32_t replacement_length, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return 0;

        return utext_replace(ut, native_start, native_limit, replacement_text, replacement_length, &status);
    }

    void setNativeIndex(int64_t native_index) {
        utext_setNativeIndex(ut, native_index);
    }

    void setup(int32_t extra_space, UErrorCode& status) {
        if (status != U_ZERO_ERROR) return;

        utext_setup(ut, extra_space, &status);
    }

    bool operator==(const Text& other) const {
        return utext_equals(ut, other.ut);
    }

    inline operator UText*() const noexcept {
        return ut;
    }

  private:
    UText* ut;
};

} // namespace fl::icuext
