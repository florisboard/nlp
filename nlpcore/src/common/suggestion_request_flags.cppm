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

#include <bit>
#include <cstdint>

export module fl.nlp.core.common:suggestion_request_flags;

namespace fl::nlp {

/**
 * @see
 * https://github.com/florisboard/florisboard/blob/master/app/src/main/kotlin/dev/patrickgold/florisboard/ime/input/InputShiftState.kt
 */
export enum class InputShiftState {
    UNSHIFTED = 0,
    SHIFTED_MANUAL = 1,
    SHIFTED_AUTOMATIC = 2,
    CAPS_LOCK = 3,
};

/**
 * Class which allows to read 31-bit binary suggestion request flags. Note that the signed bit MUST always be 0, else
 * the behavior of this class is undefined.
 *
 * Note that the layout of the suggestion request flags is not meant for serialization as this is only used for
 * runtime inter-process communication. The layout may change during any new version of this project.
 *
 * Layout of the binary flags:
 * | Byte 3 | Byte 2 | Byte 1 | Byte 0 |
 * |--------|--------|--------|--------|
 * |0       |        |        |11111111| Maximum suggestion count (1-255), 0 indicating no limit.
 * |0       |        |       1|        | Flag indicating if possibly offensive words should be suggested.
 * |0       |        |      1 |        | Flag indicating if user-hidden words should still be displayed.
 * |0       |        |     1  |        | Flag indicating if the current request is within a private session.
 * |0       |      11|        |        | Input shift state (0-3) at the start of the current word.
 * |0       |    11  |        |        | Input shift state (0-3) at the current cursor position.
 * |0       |        |        |        |
 */
export class SuggestionRequestFlags {
  public:
    static const uint32_t M_MAX_SUGGESTION_COUNT = 0x000000FF;
    static const uint32_t O_MAX_SUGGESTION_COUNT = std::countr_zero(M_MAX_SUGGESTION_COUNT);
    static const uint32_t F_ALLOW_POSSIBLY_OFFENSIVE = 0x00000100;
    static const uint32_t F_OVERRIDE_HIDDEN_FLAG = 0x00000200;
    static const uint32_t F_IS_PRIVATE_SESSION = 0x00000400;
    static const uint32_t M_INPUT_SHIFT_STATE_START = 0x00030000;
    static const uint32_t O_INPUT_SHIFT_STATE_START = std::countr_zero(M_INPUT_SHIFT_STATE_START);
    static const uint32_t M_INPUT_SHIFT_STATE_CURRENT = 0x000C0000;
    static const uint32_t O_INPUT_SHIFT_STATE_CURRENT = std::countr_zero(M_INPUT_SHIFT_STATE_CURRENT);

    SuggestionRequestFlags(uint32_t flags) : flags_(flags) {}; // NOLINT(google-explicit-constructor)
    SuggestionRequestFlags(int32_t flags) : flags_(flags) {}; // NOLINT(google-explicit-constructor)
    SuggestionRequestFlags(const SuggestionRequestFlags&) = default;
    SuggestionRequestFlags(SuggestionRequestFlags&&) = default;
    ~SuggestionRequestFlags() = default;

    SuggestionRequestFlags& operator=(const SuggestionRequestFlags&) = default;
    SuggestionRequestFlags& operator=(SuggestionRequestFlags&&) = default;

    [[nodiscard]]
    inline uint8_t maxSuggestionCount() const noexcept {
        return (flags_ & M_MAX_SUGGESTION_COUNT) >> O_MAX_SUGGESTION_COUNT;
    }

    [[nodiscard]]
    inline bool allowPossiblyOffensive() const noexcept {
        return (flags_ & F_ALLOW_POSSIBLY_OFFENSIVE) != 0;
    }

    [[nodiscard]]
    inline bool overrideHiddenFlag() const noexcept {
        return (flags_ & F_OVERRIDE_HIDDEN_FLAG) != 0;
    }

    [[nodiscard]]
    inline bool isPrivateSession() const noexcept {
        return (flags_ & F_IS_PRIVATE_SESSION) != 0;
    }

    [[nodiscard]]
    inline InputShiftState inputShiftStateStart() const noexcept {
        return static_cast<InputShiftState>((flags_ & M_INPUT_SHIFT_STATE_START) >> O_INPUT_SHIFT_STATE_START);
    }

    [[nodiscard]]
    inline InputShiftState inputShiftStateCurrent() const noexcept {
        return static_cast<InputShiftState>((flags_ & M_INPUT_SHIFT_STATE_CURRENT) >> O_INPUT_SHIFT_STATE_CURRENT);
    }

    explicit inline operator uint32_t() const noexcept {
        return flags_;
    }

    explicit inline operator int32_t() const noexcept {
        return flags_;
    }

  private:
    uint32_t flags_;
};

} // namespace fl::nlp
