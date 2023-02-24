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

#include "nlohmann/json.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

export module fl.nlp.core.common.key_proximity_map;

import fl.nlp.string;

namespace fl::nlp {

using json = nlohmann::json;

export class KeyProximityMap {
    using KeyDataMapT = std::unordered_map<fl::str::UniChar, std::vector<fl::str::UniChar>>;

  public:
    KeyDataMapT data_;

    KeyProximityMap() = default;
    ~KeyProximityMap() = default;

    [[nodiscard]] bool isInProximity(const fl::str::UniChar& assumed, const fl::str::UniChar& actual) const noexcept {
        if (!data_.contains(assumed)) return false;
        auto keys = data_.at(assumed);
        return std::find(keys.begin(), keys.end(), actual) != keys.end();
    }

    void clear() noexcept {
        data_.clear();
    }
};

export void to_json(json& j, const KeyProximityMap& config) {
    j = json(config.data_);
}

export void from_json(const json& j, KeyProximityMap& config) {
    j.get_to(config.data_);
};

} // namespace fl::nlp
