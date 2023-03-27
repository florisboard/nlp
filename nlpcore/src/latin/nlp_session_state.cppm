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
#include <map>
#include <type_traits>

export module fl.nlp.core.latin:nlp_session_state;

import :dictionary;
import :entry_properties;
import fl.nlp.core.common;
import fl.nlp.string;

namespace fl::nlp {

export struct LatinNlpSessionState {
    using DictIdT = std::uint8_t;
    using SharedNodeT = TrieNode<fl::str::UniChar, EntryProperties, DictIdT>;

    static const DictIdT USER_DICTIONARY_ID = 0;

    std::shared_ptr<SharedNodeT> shared_data = std::make_shared<SharedNodeT>();
    std::map<DictIdT, std::unique_ptr<fl::nlp::LatinDictionary>> dictionaries;

    inline fl::nlp::LatinDictionary* getDictionaryById(DictIdT id) {
        return dictionaries.at(id).get();
    }

    inline fl::nlp::LatinDictionary* getUserDictionary() {
        return getDictionaryById(USER_DICTIONARY_ID);
    }
};

} // namespace fl::nlp
