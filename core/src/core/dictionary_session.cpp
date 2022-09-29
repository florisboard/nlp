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

#include "core/dictionary_session.hpp"

#include "core/dictionary.hpp"

namespace fl::nlp {

void dictionary_session::load_base_dictionary(std::filesystem::path& dict_path) {
    auto base_dict = std::make_unique<dictionary>(dict_path);
    base_dictionaries.push_back(std::move(base_dict));
}

void dictionary_session::load_user_dictionary(std::filesystem::path& dict_path) {
    user_dictionary = std::make_unique<mutable_dictionary>(dict_path);
}

} // namespace fl::nlp
