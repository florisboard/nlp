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

export module fl.nlp.tools.common.action_config;

namespace fl::nlp::tools {

export class ActionConfig {
  public:
    std::string name;
    argparse::ArgumentParser arg_parser;

    ActionConfig() = delete;
    ActionConfig(const std::string& name) : name(name), arg_parser(name, "", argparse::default_arguments::none) {};
    virtual ~ActionConfig() = default;

    virtual void initArgumentConfig(argparse::ArgumentParser& arg_parser) = 0;
    virtual int runAction(argparse::ArgumentParser& arg_parser) = 0;
};

} // namespace fl::nlp::tools
