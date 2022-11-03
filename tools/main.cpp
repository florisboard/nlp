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

#include "core/string.hpp"
#include "tools/core_ui.hpp"
#include "tools/prep_wiktextract.hpp"

#include <iostream>
#include <vector>

static const fl::u8str VERSION = "0.1.0";

static const fl::u8str ACTION_CORE_UI = "core-ui";
static const fl::u8str ACTION_PREP_WIKTEXTRACT = "prep-wiktextract";
static const fl::u8str FLAG_INDICATOR = "-";
static const fl::u8str FLAG_HELP = "--help";
static const fl::u8str FLAG_VERSION = "--version";

void print_version() noexcept {
    std::cout << "FlorisNLP Tools v" << VERSION << "\n";
}

void print_version_with_additional_newline() noexcept {
    print_version();
    std::cout << "\n";
}

void print_usage(char* arg0) noexcept {
    print_version();
    std::cout << "\nUsage: " << arg0 << " <action> [<flags>]\n\n"
              << "Available actions:\n"
              << "    " << ACTION_CORE_UI << "\n"
              << "    " << ACTION_PREP_WIKTEXTRACT << "\n"
              << "    " << FLAG_HELP << "\n"
              << "    " << FLAG_VERSION << "\n";
}

auto collect_flags(int argc, char** argv) noexcept {
    std::vector<fl::u8str> flags;
    for (int i = 2; i < argc; i++) {
        flags.push_back(fl::u8str(argv[i]));
    }
    return flags;
}

bool has_flag(const fl::u8str& flag_to_search, const std::vector<fl::u8str>& flags) noexcept {
    for (auto& flag : flags) {
        if (flag == flag_to_search) return true;
    }
    return false;
}

int main(int argc, char** argv) {
    if (argc < 1) return 1;
    if (argc == 1) {
        print_usage(argv[0]);
        return 1;
    }

    auto action = fl::u8str(argv[1]);
    auto flags = collect_flags(argc, argv);
    if (action == FLAG_HELP) {
        print_usage(argv[0]);
    } else if (action == FLAG_VERSION) {
        print_version();
    } else if (action == ACTION_CORE_UI) {
        if (has_flag(FLAG_HELP, flags)) {
            print_version_with_additional_newline();
            return fl::nlp::tools::print_core_ui_usage(argv[0]);
        } else {
            return fl::nlp::tools::handle_core_ui_action(flags);
        }
    } else if (action == ACTION_PREP_WIKTEXTRACT) {
        if (has_flag(FLAG_HELP, flags)) {
            print_version_with_additional_newline();
            return fl::nlp::tools::print_prep_wiktextract_usage(argv[0]);
        } else {
            return fl::nlp::tools::handle_prep_wiktextract_action(flags);
        }
    } else {
        if (action.starts_with(FLAG_INDICATOR)) {
            std::cerr << "Fatal: Unknown flag'" << action << "'. See '" << argv[0] << " --help'.\n";
        } else {
            std::cerr << "Fatal: Unknown action '" << action << "'. See '" << argv[0] << " --help'.\n";
        }
        return 1;
    }

    return 0;
}
