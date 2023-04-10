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

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

export module fl.nlp.tools.common:dictionary_args_utils;

import fl.string;

namespace fl::nlp::tools {

export const auto ARG_SESSION_CONFIG_PATH = "--session-config";

export class DictionaryArgsUtils {
  public:
    static void initArgumentConfig(argparse::ArgumentParser& arg_parser) {
        arg_parser.add_argument(ARG_SESSION_CONFIG_PATH)
            .required()
            .metavar("PATH")
            .help("Path of the NLP session config file to load");
    }

    static std::string readArgumentsAndCheckFiles(argparse::ArgumentParser& arg_parser) {
        std::string session_config_path = arg_parser.get(ARG_SESSION_CONFIG_PATH);
        fl::str::trim(session_config_path);
        if (session_config_path.empty()) {
            throw std::runtime_error("Specified session config path is empty!");
        }
        if (!std::filesystem::exists(session_config_path)) {
            throw std::runtime_error("Specified session config path does not exist!");
        }
        return std::move(session_config_path);
    }
};

} // namespace fl::nlp::tools
