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

#include <argparse/argparse.hpp>
#include <fmt/core.h>
#include <unicode/unistr.h>

#include <termbox.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

export module fl.nlp.tools.core_ui;

import fl.icuext;
import fl.nlp.string;
import fl.nlp.core.common;
import fl.nlp.core.latin;
import fl.nlp.tools.common;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-avoid-magic-numbers"
#pragma ide diagnostic ignored "UnusedValue"

namespace fl::nlp::tools {

const std::string ICU_DATA_FILE_PATH = "build/debug/icu4c/host/share/icu_floris/72.1/icudt72l.dat";

const char* attrStatusSymbol(int32_t suggestion_attribute) noexcept {
    if (suggestion_attribute == fl::nlp::RESULT_ATTR_IN_THE_DICTIONARY) {
        return "✔";
    } else if (suggestion_attribute == fl::nlp::RESULT_ATTR_LOOKS_LIKE_TYPO) {
        return "✘";
    } else {
        return "?";
    }
}

auto attrStatusColor(int32_t suggestion_attribute) noexcept {
    if (suggestion_attribute == fl::nlp::RESULT_ATTR_IN_THE_DICTIONARY) {
        return TB_GREEN;
    } else if (suggestion_attribute == fl::nlp::RESULT_ATTR_LOOKS_LIKE_TYPO) {
        return TB_RED;
    } else {
        return TB_YELLOW;
    }
}

const char* inputShiftStateShorthand(fl::nlp::InputShiftState iss) noexcept {
    if (iss == fl::nlp::InputShiftState::UNSHIFTED) {
        return "US";
    } else if (iss == fl::nlp::InputShiftState::SHIFTED_MANUAL) {
        return "SM";
    } else if (iss == fl::nlp::InputShiftState::SHIFTED_AUTOMATIC) {
        return "SA";
    } else if (iss == fl::nlp::InputShiftState::CAPS_LOCK) {
        return "CL";
    } else {
        return "??";
    }
}

struct SuggestionRequestFlagsInput {
    uint8_t max_suggestion_count = 8;
    bool allow_possibly_offensive = false;
    bool override_hidden_flag = false;
    bool is_private_session = false;
    fl::nlp::InputShiftState iss_start = fl::nlp::InputShiftState::UNSHIFTED;
    fl::nlp::InputShiftState iss_current = fl::nlp::InputShiftState::UNSHIFTED;

    [[nodiscard]]
    fl::nlp::SuggestionRequestFlags toFlags() const noexcept {
        uint32_t flags = 0;
        flags |= (max_suggestion_count << fl::nlp::SuggestionRequestFlags::O_MAX_SUGGESTION_COUNT);
        if (allow_possibly_offensive) {
            flags |= fl::nlp::SuggestionRequestFlags::F_ALLOW_POSSIBLY_OFFENSIVE;
        }
        if (override_hidden_flag) {
            flags |= fl::nlp::SuggestionRequestFlags::F_OVERRIDE_HIDDEN_FLAG;
        }
        if (is_private_session) {
            flags |= fl::nlp::SuggestionRequestFlags::F_IS_PRIVATE_SESSION;
        }
        flags |= (static_cast<uint32_t>(iss_start) << fl::nlp::SuggestionRequestFlags::O_INPUT_SHIFT_STATE_START);
        flags |= (static_cast<uint32_t>(iss_current) << fl::nlp::SuggestionRequestFlags::O_INPUT_SHIFT_STATE_CURRENT);
        return flags;
    }
};

class BoundedBox {
  public:
    int x_min;
    int x_max;
    int y_min;
    int y_max;
    int width;
    int height;

    BoundedBox(int x, int y, int width, int height)
        : x_min(x), x_max(x + width - 1), y_min(y), y_max(y + height - 1), width(width), height(height) {}

    void drawOutline(const std::string& title) const noexcept {
        if (width < 4 || height < 4) return;
        auto adj_title = title.substr(0, std::min((int)title.length(), width - 4));
        int y = y_min;

        tb_printf(
            x_min, y++, 0, 0, "╔╡%s╞%s╗", adj_title.c_str(),
            fl::str::repeat("═", width - 4 - adj_title.length()).c_str()
        );
        for (; y < y_max; y++) {
            tb_print(x_min, y, 0, 0, "║");
            tb_print(x_max, y, 0, 0, "║");
        }
        tb_printf(x_min, y, 0, 0, "╚%s╝", fl::str::repeat("═", width - 2).c_str());
    }

