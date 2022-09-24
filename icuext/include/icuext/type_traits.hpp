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

#ifndef __FLORISNLP_ICUEXT_TYPE_TRAITS_H__
#define __FLORISNLP_ICUEXT_TYPE_TRAITS_H__

#include <string>

namespace icuext {

template<typename T>
struct get_extents {
    using type = T;
};

template<typename T>
struct get_extents<std::basic_string<T>> {
    using type = T;
};

template<typename T>
using get_extents_t = typename get_extents<T>::type;

} // namespace icuext

#endif
