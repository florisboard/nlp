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
#include <memory>
#include <numeric>
#include <string>
#include <vector>

export module fl.nlp.core.common:suggestion;

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
 * |0       |        |    1111|        | Maximum ngram level (2-15). Values 0 and 1 cause word history to be ignored.
 * |0       |        |  11    |        | Input shift state (0-3) at the start of the current word.
 * |0       |        |11      |        | Input shift state (0-3) at the current cursor position.
 * |0       |       1|        |        | Flag indicating if possibly offensive words should be suggested.
 * |0       |      1 |        |        | Flag indicating if user-hidden words should still be displayed.
 * |0       |     1  |        |        | Flag indicating if the current request is within a private session.
 */
export class SuggestionRequestFlags {
  public:
    static const uint32_t M_MAX_SUGGESTION_COUNT = 0x000000FF;
    static const uint32_t O_MAX_SUGGESTION_COUNT = std::countr_zero(M_MAX_SUGGESTION_COUNT);
    static const uint32_t M_MAX_NGRAM_LEVEL = 0x00000F00;
    static const uint32_t O_MAX_NGRAM_LEVEL = std::countr_zero(M_MAX_NGRAM_LEVEL);
    static const uint32_t M_INPUT_SHIFT_STATE_START = 0x00003000;
    static const uint32_t O_INPUT_SHIFT_STATE_START = std::countr_zero(M_INPUT_SHIFT_STATE_START);
    static const uint32_t M_INPUT_SHIFT_STATE_CURRENT = 0x0000C000;
    static const uint32_t O_INPUT_SHIFT_STATE_CURRENT = std::countr_zero(M_INPUT_SHIFT_STATE_CURRENT);
    static const uint32_t F_ALLOW_POSSIBLY_OFFENSIVE = 0x00010000;
    static const uint32_t F_OVERRIDE_HIDDEN_FLAG = 0x00020000;
    static const uint32_t F_IS_PRIVATE_SESSION = 0x00040000;

    SuggestionRequestFlags(uint32_t flags) : flags_(flags) {}; // NOLINT(google-explicit-constructor)
    SuggestionRequestFlags(int32_t flags) : flags_(flags) {};  // NOLINT(google-explicit-constructor)
    SuggestionRequestFlags(const SuggestionRequestFlags&) = default;
    SuggestionRequestFlags(SuggestionRequestFlags&&) = default;
    ~SuggestionRequestFlags() = default;

    SuggestionRequestFlags& operator=(const SuggestionRequestFlags&) = default;
    SuggestionRequestFlags& operator=(SuggestionRequestFlags&&) = default;

    [[nodiscard]]
    inline int maxSuggestionCount() const noexcept {
        return (flags_ & M_MAX_SUGGESTION_COUNT) >> O_MAX_SUGGESTION_COUNT;
    }

    [[nodiscard]]
    inline int maxNgramLevel() const noexcept {
        return (flags_ & M_MAX_NGRAM_LEVEL) >> O_MAX_NGRAM_LEVEL;
    }

    [[nodiscard]]
    inline InputShiftState inputShiftStateStart() const noexcept {
        return static_cast<InputShiftState>((flags_ & M_INPUT_SHIFT_STATE_START) >> O_INPUT_SHIFT_STATE_START);
    }

    [[nodiscard]]
    inline InputShiftState inputShiftStateCurrent() const noexcept {
        return static_cast<InputShiftState>((flags_ & M_INPUT_SHIFT_STATE_CURRENT) >> O_INPUT_SHIFT_STATE_CURRENT);
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

    explicit inline operator uint32_t() const noexcept {
        return flags_;
    }

    explicit inline operator int32_t() const noexcept {
        return flags_;
    }

  private:
    uint32_t flags_;
};

export struct SuggestionCandidate {
    constexpr static const double MIN_CONFIDENCE = 0.0;
    // Everything above 0.9 to 1.0 is reserved for special suggestions such as contacts, clipboard, etc., which is not
    // handled in the native implementation
    constexpr static const double MAX_CONFIDENCE = 0.9;

    std::string text;
    std::string secondary_text;
    double confidence = MIN_CONFIDENCE;
    bool is_eligible_for_auto_commit = false;
    bool is_eligible_for_user_removal = true;
};

export using SuggestionResults = std::vector<std::unique_ptr<SuggestionCandidate>>;

struct TransientSuggestionCandidate {
    std::string text_;
    double confidence_ = SuggestionCandidate::MIN_CONFIDENCE;
    bool is_eligible_for_auto_commit_ = false;
    bool is_eligible_for_user_removal_ = true;
};

export template<typename NodeT>
class TransientSuggestionResults {
  private:
    using CandidateT = TransientSuggestionCandidate;
    using CandidatePtrT = std::unique_ptr<TransientSuggestionCandidate>;
    using CandidatesListT = std::vector<CandidatePtrT>;

  public:
    void insert(CandidateT&& candidate, const SuggestionRequestFlags& flags) noexcept {
        /*auto existing_candidate = std::find_if(candidates_.begin(), candidates_.end(), [&](auto& it) {
            return it->node_ == candidate.node_;
        });
        if (existing_candidate != candidates_.end()) {
            auto new_confidence = std::midpoint(candidate.confidence_, (*existing_candidate)->confidence_);
            (*existing_candidate)->confidence_ = new_confidence;
        } else*/
        if (candidate.confidence_ < min_inserted_confidence_ && candidates_.size() > flags.maxSuggestionCount()) {
            return;
        } else {
            auto candidate_ptr = std::make_unique<CandidateT>(candidate);
            candidates_.push_back(std::move(candidate_ptr));
        }
        std::sort(candidates_.begin(), candidates_.end(), suggestions_sorter);
        if (candidates_.size() > flags.maxSuggestionCount()) {
            candidates_.erase(candidates_.end() - 1);
        }
        min_inserted_confidence_ = candidates_.back()->confidence_;
    }

    void clear() noexcept {
        candidates_.clear();
    }

    inline const CandidatesListT& get() const noexcept {
        return candidates_;
    }

    inline const CandidatePtrT& top() const {
        return candidates_[0];
    }

    inline std::size_t size() const noexcept {
        return candidates_.size();
    }

  private:
    CandidatesListT candidates_;
    double min_inserted_confidence_ = 0.0;

    static bool suggestions_sorter(
        const std::unique_ptr<TransientSuggestionCandidate>& a,
        const std::unique_ptr<TransientSuggestionCandidate>& b
    ) {
        /*if (a->edit_distance == b->edit_distance) {
            return a->confidence > b->confidence;
        }
        return a->edit_distance < b->edit_distance && a->confidence * 100.0 > b->confidence;*/
        return a->confidence_ > b->confidence_;
    }
};

} // namespace fl::nlp
