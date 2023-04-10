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

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <map>
#include <regex>
#include <string>
#include <vector>

export module fl.nlp.core.common:syllable_matcher;

import fl.utils;

namespace fl::nlp {

struct SyllableCharacterSet {
    std::string name;
    std::string shorthand;
    std::vector<std::string> values;
};

const auto SYLLABLE_TOKEN_REGEX = std::regex(R"(\\\\[\\\\<-]|<[A-Za-z0-9]+>|<[\\\\][0-9]{1,2}>|[-]+|[^\\\\<]+)");
const char SYLLABLE_START_OF_WORD = '#';//2; // ASCII value for start of text
const char SYLLABLE_END_OF_WORD = '#';//3;   // ASCII value for end of text
const std::vector<SyllableCharacterSet> SYLLABLE_DEFAULT_CHARACTER_SETS = {
    {"start-of-word", "SOW", {std::string(1, SYLLABLE_START_OF_WORD)}},
    {"end-of-word", "EOW", {std::string(1, SYLLABLE_END_OF_WORD)}}};

struct SyllableDivisionRuleTokenValue {
    std::string text;
    std::vector<std::size_t> division_indices;
};

struct SyllableDivisionRuleToken {
    std::vector<SyllableDivisionRuleTokenValue> values;

    SyllableDivisionRuleToken(const std::string& token_str, const std::vector<SyllableCharacterSet>& character_sets) {
        std::vector<std::string> matches;
        std::size_t expected_match_begin = 0;
        std::sregex_iterator it(token_str.begin(), token_str.end(), SYLLABLE_TOKEN_REGEX);
        std::sregex_iterator end;
        while (it != end) {
            std::string match = (*it).str();
            std::size_t match_begin = token_str.find(match, expected_match_begin);
            if (match_begin != expected_match_begin) {
                throw std::runtime_error(
                    fmt::format("Unable to correctly parse token string (error at index {})", expected_match_begin)
                );
            }
            matches.push_back(match);
            expected_match_begin += match.size();
            it++;
        }
        if (expected_match_begin != token_str.size()) {
            throw std::runtime_error(
                fmt::format("Unable to correctly parse token string (error at index {})", expected_match_begin)
            );
        }

        std::vector<const std::vector<std::string>*> character_set_values;
        std::map<std::string, std::size_t> character_set_shorthands;
        std::size_t shorthand_index = 0;
        for (auto& match : matches) {
            if (match.starts_with("<") && !match.starts_with("<\\")) {
                auto shorthand = match.substr(1, match.size() - 2);
                auto set_it = std::find_if(character_sets.begin(), character_sets.end(), [&](auto& set) {
                    return set.shorthand == shorthand;
                });
                if (set_it == character_sets.end()) {
                    throw std::runtime_error(
                        fmt::format("Failed to parse token str: No such character set with shorthand '{}'", shorthand)
                    );
                }
                character_set_values.push_back(&(set_it->values));
                character_set_shorthands[shorthand] = shorthand_index++;
            }
        }

        fl::utils::forEachCombination<std::string>(character_set_values, [&](const std::vector<std::string>& set) {
            SyllableDivisionRuleTokenValue value;
            for (const auto& match : matches) {
                if (match.starts_with("-")) {
                    value.division_indices.push_back(value.text.size());
                } else if (match.starts_with("\\")) {
                    value.text.append(match.substr(1));
                } else if (match.starts_with("<")) {
                    if (match.starts_with("<\\")) {
                        // backreference
                        auto index = std::stoi(match.substr(2, match.size() - 3));
                        auto& text = set[index - 1];
                        value.text.append(text);
                    } else {
                        // generator expression
                        auto shorthand = match.substr(1, match.size() - 2);
                        auto& text = set[character_set_shorthands.at(shorthand)];
                        value.text.append(text);
                    }
                } else {
                    value.text.append(match);
                }
            }
            values.push_back(value);
        });
    }
};

struct SyllableDivisionRule {
    std::string name;
    std::vector<std::string> raw_tokens;
    std::vector<SyllableDivisionRuleToken> parsed_tokens;

