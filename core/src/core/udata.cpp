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

#include "core/udata.hpp"

#include <unicode/udata.h>

#include <fstream>

UErrorCode fl::icu::load_and_set_common_data(std::string path) {
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
    in_file.read(icu_data, size);
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
