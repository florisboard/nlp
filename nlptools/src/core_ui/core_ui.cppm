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

#include <unicode/unistr.h>

#include <termbox.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

export module fl.nlp.tools.core_ui;

import fl.nlp.icuext;
import fl.nlp.string;
import fl.nlp.core.common;
import fl.nlp.core.latin;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-avoid-magic-numbers"
#pragma ide diagnostic ignored "UnusedValue"

namespace fl::nlp::tools::core_ui {

const std::string ICU_DATA_FILE_PATH = "build/debug/icu4c/host/share/icu_floris/71.1/icudt71l.dat";
const std::string NLP_SESSION_CONFIG = "data/nlp_session_config.json";

export int handleAction(const std::vector<std::string>& flags) noexcept;
export int printUsage(const char* arg0) noexcept;

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
void drawHeaderBox(const CoreUiState& state) noexcept;
void drawSuggestionRequestFlagsBox(const CoreUiState& state) noexcept;
void drawNlpSessionConfigBox(const CoreUiState& state) noexcept;
void drawSuggestionInputBox(CoreUiState& state) noexcept;

// TODO: do further cleanup of CoreUI codebase
int mainCoreUi(const std::string& fldic_path) noexcept {
    if (U_FAILURE(fl::nlp::icuext::loadAndSetCommonData(ICU_DATA_FILE_PATH))) {
        std::cerr << "Fatal: Failed to load ICU data file! Aborting.\n";
        return 1;
    }

    CoreUiState state;
    state.nlp_session.loadConfigFromFile(NLP_SESSION_CONFIG);
    try {
        state.nlp_session.loadBaseDictionary(fldic_path);
    } catch (const std::runtime_error& e) {
        std::cerr << "Fatal: " << e.what() << " Aborting.\n";
        return 1;
    }

    tb_init();
    state.width = tb_width();
    state.height = tb_height();
    while (state.is_alive) {
        state.input_buffer.clear();
        state.raw_input_buffer.toUTF8String(state.input_buffer);
        fl::str::split(state.input_buffer, ' ', state.input_words);
        state.prev_words.clear();
        for (std::size_t i = 0; i + 1 < state.input_words.size(); i++) {
            state.prev_words.push_back(state.input_words[i]);
        }
        drawHeaderBox(state);
        drawSuggestionRequestFlagsBox(state);
        drawNlpSessionConfigBox(state);
        drawSuggestionInputBox(state);
        handleEvents(state);
        tb_clear();
    }
    tb_shutdown();

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

void drawHeaderBox(const CoreUiState& state) noexcept {
    int x = 0;
    int y = 0;

    tb_printf(x, y++, 0, 0, "╔╡FlorisNLP Core Debug Frontend╞%s╗", fl::str::repeat("═", state.width - x - 33).c_str());
    tb_printf(x, y, 0, 0, "║");
    tb_printf(state.width - 1, y++, 0, 0, "║");
    tb_printf(x, y, 0, 0, "║ CTRL+C to quit | CTRL+D to toggle spell check/suggestion");
    tb_printf(state.width - 1, y++, 0, 0, "║");
    tb_printf(x, y++, 0, 0, "╚%s╝", fl::str::repeat("═", state.width - x - 2).c_str());
}

void drawSuggestionRequestFlagsBox(const CoreUiState& state) noexcept {
    int x = 0;
    int y = 4;

    tb_printf(x, y++, 0, 0, "╔╡SuggestionRequestFlags╞══════════╗");
    tb_printf(x, y++, 0, 0, "║                                  ║");
    int sug_count = state.srf_input.max_suggestion_count;
    const char* spaces = sug_count >= 100 ? " " : (sug_count >= 10 ? "  " : "   ");
    tb_printf(x, y++, 0, 0, "║ F1   maxSuggestionCount   %s= %d ║", spaces, state.srf_input.max_suggestion_count);
    tb_printf(x, y++, 0, 0, "║ F2   allowPossiblyOffensive  = %c ║",
              state.srf_input.allow_possibly_offensive ? 'Y' : 'N');
    tb_printf(x, y++, 0, 0, "║ F3   overrideHiddenFlag      = %c ║", state.srf_input.override_hidden_flag ? 'Y' : 'N');
    tb_printf(x, y++, 0, 0, "║ F4   isPrivateSession        = %c ║", state.srf_input.is_private_session ? 'Y' : 'N');
    tb_printf(x, y++, 0, 0, "║ F5   inputShiftStateStart   = %s ║",
              inputShiftStateShorthand(state.srf_input.iss_start));
    tb_printf(x, y++, 0, 0, "║ F6   inputShiftStateCurrent = %s ║",
              inputShiftStateShorthand(state.srf_input.iss_current));
    tb_printf(x, y++, 0, 0, "╚══════════════════════════════════╝");
}

void drawNlpSessionConfigBox(const CoreUiState& state) noexcept {
    int x = 0;
    int y = 13;

    tb_printf(x, y++, 0, 0, "╔╡NlpSessionConfig╞════════════════╗");
    tb_printf(x, y++, 0, 0, "║                                  ║");
    tb_printf(x, y++, 0, 0, "║ F12  keyProximityChecker     = %c ║",
              state.nlp_session.config.key_proximity_checker.enabled ? 'Y' : 'N');
    tb_printf(x, y++, 0, 0, "╚══════════════════════════════════╝");
}

void drawSuggestionInputBox(CoreUiState& state) noexcept {
    int x = 37;
    int y = 4;

    int char_count = state.raw_input_buffer.countChar32();
    tb_set_cursor(x + 9 + char_count, y + 2);
    tb_printf(x, y++, 0, 0, "╔╡Suggestion Input╞%s╗", fl::str::repeat("═", state.width - x - 20).c_str());
    tb_printf(x, y++, 0, 0, "║%*s║", state.width - x - 2, "");
    tb_printf(x, y, 0, 0, "║ Input: %s", state.input_buffer.c_str());
    tb_printf(state.width - 1, y++, 0, 0, "║");
    tb_printf(x, y, 0, 0, "║ Characters: %d | Bytes: %d", char_count, state.input_buffer.length());
    tb_printf(state.width - 1, y++, 0, 0, "║");
    tb_printf(x, y, 0, 0, "║");
    tb_printf(state.width - 1, y++, 0, 0, "║");

    auto flags = state.srf_input.toFlags();
    if (state.is_suggestion_mode) {
        auto start = std::chrono::high_resolution_clock::now();
        state.nlp_session.suggest(state.input_words[state.input_words.size() - 1], state.prev_words, flags,
                                  state.suggestion_results);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        tb_printf(x, y, 0, 0, "║ Suggested words (%d, %dms):", state.suggestion_results.size(), duration.count());
        tb_printf(state.width - 1, y++, 0, 0, "║");
        for (auto& result : state.suggestion_results) {
            tb_printf(x, y, 0, 0, "║ %s | e=%d | c=%.4f", result->text.c_str(), result->edit_distance,
                      result->confidence);
            tb_printf(state.width - 1, y++, 0, 0, "║");
        }
    } else {
        tb_printf(x, y, 0, 0, "║ Spelling results:");
        tb_printf(state.width - 1, y++, 0, 0, "║");
        for (auto& input_word : state.input_words) {
            auto result = state.nlp_session.spell(input_word, state.prev_words, state.prev_words, flags);
            tb_printf(x, y, 0, 0, "║  ");
            auto color = attrStatusColor(result.suggestion_attributes);
            tb_printf(x + 3, y, color, 0, "%s", attrStatusSymbol(result.suggestion_attributes));
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
            auto output = ss.str();
            auto max_size = state.width - x - 4;
            if (output.size() > max_size) {
                output.resize(max_size);
            }
            tb_printf(x + 5, y, 0, 0, output.c_str());
            tb_printf(state.width - 1, y++, 0, 0, "║");
        }
    }
    tb_printf(x, y++, 0, 0, "╚%s╝", fl::str::repeat("═", state.width - x - 2).c_str());
    tb_present();
}

int handleAction(const std::vector<std::string>& flags) noexcept {
    std::string fldic_path;
    if (!flags.empty()) {
        fldic_path = flags[0];
    }

    fl::str::trim(fldic_path);
    if (fldic_path.empty()) {
        std::cerr << "Fatal: No dictionary path specified! Aborting.\n";
        return 1;
    } else if (!std::filesystem::exists(fldic_path)) {
        std::cerr << "Fatal: Given dictionary path '" << fldic_path << "' does not exist! Aborting.\n";
        return 1;
    }

    return mainCoreUi(fldic_path);
}

int printUsage(const char* arg0) noexcept {
    std::cout << "TODO!!!\n";
    return 0;
}

} // namespace fl::nlp::tools::core_ui

#pragma clang diagnostic pop
