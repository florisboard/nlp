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

#include <argparse/argparse.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

import fl.nlp.core.common;
import fl.nlp.tools.common;
import fl.nlp.tools.core_ui;
import fl.nlp.tools.prep;
import fl.nlp.tools.train;
// #include "fl_nlp_core_common.hpp"
// #include "fl_nlp_tools_common.hpp"
// #include "fl_nlp_tools_core_ui.hpp"
// #include "fl_nlp_tools_prep.hpp"
// #include "fl_nlp_tools_train.hpp"

void printVersion() noexcept {
    std::cout << "FlorisNLP Tools v" << PROGRAM_VERSION << "\n";
}

// This method's code is based on this code:
// https://github.com/p-ranav/argparse/blob/e51655673324264dec95dd3b5168baf8e54cde17/include/argparse/argparse.hpp#L1070
//
// It has been modified to suit this project's needs.
void initDefaultArgumentsConfig(argparse::ArgumentParser& arg_parser) {
    arg_parser.add_argument("-h", "--help")
        .action([&](const auto&) {
            printVersion();
            std::cout << "\n" << arg_parser.help().str();
            std::exit(0);
        })
        .default_value(false)
        .implicit_value(true)
        .help("Shows this help message and exits")
        .nargs(0);

    arg_parser.add_argument("-v", "--version")
        .action([&](const auto&) {
            printVersion();
            std::exit(0);
        })
        .default_value(false)
        .implicit_value(true)
        .help("Prints version information and exits")
        .nargs(0);
}

int main(int argc, char** argv) {
    /*fl::nlp::SyllableMatcher matcher("data/syllable-matcher-en-GB.json");
    std::string word = argv[1];
    std::vector<std::string> syllables;
    matcher.divideWordIntoSyllables(word, syllables);
    std::cout << word << " ";
    for (auto& syllable : syllables) {
        std::cout << syllable << "-";
    }
    std::cout << std::endl;
    return 0;*/

    std::vector<std::unique_ptr<fl::nlp::tools::ActionConfig>> actions;
    actions.emplace_back(std::make_unique<fl::nlp::tools::CoreUiActionConfig>());
    actions.emplace_back(std::make_unique<fl::nlp::tools::PrepWiktextractActionConfig>());
    actions.emplace_back(std::make_unique<fl::nlp::tools::TrainRawTextActionConfig>());
    actions.emplace_back(std::make_unique<fl::nlp::tools::TrainWordScoresActionConfig>());

    fl::nlp::tools::Program program(PROGRAM_NAME, PROGRAM_VERSION, argc, argv);
    for (auto& action : actions) {
        action->initArgumentConfig(action->arg_parser);
        initDefaultArgumentsConfig(action->arg_parser);
        program.arg_parser.add_subparser(action->arg_parser);
    }
    initDefaultArgumentsConfig(program.arg_parser);

    if (argc <= 1) {
        printVersion();
        std::cout << "\n" << program.arg_parser.help().str();
        return 0;
    }

    try {
        program.parse_args();
        for (auto& action : actions) {
            if (program.arg_parser.is_subcommand_used(action->name)) {
                return action->runAction(action->arg_parser);
            }
        }
    } catch (const std::exception& err) {
        std::cerr << "Fatal: " << err.what() << " Aborting." << std::endl;
        return 1;
    }

    std::cerr << "Fatal: How Did We Get Here? Aborting.\n";
    return 1;
}
