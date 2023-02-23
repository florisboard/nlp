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

namespace fl::nlp::tools::core_ui {

static const std::string ICU_DATA_FILE_PATH = "build/debug/icu4c/host/share/icu_floris/71.1/icudt71l.dat";
static const std::string KEY_MAPPING_FILE = "data/qwerty_proximity_map.json";

export int handleAction(const std::vector<std::string>& flags) noexcept;

export int printUsage(const char* arg0) noexcept;

const char* attrStatusSymbol(int32_t suggestion_attribute) noexcept {
    if (suggestion_attribute == fl::nlp::RESULT_ATTR_IN_THE_DICTIONARY) {
        return "✅";
    } else if (suggestion_attribute == fl::nlp::RESULT_ATTR_LOOKS_LIKE_TYPO) {
        return "❌";
    } else {
        return "❔";
    }
}

// TODO: clean up code and restructure UI
int mainCoreUi(const std::string& fldic_path) noexcept {
    if (U_FAILURE(fl::nlp::icuext::loadAndSetCommonData(ICU_DATA_FILE_PATH))) {
        std::cerr << "Fatal: Failed to load ICU data file! Aborting.\n";
        return 1;
    }

    fl::nlp::SuggestionRequestFlags flags(0);
    std::vector<std::unique_ptr<fl::nlp::SuggestionCandidate>> suggestion_results;
    std::vector<std::string> prev_words;
    fl::nlp::LatinNlpSession nlp_session;
    nlp_session.key_proximity_map.loadFromFile(KEY_MAPPING_FILE);
    try {
        nlp_session.loadBaseDictionary(fldic_path);
    } catch (const std::runtime_error& e) {
        std::cerr << "Fatal: " << e.what() << " Aborting.\n";
        return 1;
    }

    int y = 0;
    int width = 0;
    int height = 0;
    icu::UnicodeString raw_input_buffer;
    std::string input_buffer;
    std::vector<std::string> input_words;
    tb_event ev;
    bool is_alive = true;
    bool is_suggestion_mode = true;
    bool allow_possible_offensive = true;

    tb_init();
    width = tb_width();
    height = tb_height();
    while (is_alive) {
        y = 0;
        input_buffer.clear();
        raw_input_buffer.toUTF8String(input_buffer);
        fl::str::split(input_buffer, ' ', input_words);
        prev_words.clear();
        for (std::size_t i = 0; i + 1 < input_words.size(); i++) {
            prev_words.push_back(input_words[i]);
        }

        tb_printf(0, y++, 0, 0, "FlorisNLP Core Debug Frontend");
        tb_printf(0, y++, 0, 0, "CTRL+C to quit | CTRL+D to toggle spell check/suggestion");
        tb_printf(0, y++, 0, 0, "---");
        tb_set_cursor(7 + raw_input_buffer.countChar32(), y);
        tb_printf(0, y++, 0, 0, "Input: %s", input_buffer.c_str());
        tb_printf(0, y++, 0, 0, "Length: %d", input_buffer.length());
        tb_printf(0, y++, 0, 0, "");
        if (allow_possible_offensive) {
            flags = 8 | fl::nlp::SuggestionRequestFlags::F_ALLOW_POSSIBLY_OFFENSIVE;
        } else {
            flags = 8;
        }
        if (is_suggestion_mode) {
            auto start = std::chrono::high_resolution_clock::now();
            nlp_session.suggest(input_words[input_words.size() - 1], prev_words, flags, suggestion_results);
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            tb_printf(0, y++, 0, 0, "Suggested words (%d, %dms):", suggestion_results.size(), duration.count());
            for (auto& result : suggestion_results) {
                tb_printf(0, y++, 0, 0, " %s | e=%d | c=%.4f", result->text.c_str(), result->edit_distance,
                          result->confidence);
            }
        } else {
            tb_printf(0, y++, 0, 0, "Spelling results:");
            for (auto& input_word : input_words) {
                auto result = nlp_session.spell(input_word, prev_words, prev_words, flags);
                std::stringstream ss;
                ss << "  " << input_word << " " << attrStatusSymbol(result.suggestion_attributes) << "  ->  ";
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
                tb_printf(0, y++, 0, 0, output.c_str());
            }
        }
        tb_printf(0, y++, 0, 0, "");
        tb_present();

        tb_poll_event(&ev);
        if (ev.type == TB_EVENT_RESIZE) {
            width = ev.w;
            height = ev.h;
        } else if (ev.type == TB_EVENT_KEY) {
            if (ev.key == TB_KEY_BACKSPACE || ev.key == TB_KEY_BACKSPACE2) {
                if (!raw_input_buffer.isEmpty()) {
                    raw_input_buffer.remove(raw_input_buffer.countChar32() - 1, 1);
                }
            } else if (ev.key == TB_KEY_CTRL_C) {
                is_alive = false;
            } else if (ev.key == TB_KEY_CTRL_D) {
                is_suggestion_mode = !is_suggestion_mode;
            } else if (ev.key == TB_KEY_CTRL_A) {
                allow_possible_offensive = !allow_possible_offensive;
            } else if (ev.ch != 0x0) {
                raw_input_buffer.append(static_cast<UChar32>(ev.ch));
            }
        }

        tb_clear();
    }
    tb_shutdown();

    return 0;
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
