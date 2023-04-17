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

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

export module fl.nlp.tools.train:word_scores;

import fl.nlp.core.latin;
import fl.nlp.tools.common;
import fl.icuext;
import fl.string;

const auto ARG_DICTIONARY = "--dictionary";
const auto ARG_WORDLIST = "--wordlist";
const auto ARG_SCORE_THRESHOLD = "--score-threshold";

void trainDict(fl::nlp::LatinDictionary& dict, std::istream& wordlist_file) {
    std::string line;
    std::vector<std::string> line_components;
    fl::str::UniString word;
    while (std::getline(wordlist_file, line)) {
        fl::str::split(line, '\t', line_components);
        if (line_components.size() != 2) continue;
        auto& raw_word = line_components[0];
        fl::str::toUniString(raw_word, word);
        fl::nlp::ScoreT word_count = std::stoll(line_components[1]);
        ;
        auto* node = dict.data_->findOrNull(word);
        if (node == nullptr) continue;
        auto* value = node->valueOrNull(dict.dict_id_);
        if (value == nullptr) continue;
        auto* properties = value->wordPropertiesOrNull();
        if (properties == nullptr) continue;
        properties->absolute_score += word_count;
    }
}

void removeWordsBelowThreshold(fl::nlp::LatinDictionary& dict, fl::nlp::ScoreT score_threshold) {
    fl::nlp::algorithms::forEachWord(
        dict.data_.get(), dict.dict_id_,
        [&](auto word, fl::nlp::LatinTrieNode* node, auto* properties) {
            if (properties->absolute_score < score_threshold) {
                node->value(dict.dict_id_)->removeWordProperties();
            }
        }
    );
}

namespace fl::nlp::tools {

export class TrainWordScoresActionConfig : public ActionConfig {
  public:
    TrainWordScoresActionConfig() : ActionConfig("train-word-scores") {};

    void initArgumentConfig(argparse::ArgumentParser& arg_parser) override {
        arg_parser.add_description(
            "Train dictionary word scores based on sentences in a raw text file. Note that the dictionary must be"
            "pre-populated and only words that already exist in the dictionary are counted!"
        );
        arg_parser.add_argument(ARG_DICTIONARY).required().help("the dictionary file to edit");
        arg_parser.add_argument(ARG_WORDLIST).required().help("the text file to read from");
        arg_parser.add_argument(ARG_SCORE_THRESHOLD)
            .default_value(std::string("1"))
            .help("the score threshold for words to be kept in the dictionary");
    }

    int runAction(argparse::ArgumentParser& arg_parser) override {
        std::string dictionary_path = arg_parser.get(ARG_DICTIONARY);
        std::string wordlist_path = arg_parser.get(ARG_WORDLIST);
        fl::nlp::ScoreT score_threshold = std::stoll(arg_parser.get(ARG_SCORE_THRESHOLD));

        Stopwatch stopwatch;
        LatinDictionary dict(0);

        std::cout << "Loading dictionary... " << std::flush;
        stopwatch.start();
        dict.loadFromDisk(dictionary_path);
        stopwatch.stop();
        std::cout << "Done in " << stopwatch.elapsed() << "ms." << std::endl;

        std::cout << "Training dictionary with provided txt file... " << std::flush;
        stopwatch.start();
        std::ifstream wordlist_file(wordlist_path);
        trainDict(dict, wordlist_file);
        wordlist_file.close();
        stopwatch.stop();
        std::cout << "Done in " << stopwatch.elapsed() << "ms." << std::endl;

        std::cout << "Removing words with a score below given threshold (" << score_threshold << ")... " << std::flush;
        stopwatch.start();
        removeWordsBelowThreshold(dict, score_threshold);
        stopwatch.stop();
        std::cout << "Done in " << stopwatch.elapsed() << "ms." << std::endl;

        std::cout << "Persisting user dictionary to disk... " << std::flush;
        stopwatch.start();
        dict.persistToDisk();
        stopwatch.stop();
        std::cout << "Done in " << stopwatch.elapsed() << "ms." << std::endl;

        return 0;
    }
};

} // namespace fl::nlp::tools
