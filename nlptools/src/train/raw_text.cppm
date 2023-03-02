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
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <unicode/brkiter.h>
#include <unicode/uchar.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <vector>

export module fl.nlp.tools.train:raw_text;

import fl.icuext;
import fl.nlp.core.latin;
import fl.nlp.string;
import fl.nlp.tools.common;

const auto ARG_TXT_FILE = "--txt-file";

void trainDict(const fl::nlp::LatinDictionary& base_dict, fl::nlp::LatinDictionary& user_dict, std::istream& istream) {
    UErrorCode status = U_ZERO_ERROR;
    const std::string content(std::istreambuf_iterator<char>(istream), {});
    fl::icuext::Text content_text;
    fl::icuext::Text sentence_text;
    content_text.openUTF8(content, status);
    auto sentence_iterator = icu::BreakIterator::createSentenceInstance(base_dict.meta.locales[0], status);
    auto word_iterator = icu::BreakIterator::createWordInstance(base_dict.meta.locales[0], status);
    sentence_iterator->setText(content_text, status);
    std::string sentence;
    std::string word;
    fl::str::UniString uni_word;
    std::vector<fl::str::UniString> uni_words;

    if (U_FAILURE(status)) {
        throw std::runtime_error("Error in initializing trainDict()");
    }

    std::cout << "\n";
    fl::icuext::brkiter::forEach(sentence_iterator, [&](int32_t start, int32_t end) {
        sentence.assign(content, start, end - start);
        fl::str::trim(sentence);
        if (!sentence.empty()) {
            sentence_text.openUTF8(sentence, status);
            word_iterator->setText(sentence_text, status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Error in initializing word iterator data");
            }
            uni_words.clear();
            fl::icuext::brkiter::forEach(word_iterator, [&](int32_t start, int32_t end) {
                word.assign(sentence, start, end - start);
                auto rule_status = word_iterator->getRuleStatus();
                if (rule_status != UWordBreak::UBRK_WORD_NONE) {
                    fl::str::toUniString(word, uni_word);
                    uni_words.emplace_back(uni_word);
                }
            });
            fmt::print("{}\n", uni_words);
            // TODO: pass uni_words to NLP Session training here
        }
    });

    delete word_iterator;
    delete sentence_iterator;
}

namespace fl::nlp::tools {

export class TrainRawTextActionConfig : public ActionConfig {
  public:
    TrainRawTextActionConfig() : ActionConfig("train-raw-text") {};

    void initArgumentConfig(argparse::ArgumentParser& arg_parser) override {
        arg_parser.add_description("Train a user dictionary based on sentences in a raw text file");
        DictionaryArgsUtils::initArgumentConfig(arg_parser);
        arg_parser.add_argument(ARG_TXT_FILE).required().help("the text file to read from");
    }

    int runAction(argparse::ArgumentParser& arg_parser) override {
        auto [base_dict_path, user_dict_path] = DictionaryArgsUtils::readArgumentsAndCheckFiles(arg_parser);
        std::string txt_file_path = arg_parser.get(ARG_TXT_FILE);
        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = std::chrono::high_resolution_clock::now();

        fl::nlp::LatinDictionary base_dict;
        fl::nlp::LatinDictionary user_dict;

        std::cout << "Loading base dictionary from disk... " << std::flush;
        start_time = std::chrono::high_resolution_clock::now();
        base_dict.loadFromDisk(base_dict_path);
        end_time = std::chrono::high_resolution_clock::now();
        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
                  << "ms." << std::endl;

        std::cout << "Loading user dictionary from disk... " << std::flush;
        start_time = std::chrono::high_resolution_clock::now();
        user_dict.loadFromDisk(user_dict_path);
        end_time = std::chrono::high_resolution_clock::now();
        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
                  << "ms." << std::endl;

        std::cout << "Training user dictionary with provided txt file... " << std::flush;
        start_time = std::chrono::high_resolution_clock::now();
        std::ifstream txt_file(txt_file_path);
        trainDict(base_dict, user_dict, txt_file);
        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
                  << "ms." << std::endl;

        std::cout << "Persisting user dictionary to disk... " << std::flush;
        start_time = std::chrono::high_resolution_clock::now();
        user_dict.persistToDisk();
        end_time = std::chrono::high_resolution_clock::now();
        std::cout << "Done in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
                  << "ms." << std::endl;

        return 0;
    }
};

} // namespace fl::nlp::tools
