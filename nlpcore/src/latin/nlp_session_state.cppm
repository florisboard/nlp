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

#include <functional>
#include <memory>
#include <vector>

export module fl.nlp.core.latin:nlp_session_state;

import :dictionary;

namespace fl::nlp {

export struct LatinNlpSessionState {
    std::vector<std::unique_ptr<fl::nlp::LatinDictionary>> base_dictionaries;
    std::unique_ptr<fl::nlp::LatinDictionary> user_dictionary = nullptr;

    void forEachDictionary(const std::function<void(const LatinDictionary&)>& action) const noexcept {
        for (auto& base_dictionary : base_dictionaries) {
            action(*base_dictionary);
        }
        if (user_dictionary != nullptr) {
            action(*user_dictionary);
        }
    }
};

} // namespace fl::nlp