    void drawTextStart(int line_num, const std::string& text) const noexcept {
        if (width < 4 || height < 4) return;
        if (line_num >= height - 3) return;
        auto adj_text = text.substr(0, std::min((int)text.length(), width - 4));
        tb_print(x_min + 2, y_min + 2 + line_num, 0, 0, adj_text.c_str());
    }

    void drawTextEnd(int line_num, const std::string& text) const noexcept {
        if (width < 4 || height < 4) return;
        if (line_num >= height - 3) return;
        auto adj_text = text.substr(0, std::min((int)text.length(), width - 4));
        tb_print(x_max - 1 - adj_text.length(), y_min + 2 + line_num, 0, 0, adj_text.c_str());
    }
};

struct CoreUiState {
    bool is_alive = true;
    bool is_suggestion_mode = true;
    int width = 0;
    int height = 0;

    fl::nlp::LatinNlpSession nlp_session;
    SuggestionRequestFlagsInput srf_input;
    icu::UnicodeString raw_input_buffer;
    std::string input_buffer;
    std::vector<std::string> input_words;
    std::vector<std::unique_ptr<fl::nlp::SuggestionCandidate>> suggestion_results;
    std::vector<std::string> prev_words;
};

void handleEvents(CoreUiState& state) noexcept;
void drawHeaderBox(const CoreUiState& state, const BoundedBox& bounds) noexcept;
void drawSuggestionRequestFlagsBox(const CoreUiState& state, const BoundedBox& bounds) noexcept;
void drawNlpSessionConfigBox(const CoreUiState& state, const BoundedBox& bounds) noexcept;
void drawSuggestionInputBox(CoreUiState& state, const BoundedBox& bounds) noexcept;

int mainCoreUi(const std::string& session_config_path) {
    if (U_FAILURE(fl::icuext::loadAndSetCommonData(ICU_DATA_FILE_PATH))) {
        std::cerr << "Fatal: Failed to load ICU data file! Aborting.\n";
        return 1;
    }

    CoreUiState state;
    state.nlp_session.loadConfigFromFile(session_config_path);

    tb_init();
    state.width = tb_width();
    state.height = tb_height();
    while (state.is_alive) {
        state.input_buffer.clear();
        state.raw_input_buffer.toUTF8String(state.input_buffer);

        // TODO: use proper word iterators for extracting words
        fl::str::split(state.input_buffer, ' ', state.input_words);
        state.prev_words.clear();
        for (std::size_t i = 0; i + 1 < state.input_words.size(); i++) {
            state.prev_words.push_back(state.input_words[i]);
        }

        // TODO: clean up this hardcoded mess of coords
        if (state.width > 120) {
            drawHeaderBox(state, BoundedBox(0, 0, 44, 6));
            drawSuggestionRequestFlagsBox(state, BoundedBox(state.width - 36, 0, 36, 9));
            drawNlpSessionConfigBox(state, BoundedBox(state.width - 36, 9, 36, 4));
            drawSuggestionInputBox(state, BoundedBox(45, 0, state.width - 45 - 37, state.height));
        } else if (state.width > 80) {
            drawHeaderBox(state, BoundedBox(0, 0, 44, 6));
            drawSuggestionRequestFlagsBox(state, BoundedBox(state.width - 36, 0, 36, 9));
            drawNlpSessionConfigBox(state, BoundedBox(state.width - 36, 9, 36, 4));
            drawSuggestionInputBox(state, BoundedBox(0, 9 + 4, state.width, state.height - 9 - 4));
        } else {
            drawHeaderBox(state, BoundedBox(0, 0, state.width, 6));
            drawSuggestionRequestFlagsBox(state, BoundedBox(0, 6, state.width, 9));
            drawNlpSessionConfigBox(state, BoundedBox(0, 6 + 9, state.width, 4));
            drawSuggestionInputBox(state, BoundedBox(0, 6 + 9 + 4, state.width, state.height - 5 - 9 - 4));
        }

        tb_present();
        handleEvents(state);
        tb_clear();
    }
    tb_shutdown();

    state.nlp_session.user_dictionary->persistToDisk();

    return 0;
}

void handleEvents(CoreUiState& state) noexcept {
    tb_event ev;
    tb_poll_event(&ev);
    if (ev.type == TB_EVENT_RESIZE) {
        state.width = ev.w;
        state.height = ev.h;
    } else if (ev.type == TB_EVENT_KEY) {
        if (ev.key == TB_KEY_BACKSPACE || ev.key == TB_KEY_BACKSPACE2) {
            if (!state.raw_input_buffer.isEmpty()) {
                state.raw_input_buffer.remove(state.raw_input_buffer.countChar32() - 1, 1);
            }
        } else if (ev.key == TB_KEY_ENTER) {
            if (!state.raw_input_buffer.isEmpty()) {
                state.nlp_session.train(state.input_words, 3);
            }
            state.raw_input_buffer.remove();
        } else if (ev.key == TB_KEY_CTRL_C) {
            state.is_alive = false;
        } else if (ev.key == TB_KEY_CTRL_D) {
            state.is_suggestion_mode = !state.is_suggestion_mode;
        } else if (ev.key == TB_KEY_F1) {
            bool increment = (ev.mod & TB_MOD_SHIFT) == 0;
            // Note: possible over/underflow is on purpose in this specific case!!
            if (increment) {
                state.srf_input.max_suggestion_count++;
            } else {
                state.srf_input.max_suggestion_count--;
            }
        } else if (ev.key == TB_KEY_F2) {
            state.srf_input.allow_possibly_offensive = !state.srf_input.allow_possibly_offensive;
        } else if (ev.key == TB_KEY_F3) {
            state.srf_input.override_hidden_flag = !state.srf_input.override_hidden_flag;
        } else if (ev.key == TB_KEY_F4) {
            state.srf_input.is_private_session = !state.srf_input.is_private_session;
        } else if (ev.key == TB_KEY_F5) {
            bool increment = (ev.mod & TB_MOD_SHIFT) == 0;
            auto iss_val = static_cast<uint8_t>(state.srf_input.iss_start);
            if (increment) {
                state.srf_input.iss_start = static_cast<fl::nlp::InputShiftState>(iss_val > 2 ? 0 : iss_val + 1);
            } else {
                state.srf_input.iss_start = static_cast<fl::nlp::InputShiftState>(iss_val < 1 ? 3 : iss_val - 1);
            }
        } else if (ev.key == TB_KEY_F6) {
            bool increment = (ev.mod & TB_MOD_SHIFT) == 0;
            auto iss_val = static_cast<uint8_t>(state.srf_input.iss_current);
            if (increment) {
                state.srf_input.iss_current = static_cast<fl::nlp::InputShiftState>(iss_val > 2 ? 0 : iss_val + 1);
            } else {
                state.srf_input.iss_current = static_cast<fl::nlp::InputShiftState>(iss_val < 1 ? 3 : iss_val - 1);
            }
        } else if (ev.key == TB_KEY_F12) {
            state.nlp_session.config.key_proximity_checker.enabled =
                !state.nlp_session.config.key_proximity_checker.enabled;
        } else if (ev.ch != 0x0) {
            state.raw_input_buffer.append(static_cast<UChar32>(ev.ch));
        }
    }
}

void drawHeaderBox(const CoreUiState& state, const BoundedBox& bounds) noexcept {
    int line = 0;
    bounds.drawOutline("FlorisNLP Core Debug Frontend");
    bounds.drawTextStart(line++, "CTRL+C  quit");
    bounds.drawTextStart(line++, "CTRL+D  toggle spell check/suggestion");
    bounds.drawTextStart(line++, "ENTER   train user dictionary with input");
}

void drawSuggestionRequestFlagsBox(const CoreUiState& state, const BoundedBox& bounds) noexcept {
    int line = 0;
    bounds.drawOutline("SuggestionRequestFlags");
    bounds.drawTextStart(line, "F1   maxSuggestionCount");
    bounds.drawTextEnd(line++, fmt::format("= {}", state.srf_input.max_suggestion_count));
    bounds.drawTextStart(line, "F2   allowPossiblyOffensive");
    bounds.drawTextEnd(line++, fmt::format("= {}", state.srf_input.allow_possibly_offensive ? 'Y' : 'N'));
    bounds.drawTextStart(line, "F3   overrideHiddenFlag");
    bounds.drawTextEnd(line++, fmt::format("= {}", state.srf_input.override_hidden_flag ? 'Y' : 'N'));
    bounds.drawTextStart(line, "F4   isPrivateSession");
    bounds.drawTextEnd(line++, fmt::format("= {}", state.srf_input.override_hidden_flag ? 'Y' : 'N'));
    bounds.drawTextStart(line, "F5   inputShiftStateStart");
    bounds.drawTextEnd(line++, fmt::format("= {}", inputShiftStateShorthand(state.srf_input.iss_start)));
    bounds.drawTextStart(line, "F6   inputShiftStateCurrent");
    bounds.drawTextEnd(line++, fmt::format("= {}", inputShiftStateShorthand(state.srf_input.iss_current)));
}

void drawNlpSessionConfigBox(const CoreUiState& state, const BoundedBox& bounds) noexcept {
    int line = 0;
    bounds.drawOutline("NlpSessionConfig");
    bounds.drawTextStart(line, "F12  keyProximityChecker");
    bounds.drawTextEnd(line++, fmt::format("= {}", state.nlp_session.config.key_proximity_checker.enabled ? 'Y' : 'N'));
}

void drawSuggestionInputBox(CoreUiState& state, const BoundedBox& bounds) noexcept {
    int line = 0;
    int char_count = state.raw_input_buffer.countChar32();
    tb_set_cursor(bounds.x_min + 9 + char_count, bounds.y_min + 2);
    bounds.drawOutline("Suggestion Input");
    bounds.drawTextStart(line++, fmt::format("Input: {}", state.input_buffer.c_str()));
    bounds.drawTextStart(line++, fmt::format("Characters: {} | Bytes: {}", char_count, state.input_buffer.length()));
    line++;

    auto flags = state.srf_input.toFlags();
    if (state.is_suggestion_mode) {
        auto start = std::chrono::high_resolution_clock::now();
        state.nlp_session.suggest(
            state.input_words[state.input_words.size() - 1], state.prev_words, flags, state.suggestion_results
        );
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        bounds.drawTextStart(
            line++, fmt::format("Suggested words ({}, {}ms):", state.suggestion_results.size(), duration.count())
        );
        for (auto& result : state.suggestion_results) {
            bounds.drawTextStart(
                line++, fmt::format("{} | e={} | c={}", result->text.c_str(), result->edit_distance, result->confidence)
            );
        }
    } else {
        bounds.drawTextStart(line++, "Spelling results:");
        for (auto& input_word : state.input_words) {
            auto result = state.nlp_session.spell(input_word, state.prev_words, state.prev_words, flags);
            auto color = attrStatusColor(result.suggestion_attributes);
            std::stringstream ss;
            ss << " " << input_word << "  ->  ";
            if (!result.suggestions.empty()) {
                std::size_t i = 0;
                for (auto& suggestion : result.suggestions) {
                    if (i++ > 0) {
                        ss << " , ";
                    }
                    ss << suggestion;
                }
            }
            bounds.drawTextStart(
                line++, fmt::format("  {}{}", attrStatusSymbol(result.suggestion_attributes), ss.str())
            );
        }
    }
}

export class CoreUiActionConfig : public ActionConfig {
  public:
    CoreUiActionConfig() : ActionConfig("core-ui") {};

    void initArgumentConfig(argparse::ArgumentParser& arg_parser) override {
        arg_parser.add_description("Debug frontend UI for the NLP core");
        DictionaryArgsUtils::initArgumentConfig(arg_parser);
    }

    int runAction(argparse::ArgumentParser& arg_parser) override {
        auto session_config_path = DictionaryArgsUtils::readArgumentsAndCheckFiles(arg_parser);
        return mainCoreUi(session_config_path);
    }
};

} // namespace fl::nlp::tools

#pragma clang diagnostic pop
