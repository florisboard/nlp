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

#include "icuext/data.hpp"
#include "icuext/string.hpp"
#include "nlp/spelling_result.hpp"

#include <tsl/htrie_map.h>

#include <termbox.h>

#include <fstream>
#include <iostream>

// TODO: get this path dynamically and remove the hardcoded path
const std::string ICU_DATA_FILE_PATH = "build/debug/icu4c/host/share/icu_floris/71.1/icudt71l.dat";

int main(int argc, char** argv) {
    if (icuext::load_and_set_common_data(ICU_DATA_FILE_PATH) != UErrorCode::U_ZERO_ERROR) {
        std::cout << "Failed to load ICU data file!" << std::endl;
        return 1;
    }

    auto x = floris::nlp::SpellingResult::valid_word();
    int y = 0;
    int width = 0;
    int height = 0;
    icuext::u32str input_buffer;
    icuext::u8str output_buffer;
    tb_event ev;
    bool is_alive = true;

    tb_init();
    width = tb_width();
    height = tb_height();
    while (is_alive) {
        tb_printf(0, y++, 0, 0, "FlorisNLP Core Debug Frontend");
        tb_printf(0, y++, 0, 0, "---");
        tb_printf(0, y++, 0, 0, "Suggested words: <none>");
        tb_printf(0, y++, 0, 0, "");
        icuext::str::to_u8str(input_buffer, output_buffer);
        tb_set_cursor(7 + input_buffer.size(), y);
        tb_printf(0, y++, 0, 0, "Input: %s", output_buffer.c_str());
        tb_printf(0, y++, 0, 0, "");
        tb_printf(0, y++, 0, 0, "CTRL+C to quit");
        tb_present();

        tb_poll_event(&ev);
        if (ev.type == TB_EVENT_RESIZE) {
            width = ev.w;
            height = ev.h;
        } else if (ev.type == TB_EVENT_KEY) {
            if (ev.key == TB_KEY_BACKSPACE || ev.key == TB_KEY_BACKSPACE2) {
                if (!input_buffer.empty()) {
                    input_buffer.pop_back();
                }
            } else if (ev.key == TB_KEY_CTRL_C) {
                is_alive = false;
            } else if (ev.ch != 0x0) {
                char32_t ch = ev.ch;
                input_buffer.push_back(ch);
            }
        }

        tb_clear();
        y = 0;
    }
    tb_shutdown();

    return 0;
}
