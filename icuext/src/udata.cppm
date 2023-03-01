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

#include <nlohmann/json.hpp>
#include <unicode/locid.h>
#include <unicode/udata.h>
#include <unicode/utypes.h>

#include <fstream>

export module fl.icuext:udata;

namespace fl::icuext {

export UErrorCode loadAndSetCommonData(const std::string& path) {
    std::ifstream in_file(path, std::ios::in | std::ios::binary);
    if (!in_file) {
        return U_FILE_ACCESS_ERROR;
    }
    in_file.seekg(0, std::ios::end);
    size_t size = in_file.tellg();
    if (size <= 0) {
        return U_FILE_ACCESS_ERROR;
    }
    in_file.seekg(0, std::ios::beg);
    char* icu_data = new char[size + 1];
    in_file.read(icu_data, static_cast<std::streamsize>(size));
    if (!in_file) {
        delete[] icu_data;
        in_file.close();
        return U_FILE_ACCESS_ERROR;
    }
    icu_data[size] = 0;
    in_file.close();
    UErrorCode status = U_ZERO_ERROR;
    udata_setCommonData(reinterpret_cast<void*>(icu_data), &status);
    return status;
}

} // namespace fl::icuext

namespace nlohmann {

export template<>
struct adl_serializer<icu::Locale> {
    static void to_json(nlohmann::json& j, const icu::Locale& locale) {
        UErrorCode status = U_ZERO_ERROR;
        j = nlohmann::json(locale.toLanguageTag<std::string>(status));
    }

    static void from_json(const nlohmann::json& j, icu::Locale& locale) {
        UErrorCode status = U_ZERO_ERROR;
        auto tag = j.get<std::string>();
        locale = icu::Locale::forLanguageTag(tag, status);
    }
};

} // namespace nlohmann
