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

import fl.nlp.string;

namespace fl::nlp::tools {

export const auto ARG_BASE_DICT_PATH = "--base-dict";
export const auto ARG_USER_DICT_PATH = "--user-dict";

export class DictionaryArgsUtils {
  public:
    static void initArgumentConfig(argparse::ArgumentParser& arg_parser, bool user_dict_required = true) {
        arg_parser.add_argument(ARG_BASE_DICT_PATH)
            .required()
            .metavar("PATH")
            .help("Path of the base dictionary to load in");
        auto& arg =
            arg_parser.add_argument(ARG_USER_DICT_PATH).metavar("PATH").help("Path of the user dictionary to load in");
        if (user_dict_required) {
            arg.required();
        } else {
            arg.default_value(std::string(""));
        }
    }

    static std::pair<std::string, std::string> readArgumentsAndCheckFiles(argparse::ArgumentParser& arg_parser,
                                                                          bool user_dict_required = true) {
        std::string base_dict_path = arg_parser.get(ARG_BASE_DICT_PATH);
        std::string user_dict_path = arg_parser.get(ARG_USER_DICT_PATH);
        fl::str::trim(base_dict_path);
        fl::str::trim(user_dict_path);
        if (base_dict_path.empty()) {
            throw std::runtime_error("Specified base dictionary path is empty!");
        }
        if (!std::filesystem::exists(base_dict_path)) {
            throw std::runtime_error("Specified base dictionary path does not exist!");
        }
        if (user_dict_required && user_dict_path.empty()) {
            throw std::runtime_error("Specified user dictionary path is empty!");
        }
        if (user_dict_required && !std::filesystem::exists(user_dict_path)) {
            throw std::runtime_error("Specified user dictionary path does not exist!");
        }
        return std::make_pair(std::move(base_dict_path), std::move(user_dict_path));
    }
};

} // namespace fl::nlp::tools
