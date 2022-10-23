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

#include "core/common.hpp"
#include "core/dictionary.hpp"
#include "core/dictionary_session.hpp"
#include "core/string.hpp"
#include "core/udata.hpp"

#include <unicode/unistr.h>

#include <termbox.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <vector>

// TODO: get this path dynamically and remove the hardcoded path
const std::string ICU_DATA_FILE_PATH = "build/debug/icu4c/host/share/icu_floris/71.1/icudt71l.dat";
const std::filesystem::path DICT_PATH = "data/wikt_en.fldic";

const char* attr_status_symbol(int32_t suggestion_attribute) noexcept {
    if (suggestion_attribute == fl::nlp::RESULT_ATTR_IN_THE_DICTIONARY) {
        return "✅";
    } else if (suggestion_attribute == fl::nlp::RESULT_ATTR_LOOKS_LIKE_TYPO) {
        return "❌";
    } else {
        return "❔";
    }
}

int main(int argc, char** argv) {
    if (fl::icu::load_and_set_common_data(ICU_DATA_FILE_PATH) != UErrorCode::U_ZERO_ERROR) {
        std::cout << "Failed to load ICU data file!" << std::endl;
        return 1;
    }

    fl::nlp::suggestion_request_flags flags = 8;
    std::vector<std::unique_ptr<fl::nlp::suggestion_candidate>> suggestion_results;
    std::vector<fl::u8str> prev_words;
    fl::nlp::dictionary_session dict_session;
    dict_session.key_proximity_mapping.load_from_file("data/qwerty_proximity_map.json");
    dict_session.load_base_dictionary(DICT_PATH);

    int y = 0;
    int width = 0;
    int height = 0;
    icu::UnicodeString raw_input_buffer;
    fl::u8str input_buffer;
    std::vector<fl::u8str> input_words;
    tb_event ev;
    bool is_alive = true;
    bool is_suggestion_mode = true;

    tb_init();
    width = tb_width();
    height = tb_height();
    while (is_alive) {
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
        if (is_suggestion_mode) {
            auto start = std::chrono::high_resolution_clock::now();
            dict_session.suggest(input_words[input_words.size() - 1], prev_words, flags, suggestion_results);
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            tb_printf(0, y++, 0, 0, "Suggested words (%d, %dms):", suggestion_results.size(), duration.count());
            for (auto& result : suggestion_results) {
                tb_printf(
                    0, y++, 0, 0, " %s | e=%d | c=%.4f", result->text.c_str(), result->edit_distance, result->confidence
                );
            }
        } else {
            tb_printf(0, y++, 0, 0, "Spelling results:");
            for (auto& input_word : input_words) {
                auto result = dict_session.spell(input_word, prev_words, prev_words, flags);
                std::stringstream ss;
                ss << "  " << input_word << " " << attr_status_symbol(result.suggestion_attributes) << "  ->  ";
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
            } else if (ev.ch != 0x0) {
                raw_input_buffer.append(static_cast<UChar32>(ev.ch));
            }
        }

        tb_clear();
        y = 0;
    }
    tb_shutdown();

    return 0;
}
