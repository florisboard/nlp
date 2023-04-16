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

export module fl.nlp.core.common:key_proximity_checker;

import fl.string;

namespace fl::nlp {

using json = nlohmann::json;

export class KeyProximityChecker {
  public:
    using MappingT = std::unordered_map<fl::str::UniChar, std::vector<fl::str::UniChar>>;

    bool enabled_ = false;
    MappingT mapping_;

    KeyProximityChecker() = default;
    ~KeyProximityChecker() = default;

    [[nodiscard]]
    inline bool isInProximity(const fl::str::UniChar& assumed, const fl::str::UniChar& actual) const noexcept {
        if (!enabled_) return false;
        return isInProximityInternal(assumed, actual);
    }

    void reset() noexcept {
        enabled_ = false;
        mapping_.clear();
    }

  private:
    [[nodiscard]]
    bool isInProximityInternal(const fl::str::UniChar& assumed, const fl::str::UniChar& actual) const noexcept {
        auto it = mapping_.find(assumed);
        if (it != mapping_.end()) {
            auto& vec = it->second;
            return std::find(vec.begin(), vec.end(), actual) != vec.end();
        } else {
            return false;
        }
    }
};

export void to_json(json& j, const KeyProximityChecker& checker) {
    j = json {{"enabled", checker.enabled_}, {"mapping", checker.mapping_}};
}

export void from_json(const json& j, KeyProximityChecker& checker) {
    checker.enabled_ = j.value("enabled", false);
    checker.mapping_ = j.value("mapping", KeyProximityChecker::MappingT());
};

} // namespace fl::nlp
