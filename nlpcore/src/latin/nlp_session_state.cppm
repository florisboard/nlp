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
#include <map>
#include <memory>
#include <type_traits>

export module fl.nlp.core.latin:nlp_session_state;

import :dictionary;
import :entry_properties;
import fl.nlp.core.common;
import fl.string;

namespace fl::nlp {

export struct LatinNlpSessionState {
    static const LatinDictId USER_DICTIONARY_ID = 0;

    std::shared_ptr<LatinTrieNode> shared_data = std::make_shared<LatinTrieNode>();
    std::vector<std::unique_ptr<fl::nlp::LatinDictionary>> dictionaries;

    inline const fl::nlp::LatinDictionary* getDictionaryById(LatinDictId id) const {
        return dictionaries[id].get();
    }

    inline fl::nlp::LatinDictionary* getDictionaryById(LatinDictId id) {
        return dictionaries[id].get();
    }

    inline const fl::nlp::LatinDictionary* getUserDictionary() const {
        return getDictionaryById(USER_DICTIONARY_ID);
    }

    inline fl::nlp::LatinDictionary* getUserDictionary() {
        return getDictionaryById(USER_DICTIONARY_ID);
    }
};

} // namespace fl::nlp
