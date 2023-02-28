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
#include <unicode/uchar.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <vector>

export module fl.nlp.tools.train:raw_text;

import fl.nlp.core.latin;
import fl.nlp.string;
import fl.nlp.tools.common;

namespace fl::nlp::tools {

export class TrainRawTextActionConfig : public ActionConfig {
  public:
    TrainRawTextActionConfig() : ActionConfig("train-raw-text") {};

    void initArgumentConfig(argparse::ArgumentParser& arg_parser) override {
        arg_parser.add_description("Train a user dictionary based on sentences in a raw text file");
        DictionaryArgsUtils::initArgumentConfig(arg_parser);
    }

    int runAction(argparse::ArgumentParser& arg_parser) override {
        auto [base_dict_path, user_dict_path] = DictionaryArgsUtils::readArgumentsAndCheckFiles(arg_parser);
        // TODO hook up code for training here
        return 0;
    }
};

} // namespace fl::nlp::tools
