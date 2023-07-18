#!/usr/bin/env python3

# Copyright 2023 Patrick Goldinger
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
import sys
import time
import googlengram_wordlist
from devtools import corpusdata, flutils

DATA_DIR = os.path.join(flutils.dir_of(__file__), "../data/dicts/v0~draft1")
TMP_DIR = os.path.join(DATA_DIR, ".tmp/{lang}")
CORPUSDATA_DIR = os.path.join(flutils.dir_of(__file__), "../data/.corpusdata")
CORPUSDATA_CONFIG_PATH = os.path.join(flutils.dir_of(__file__), "../data/corpusdata-config.json")
CONFIG_PATH = os.path.join(flutils.dir_of(__file__), "../data/wiktextract-config.json")
DST_DICTIONARY = os.path.join(DATA_DIR, "words_{lang}.fldic")
SCORE_THRESHOLD = 2

WIKTEXTRACT = "wiktextract"
GOOGLENGRAM = "googlengram"
FILTER_NAME = "filter_name"
LANGUAGE_MAPPING = {
    "en-US": {
        WIKTEXTRACT: "en",
        GOOGLENGRAM: "en-US",
        FILTER_NAME: "en",
    },
    "en-GB": {
        WIKTEXTRACT: "en",
        GOOGLENGRAM: "en-GB",
        FILTER_NAME: "en",
    },
    "de": {
        WIKTEXTRACT: "de",
        GOOGLENGRAM: "de",
        FILTER_NAME: "de",
    },
    "fr": {
        WIKTEXTRACT: "fr",
        GOOGLENGRAM: "fr",
        FILTER_NAME: "root",
    },
    "es": {
        WIKTEXTRACT: "es",
        GOOGLENGRAM: "es",
        FILTER_NAME: "root",
    },
    "it": {
        WIKTEXTRACT: "it",
        GOOGLENGRAM: "it",
        FILTER_NAME: "root",
    },
    "ru": {
        WIKTEXTRACT: "ru",
        GOOGLENGRAM: "ru",
        FILTER_NAME: "root",
    },
}


def build_dictionary(lang_code: str) -> int:
    print(f"-> LANGUAGE: {lang_code}")
    corpusdata_config = corpusdata.parse_json_to_corpus_config(CORPUSDATA_CONFIG_PATH)

    tmp_dir = TMP_DIR.format(lang=lang_code)
    if os.path.isfile(tmp_dir):
        print(f"FATAL: Given temporary directory path '{tmp_dir}' is a file! Aborting.")
        return os.EX_USAGE
    os.makedirs(tmp_dir, exist_ok=True)

    language_config = LANGUAGE_MAPPING.get(lang_code, None)
    if lang_code is None:
        print(f"FATAL: Language '{lang_code}' is unsupported! Aborting.")
        return os.EX_USAGE

    wiktextract_lang = language_config[WIKTEXTRACT]
    googlengram_lang = language_config[GOOGLENGRAM]
    filter_name = language_config[FILTER_NAME]
    flutils.print_large_separator()

    print("Check if wiktextract data exists...")
    tmp_path = corpusdata_config.wiktextract.data_file(wiktextract_lang)
    tmp_path = os.path.join(CORPUSDATA_DIR, tmp_path) if tmp_path is not None else None
    if tmp_path is None or not os.path.exists(tmp_path) or not os.path.isfile(tmp_path):
        print(f"FATAL: Couldn't find required wiktextract source data! Aborting.")
        return 1
    print(f"Found at {tmp_path}")
    wiktextract_path = tmp_path
    flutils.print_separator()

    print("Check if Google Ngram data exists...")
    tmp_path = corpusdata_config.googlengram.data_dir(googlengram_lang)
    tmp_path = os.path.join(CORPUSDATA_DIR, tmp_path) if tmp_path is not None else None
    if tmp_path is None or not os.path.exists(tmp_path) or not os.path.isdir(tmp_path):
        print(f"FATAL: Couldn't find required googlengram source data! Aborting.")
        return 1
    print(f"Found at {tmp_path}")
    googlengram_dir = tmp_path
    flutils.print_large_separator()

    print("Build dictionary with wiktextract data\n")
    stats_path = os.path.join(tmp_dir, "wiktextract_stats.json")
    dictionary_path = DST_DICTIONARY.format(lang=lang_code)
    ret_code = flutils.prep_wiktextract(wiktextract_path, dictionary_path, CONFIG_PATH, filter_name, stats_path)
    if ret_code != os.EX_OK:
        print(f"FATAL: Failed to build dictionary (error code {ret_code})! Aborting.")
        return ret_code
    flutils.print_large_separator()

    print("Build filtered wordlist from Google Ngram data\n")
    wordlist_path = os.path.join(tmp_dir, "wordlist.txt")
    exclude_filters = googlengram_wordlist.parse_exclude_filters(CONFIG_PATH, filter_name)
    ret_code = googlengram_wordlist.generate_wordlist(googlengram_dir, wordlist_path, exclude_filters)
    if ret_code != os.EX_OK:
        print(f"FATAL: Failed to build filtered wordlist (error code {ret_code})! Aborting.")
        return ret_code
    flutils.print_large_separator()

    print("Insert scores into dictionary and apply threshold filtering\n")
    ret_code = flutils.train_wordscores(dictionary_path, wordlist_path, str(SCORE_THRESHOLD))
    if ret_code != os.EX_OK:
        print(f"FATAL: Failed insert scores (error code {ret_code})! Aborting.")
        return ret_code

    return os.EX_OK


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Utility which generates a dictionary for the given language code.",
    )
    parser.add_argument(
        "--lang",
        type=str,
        required=True,
        help="the 2-letter language code",
    )
    args = parser.parse_args()

    start_time = time.time()
    ret_code = build_dictionary(args.lang)
    if ret_code != os.EX_OK:
        sys.exit(ret_code)
    end_time = time.time()
    elapsed_time = end_time - start_time
    flutils.print_large_separator()
    print(f"Finished in {elapsed_time:.2f}s")
    sys.exit(os.EX_OK)


if __name__ == "__main__":
    main()
