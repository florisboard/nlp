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

export module fl.nlp.tools.common.program;

namespace fl::nlp::tools {

export class Program {
  public:
    argparse::ArgumentParser arg_parser;
    int argc;
    char** argv;

    Program() = delete;
    Program(const std::string& name, const std::string& version, int argc, char** argv)
        : argc(argc), argv(argv), arg_parser(name, version, argparse::default_arguments::none) {}
    ~Program() = default;

    void parse_args() {
        arg_parser.parse_args(argc, argv);
    }

    void parse_known_args() {
        arg_parser.parse_known_args(argc, argv);
    }
};

} // namespace fl::nlp::tools
