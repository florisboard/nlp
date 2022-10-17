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

#include <filesystem>
#include <iostream>

// TODO: get this path dynamically and remove the hardcoded path
const std::string ICU_DATA_FILE_PATH = "build/debug/icu4c/host/share/icu_floris/71.1/icudt71l.dat";
const std::filesystem::path DICT_PATH = "data/test_out.fldic";

int main(int argc, char** argv) {
    if (fl::icu::load_and_set_common_data(ICU_DATA_FILE_PATH) != UErrorCode::U_ZERO_ERROR) {
        std::cout << "Failed to load ICU data file!" << std::endl;
        return 1;
    }

    fl::nlp::suggestion_request_flags flags = 8;
    std::vector<std::unique_ptr<fl::nlp::suggestion_candidate>> results;
    const std::vector<fl::u8str> prev_words;
    fl::nlp::dictionary_session dict_session;
    dict_session.load_base_dictionary(DICT_PATH);

    int y = 0;
    int width = 0;
    int height = 0;
    icu::UnicodeString raw_input_buffer;
    fl::u8str input_buffer;
    tb_event ev;
    bool is_alive = true;

    tb_init();
    width = tb_width();
    height = tb_height();
    while (is_alive) {
        input_buffer.clear();
        raw_input_buffer.toUTF8String(input_buffer);
        dict_session.suggest(input_buffer, prev_words, flags, results);

        tb_printf(0, y++, 0, 0, "FlorisNLP Core Debug Frontend");
        tb_printf(0, y++, 0, 0, "CTRL+C to quit");
        tb_printf(0, y++, 0, 0, "---");
        tb_set_cursor(7 + raw_input_buffer.countChar32(), y);
        tb_printf(0, y++, 0, 0, "Input: %s", input_buffer.c_str());
        tb_printf(0, y++, 0, 0, "Length: %d", input_buffer.length());
        tb_printf(0, y++, 0, 0, "");
        tb_printf(0, y++, 0, 0, "Suggested words:");
        for (auto& result : results) {
            tb_printf(0, y++, 0, 0, " %s | %.4f", result->text.c_str(), result->confidence);
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
