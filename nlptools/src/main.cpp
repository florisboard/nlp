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

import fl.nlp.tools.common;
import fl.nlp.tools.core_ui;
import fl.nlp.tools.prep_wiktextract;

void printVersion() noexcept {
    std::cout << "FlorisNLP Tools v" << PROGRAM_VERSION << "\n";
}

// This method's code is based on this code:
// https://github.com/p-ranav/argparse/blob/e51655673324264dec95dd3b5168baf8e54cde17/include/argparse/argparse.hpp#L1070
//
// It has been modified to suit this project's needs.
void initDefaultArguments(argparse::ArgumentParser& arg_parser) {
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
    std::vector<std::unique_ptr<fl::nlp::tools::ActionConfig>> actions;
    actions.emplace_back(std::make_unique<fl::nlp::tools::CoreUiActionConfig>());
    actions.emplace_back(std::make_unique<fl::nlp::tools::PrepWiktextractActionConfig>());

    fl::nlp::tools::Program program(PROGRAM_NAME, PROGRAM_VERSION, argc, argv);
    initDefaultArguments(program.arg_parser);
    for (auto& action : actions) {
        initDefaultArguments(action->arg_parser);
        action->initArgumentConfig(action->arg_parser);
        program.arg_parser.add_subparser(action->arg_parser);
    }

    if (argc <= 1) {
        printVersion();
        std::cout << "\n" << program.arg_parser.help().str();
        return 0;
    }

    try {
        program.parse_args();
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return 1;
    }

    for (auto& action : actions) {
        if (program.arg_parser.is_subcommand_used(action->name)) {
            return action->runAction(action->arg_parser);
        }
    }

    std::cerr << "Fatal: How Did We Get Here? Aborting.\n";
    return 1;
}
