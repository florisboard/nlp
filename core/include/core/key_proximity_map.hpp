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

#ifndef __FLORISNLP_CORE_KEY_PROXIMITY_MAP_H__
#define __FLORISNLP_CORE_KEY_PROXIMITY_MAP_H__

#include "core/string.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <vector>

namespace fl::nlp {

class key_proximity_map {
    using key_data_map = std::map<fl::u8str, std::vector<fl::u8str>>;

  public:
    key_proximity_map() = default;
    key_proximity_map(const key_data_map& data) : _data(data) {};
    ~key_proximity_map() = default;

    bool is_in_proximity(const fl::u8str& assumed, const fl::u8str& actual) const noexcept {
        if (_data.contains(actual)) {
            auto& surrounding_keys = _data.at(actual);
            return std::find(surrounding_keys.begin(), surrounding_keys.end(), assumed) != surrounding_keys.end();
        }
        return false;
    }

    constexpr std::vector<fl::u8str>& operator[](const fl::u8str& chstr) {
        return _data[chstr];
    }

    constexpr const std::vector<fl::u8str>& at(const fl::u8str& chstr) const {
        return _data.at(chstr);
    }

    constexpr std::vector<fl::u8str>& at(const fl::u8str& chstr) {
        return _data.at(chstr);
    }

    void clear() noexcept {
        _data.clear();
    }

    void load_from_file(std::filesystem::path path, bool clear_existing = true) {
        using namespace std::string_literals;

        std::ifstream json_mapping_file(path);
        if (!json_mapping_file.is_open()) {
            throw std::runtime_error("Could not load from file '"s + path.string() + "'"s);
        }
        auto json_mapping_data = nlohmann::json::parse(json_mapping_file);
        json_mapping_file.close();

        auto parsed_data = json_mapping_data.get<key_data_map>();
        if (clear_existing) {
            _data.clear();
        }
        _data.insert(parsed_data.begin(), parsed_data.end());
    }

  private:
    key_data_map _data;
};

} // namespace fl::nlp

#endif
