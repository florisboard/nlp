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
import flutils
import os
import sys
import time
import googlengram_download
import googlengram_wordlist

TMP_DIR = os.path.join(flutils.dir_of(__file__), "../data/dicts/v0~draft1/.tmp/{lang}")
CONFIG_PATH = os.path.join(flutils.dir_of(__file__), "../data/wiktextract-config.json")
DST_DICTIONARY = os.path.join(flutils.dir_of(__file__), "../data/dicts/v0~draft1/words_{lang}.fldic")
SCORE_THRESHOLD = 2

WIKTEXTRACT = "wiktextract"
GOOGLENGRAM = "googlengram"
FILTER_NAME = "filter_name"
LANGUAGE_MAPPING = {
    "en-US": {
        WIKTEXTRACT: "https://kaikki.org/dictionary/English/kaikki.org-dictionary-English.json",
        GOOGLENGRAM: "https://storage.googleapis.com/books/ngrams/books/20200217/eng-us/eng-us-1-ngrams_exports.html",
        FILTER_NAME: "en",
    },
    "en-GB": {
        WIKTEXTRACT: "https://kaikki.org/dictionary/English/kaikki.org-dictionary-English.json",
        GOOGLENGRAM: "https://storage.googleapis.com/books/ngrams/books/20200217/eng-gb/eng-gb-1-ngrams_exports.html",
        FILTER_NAME: "en",
    },
    "de": {
        WIKTEXTRACT: "https://kaikki.org/dictionary/German/kaikki.org-dictionary-German.json",
        GOOGLENGRAM: "https://storage.googleapis.com/books/ngrams/books/20200217/ger/ger-1-ngrams_exports.html",
        FILTER_NAME: "de",
    },
    "fr": {
        WIKTEXTRACT: "https://kaikki.org/dictionary/French/kaikki.org-dictionary-French.json",
        GOOGLENGRAM: "https://storage.googleapis.com/books/ngrams/books/20200217/fre/fre-1-ngrams_exports.html",
        FILTER_NAME: "root",
    },
    "es": {
        WIKTEXTRACT: "https://kaikki.org/dictionary/Spanish/kaikki.org-dictionary-Spanish.json",
        GOOGLENGRAM: "https://storage.googleapis.com/books/ngrams/books/20200217/spa/spa-1-ngrams_exports.html",
        FILTER_NAME: "root",
    },
    "it": {
        WIKTEXTRACT: "https://kaikki.org/dictionary/Italian/kaikki.org-dictionary-Italian.json",
        GOOGLENGRAM: "https://storage.googleapis.com/books/ngrams/books/20200217/ita/ita-1-ngrams_exports.html",
        FILTER_NAME: "root",
    },
    "ru": {
        WIKTEXTRACT: "https://kaikki.org/dictionary/Russian/kaikki.org-dictionary-Russian.json",
        GOOGLENGRAM: "https://storage.googleapis.com/books/ngrams/books/20200217/rus/rus-1-ngrams_exports.html",
        FILTER_NAME: "root",
    },
}


def build_dictionary(language_code: str) -> int:
    tmp_dir = TMP_DIR.format(lang=language_code)
    if os.path.isfile(tmp_dir):
        print(f"FATAL: Given temporary directory path '{tmp_dir}' is a file! Aborting.")
        return os.EX_USAGE
    os.makedirs(tmp_dir, exist_ok=True)

    language_config = LANGUAGE_MAPPING.get(language_code, None)
    if language_code is None:
        print(f"FATAL: Language '{language_code}' is unsupported! Aborting.")
        return os.EX_USAGE

    wiktextract_url = language_config[WIKTEXTRACT]
    googlengram_url = language_config[GOOGLENGRAM]
    filter_name = language_config[FILTER_NAME]
    flutils.print_large_separator()

    print("Download wiktextract data\n")
    wiktextract_name = wiktextract_url.split("/")[-1]
    wiktextract_path = os.path.join(tmp_dir, wiktextract_name)
    if os.path.exists(wiktextract_path):
        print(f"Skip {wiktextract_name} (already exists)")
    else:
        print(f"Download {wiktextract_name}")
        ret_code = flutils.download(url=wiktextract_url, to_file=wiktextract_path)
        if ret_code != os.EX_OK:
            print(f"FATAL: Failed to download wiktextract file (error code {ret_code})! Aborting.")
            return ret_code
    flutils.print_large_separator()

    print("Download Google Ngram data\n")
    ret_code = googlengram_download.download_ngram_data(googlengram_url, tmp_dir)
    if ret_code != os.EX_OK:
        print(f"FATAL: Failed to download Google Ngram data! Aborting.")
        return ret_code
    flutils.print_large_separator()

    print("Build dictionary with wiktextract data\n")
    stats_path = os.path.join(tmp_dir, "wiktextract_stats.json")
    dictionary_path = DST_DICTIONARY.format(lang=language_code)
    ret_code = flutils.prep_wiktextract(wiktextract_path, dictionary_path, CONFIG_PATH, filter_name, stats_path)
    if ret_code != os.EX_OK:
        print(f"FATAL: Failed to build dictionary (error code {ret_code})! Aborting.")
        return ret_code
    flutils.print_large_separator()

    print("Build filtered wordlist from Google Ngram data\n")
    wordlist_path = os.path.join(tmp_dir, "wordlist.txt")
    exclude_filters = googlengram_wordlist.parse_exclude_filters(CONFIG_PATH, filter_name)
    ret_code = googlengram_wordlist.generate_wordlist(tmp_dir, wordlist_path, exclude_filters)
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
