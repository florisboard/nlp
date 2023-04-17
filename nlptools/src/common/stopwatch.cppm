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

#include <chrono>

export module fl.nlp.tools.common:stopwatch;

namespace fl::nlp::tools {

export class Stopwatch {
  private:
    using TimePointT = std::chrono::time_point<std::chrono::high_resolution_clock>;

    TimePointT start_time_;
    TimePointT end_time_;

  public:
    void start() noexcept {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    void stop() noexcept {
        end_time_ = std::chrono::high_resolution_clock::now();
    }

    [[nodiscard]]
    std::int64_t elapsed() const noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_time_ - start_time_).count();
    }
};

} // namespace fl::nlp::tools
