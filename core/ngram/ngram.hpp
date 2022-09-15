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

#ifndef _FLORISNLP_CORE_NGRAM_NGRAM
#define _FLORISNLP_CORE_NGRAM_NGRAM

#include <cstdint>
#include <string>

namespace floris::nlp {

using ucodepoint_t = uint32_t;
static const ucodepoint_t UCODEPOINT_MIN = 0x0;
static const ucodepoint_t UCODEPOINT_MAX = 0x10FFFF;

using ustring_t = std::basic_string<ucodepoint_t>;
using ustring_view_t = std::basic_string_view<ucodepoint_t>;

using score_t = uint16_t;
static const score_t SCORE_MIN = 0x0;
static const score_t SCORE_MAX = UINT16_MAX;

}

#endif