    void parseRawTokens(const std::vector<SyllableCharacterSet>& character_sets) {
        parsed_tokens.clear();
        for (auto& raw_token : raw_tokens) {
            parsed_tokens.emplace_back(raw_token, character_sets);
        }
    }
};

struct SyllableMatcherConfig {
    std::vector<SyllableCharacterSet> character_sets;
    std::vector<SyllableDivisionRule> division_rules;
};

void to_json(nlohmann::json& j, const SyllableCharacterSet& s) {
    j = nlohmann::json {{"name", s.name}, {"shorthand", s.shorthand}, {"values", s.values}};
}

void from_json(const nlohmann::json& j, SyllableCharacterSet& s) {
    j.at("name").get_to(s.name);
    j.at("shorthand").get_to(s.shorthand);
    j.at("values").get_to(s.values);
}

void to_json(nlohmann::json& j, const SyllableDivisionRule& r) {
    j = nlohmann::json {{"name", r.name}, {"tokens", r.raw_tokens}};
}

void from_json(const nlohmann::json& j, SyllableDivisionRule& r) {
    j.at("name").get_to(r.name);
    j.at("tokens").get_to(r.raw_tokens);
}

void to_json(nlohmann::json& j, const SyllableMatcherConfig& c) {
    j = nlohmann::json {{"characterSets", c.character_sets}, {"divisionRules", c.division_rules}};
}

void from_json(const nlohmann::json& j, SyllableMatcherConfig& c) {
    j.at("characterSets").get_to(c.character_sets);
    j.at("divisionRules").get_to(c.division_rules);
    c.character_sets.insert(
        c.character_sets.end(), SYLLABLE_DEFAULT_CHARACTER_SETS.begin(), SYLLABLE_DEFAULT_CHARACTER_SETS.end()
    );

    for (auto& division_rule : c.division_rules) {
        division_rule.parseRawTokens(c.character_sets);
    }
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
        nlohmann::json j;
        config_file >> j;
        j.get_to(config);
    }

    void divideWordIntoSyllables(const std::string& original_word, std::vector<std::string>& syllables) const noexcept {
        syllables.clear();
        std::string word = fmt::format("{}{}{}", SYLLABLE_START_OF_WORD, original_word, SYLLABLE_END_OF_WORD);
        std::vector<SyllableDivisionState> state(word.size() + 1, SyllableDivisionState::UNDECIDED);

        for (const auto& rule : config.division_rules) {
            for (const auto& token : rule.parsed_tokens) {
                for (const auto& value : token.values) {
                    auto& text = value.text;
                    auto& div = value.division_indices;
                    std::size_t global_index = 0;
                    while ((global_index = word.find(text, global_index)) != std::string::npos) {
                        bool is_allowed = true;
                        for (std::size_t i = 0; i <= text.length(); i++) {
                            if (state[global_index + i] == SyllableDivisionState::UNDECIDED) {
                                continue;
                            } else if (state[global_index + i] == SyllableDivisionState::SPLIT) {
                                if (i > 0 && i < text.length() && std::find(div.begin(), div.end(), i) == div.end()) {
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
                            for (std::size_t i = 0; i <= text.length(); i++) {
                                if (std::find(div.begin(), div.end(), i) != div.end()) {
                                    state[global_index + i] = SyllableDivisionState::SPLIT;
                                } else if (i > 0 && i < text.length()) {
                                    state[global_index + i] = SyllableDivisionState::DONT_SPLIT;
                                }
                            }
                        }
                        global_index += text.length();
                    }
                }
            }
        }

        std::string temp;
        for (std::size_t i = 0; i < original_word.size(); i++) {
            if (i > 0 && state[i + 1] == SyllableDivisionState::SPLIT) {
                syllables.push_back(std::move(temp));
            }
            temp.push_back(original_word[i]);
        }
        syllables.push_back(std::move(temp));
    }
};

} // namespace fl::nlp
