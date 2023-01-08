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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

export module fl.nlp.core.key_proximity_map;

import fl.nlp.string;

namespace fl::nlp {

export class KeyProximityMap {
    using KeyDataMapT = std::unordered_map<fl::str::UniChar, std::vector<fl::str::UniChar>>;

  public:
    KeyProximityMap() = default;
    ~KeyProximityMap() = default;

    bool isInProximity(const fl::str::UniChar& assumed, const fl::str::UniChar& actual) const noexcept {
        if (!data_.contains(assumed)) return false;
        auto keys = data_.at(assumed);
        return std::find(keys.begin(), keys.end(), actual) != keys.end();
    }

    void clear() noexcept {
        //_data.clear();
    }

    void loadFromFile(const std::filesystem::path& path, bool clear_existing = true) {
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
        json_mapping_data.get_to(data_);
    }

  private:
    KeyDataMapT data_;
};

} // namespace fl::nlp
