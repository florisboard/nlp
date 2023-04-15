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

#include <nlohmann/json.hpp>
#include <unicode/locid.h>

#include <string>
#include <vector>

export module fl.nlp.core.latin:nlp_session_config;

import :prediction_weights;
import fl.icuext;
import fl.nlp.core.common;

namespace fl::nlp {

using json = nlohmann::json;

export struct LatinNlpSessionConfig {
    icu::Locale primary_locale;
    std::vector<icu::Locale> secondary_locales;
    std::vector<std::string> base_dictionary_paths;
    std::string user_dictionary_path;
    LatinPredictionWeights weights_;
    KeyProximityChecker key_proximity_checker;
};

export void to_json(json& j, const LatinNlpSessionConfig& config) {
    j = json {
        {"primaryLocale", config.primary_locale},
        {"secondaryLocales", config.secondary_locales},
        {"baseDictionaries", config.base_dictionary_paths},
        {"userDictionary", config.user_dictionary_path},
        {"predictionWeights", config.weights_},
        {"keyProximityChecker", config.key_proximity_checker}};
}

export void from_json(const json& j, LatinNlpSessionConfig& config) {
    j.at("primaryLocale").get_to(config.primary_locale);
    j.at("secondaryLocales").get_to(config.secondary_locales);
    j.at("baseDictionaries").get_to(config.base_dictionary_paths);
    j.at("userDictionary").get_to(config.user_dictionary_path);
    j.at("predictionWeights").get_to(config.weights_);
    j.at("keyProximityChecker").get_to(config.key_proximity_checker);
}

} // namespace fl::nlp
