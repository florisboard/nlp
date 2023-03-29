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

#include <fstream>
#include <string>
#include <vector>

export module fl.nlp.core.common:syllable_matcher;

namespace fl::nlp {

using json = nlohmann::json;

struct SyllableDivisionRuleToken {
    std::string value;
    std::vector<std::size_t> division_indices;
};

struct SyllableDivisionRule {
    std::string name;
    std::vector<SyllableDivisionRuleToken> tokens;
};

struct SyllableMatcherConfig {
    std::vector<SyllableDivisionRule> division_rules;
};

void to_json(json& j, const SyllableDivisionRuleToken& token) {
    std::string token_str = token.value;
    for (auto it = token.division_indices.rbegin(); it != token.division_indices.rend(); it++) {
        token_str.insert(*it, 1, '-');
    }
    j = token_str;
}

void from_json(const json& j, SyllableDivisionRuleToken& token) {
    std::string token_str;
    j.get_to(token_str);
    std::size_t index = 0;
    for (const auto& ch : token_str) {
        if (ch == '-') {
            token.division_indices.push_back(index);
        } else {
            token.value.push_back(ch);
            index++;
        }
    }
}

void to_json(json& j, const SyllableDivisionRule& r) {
    j = json {{"name", r.name}, {"tokens", r.tokens}};
}

void from_json(const json& j, SyllableDivisionRule& r) {
    j.at("name").get_to(r.name);
    j.at("tokens").get_to(r.tokens);
}

void to_json(json& j, const SyllableMatcherConfig& c) {
    j = json {{"divisionRules", c.division_rules}};
}

void from_json(const json& j, SyllableMatcherConfig& c) {
    j.at("divisionRules").get_to(c.division_rules);
}

enum class SyllableDivisionState {
    UNDECIDED,
    SPLIT,
    DONT_SPLIT,
};

export class SyllableMatcher {
  public:
    bool enabled = false;
    SyllableMatcherConfig config;

  public:
    SyllableMatcher(const std::string& config_path) {
        loadConfigFromFile(config_path);
    }

    void loadConfigFromFile(const std::string& config_path) {
        std::ifstream config_file(config_path);
        json j;
        config_file >> j;
        j.get_to(config);
    }

    void divideWordIntoSyllables(const std::string& word, std::vector<std::string>& syllables) const noexcept {
        syllables.clear();
        std::vector<SyllableDivisionState> state(word.size() + 1, SyllableDivisionState::UNDECIDED);

        for (const auto& rule : config.division_rules) {
            for (const auto& token : rule.tokens) {
                auto& val = token.value;
                auto& div = token.division_indices;
                std::size_t global_index = 0;
                while ((global_index = word.find(val, global_index)) != std::string::npos) {
                    bool is_allowed = true;
                    for (std::size_t i = 0; i <= val.length(); i++) {
                        if (state[global_index + i] == SyllableDivisionState::UNDECIDED) {
                            continue;
                        } else if (state[global_index + i] == SyllableDivisionState::SPLIT) {
                            if (i > 0 && i < val.length() && std::find(div.begin(), div.end(), i) == div.end()) {
                                is_allowed = false;
                                break;
                            }
                        } else if (state[global_index + i] == SyllableDivisionState::DONT_SPLIT) {
                            if (std::find(div.begin(), div.end(), i) != div.end()) {
                                is_allowed = false;
                                break;
                            }
                        }
                    }
                    if (is_allowed) {
                        for (std::size_t i = 0; i <= val.length(); i++) {
                            if (std::find(div.begin(), div.end(), i) != div.end()) {
                                state[global_index + i] = SyllableDivisionState::SPLIT;
                            } else if (i > 0 && i < val.length()) {
                                state[global_index + i] = SyllableDivisionState::DONT_SPLIT;
                            }
                        }
                    }
                    global_index += token.value.length();
                }
            }
        }

        std::string temp;
        for (std::size_t i = 0; i < word.size(); i++) {
            if (i > 0 && state[i] == SyllableDivisionState::SPLIT) {
                syllables.push_back(std::move(temp));
            }
            temp.push_back(word[i]);
        }
        syllables.push_back(std::move(temp));
    }
};

} // namespace fl::nlp
