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

class KeyProximityMap {
    using KeyDataMapT = std::unordered_map<fl::u8str, std::vector<fl::u8str>>;

  public:
    KeyProximityMap() = default;
    ~KeyProximityMap() = default;

    bool isInProximity(const fl::u8str& assumed, const fl::u8str& actual) const noexcept {
        if (!_data.contains(assumed)) return false;
        auto keys = _data.at(assumed);
        return std::find(keys.begin(), keys.end(), actual) != keys.end();
    }

    void clear() noexcept {
        //_data.clear();
    }

    void loadFromFile(std::filesystem::path path, bool clear_existing = true) {
        using namespace std::string_literals;

        std::ifstream json_mapping_file(path);
        if (!json_mapping_file.is_open()) {
            throw std::runtime_error("Could not load from file '"s + path.string() + "'"s);
        }
        auto json_mapping_data = nlohmann::json::parse(json_mapping_file);
        json_mapping_file.close();

        if (clear_existing) {
            clear();
        }
        json_mapping_data.get_to(_data);
    }

  private:
    KeyDataMapT _data;
};

} // namespace fl::nlp

#endif
