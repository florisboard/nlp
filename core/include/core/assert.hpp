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

#ifndef __FLORISNLP_CORE_ASSERT_H__
#define __FLORISNLP_CORE_ASSERT_H__

#include <stdexcept>

namespace fl {

class assertion_error : public std::exception {
  public:
    explicit assertion_error(const char* msg) : exception(), __msg(msg) {};
    ~assertion_error() = default;

    const char* what() const noexcept {
        return __msg;
    }

  private:
    const char* __msg;
};

inline void assert(bool condition) {
    if (!condition) throw assertion_error("");
}

inline void assert(bool condition, const char* msg) {
    if (!condition) throw assertion_error(msg);
}

} // namespace fl

#endif
