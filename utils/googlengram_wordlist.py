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
import gzip
import json
import os
import re
import sys
import time
from concurrent.futures import ProcessPoolExecutor
from typing import Pattern, Tuple
from devtools import flutils


def parse_line(line: str) -> Tuple[str, int]:
    line_components = line.split("\t")
    if len(line_components) <= 1:
        return (line, 0)

    word = line_components[0].rsplit("_", 1)[0]
    word_count: int = 0

    for data_point in line_components[1:]:
        data_point_components = data_point.split(",")
        if len(data_point_components) != 3:
            continue
        word_count += int(data_point_components[1])

    return (word, word_count)


def parse_partition_file(partition_file_path: str, exclude_filters: list[Pattern[str]]) -> dict[str, int]:
    print(f"Parse {partition_file_path}")
    words = dict()
    with gzip.open(partition_file_path, "rt") as partition_file:
        for line in partition_file:
            [word, word_count] = parse_line(line)
            if is_excluded(word, exclude_filters):
                continue
            words[word] = words.get(word, 0) + word_count
    return words


def parse_partition_file_wrapper(args) -> dict[str, int]:
    return parse_partition_file(*args)


def is_excluded(word: str, exclude_filters: list[Pattern[str]]) -> bool:
    return next(filter(lambda f: f.match(word), exclude_filters), None) is not None


def merge_dicts(dict1: dict[str, int], dict2: dict[str, int]) -> dict[str, int]:
    for key, value in dict2.items():
        dict1[key] = dict1.get(key, 0) + value
    return dict1


def generate_wordlist(unigram_dir: str, wordlist_path: str, exclude_filters: list[Pattern[str]]) -> int:
    if not os.path.exists(unigram_dir):
        print(f"FATAL: Given unigram directory path '{unigram_dir}' does not exist! Aborting.")
        return os.EX_USAGE
    if not os.path.isdir(unigram_dir):
        print(f"FATAL: Given unigram directory path '{unigram_dir}' is a file! Aborting.")
        return os.EX_USAGE

    partition_files = []
    for file_name in os.listdir(unigram_dir):
        file_path = os.path.join(unigram_dir, file_name)
        if file_name.startswith("1-") and file_name.endswith(".gz"):
            print(f"Queue {file_path}")
            partition_files.append((file_path, exclude_filters))
        else:
            print(f"Skip {file_path}")
    flutils.print_separator()

    with ProcessPoolExecutor() as executor:
        results = executor.map(parse_partition_file_wrapper, partition_files)

    words = dict()
    for result in results:
        words = merge_dicts(words, result)

    print(f"Write result to {wordlist_path}")
    with open(wordlist_path, "w") as wordlist_file:
        for word, word_count in words.items():
            wordlist_file.write(word)
            wordlist_file.write("\t")
            wordlist_file.write(str(word_count))
            wordlist_file.write("\n")

    return os.EX_OK


def parse_exclude_filters(wiktextract_config_path: str, filter_name: str) -> list[Pattern[str]]:
    if len(wiktextract_config_path) == 0 or not os.path.isfile(wiktextract_config_path):
        return list()
    with open(wiktextract_config_path, "r") as wiktextract_config_file:
        wiktextract_config = json.loads(wiktextract_config_file.read())
        for filter_config in wiktextract_config["filters"]:
            if filter_config["name"] == filter_name:
                excluded_filters = list()
                for pattern in filter_config["excluded"]["words"]:
                    excluded_filters.append(re.compile(pattern))
        return excluded_filters


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Utility which reads unigram partition files from a Google Ngram Viewer export and generates a wordlist from the data.",
        epilog="See https://storage.googleapis.com/books/ngrams/books/datasetsv3.html for an overview of Google Ngram data.",
    )
    parser.add_argument(
        "--src-dir",
        type=str,
        required=True,
        help="the path of the directory where unigram partitions should be read",
    )
    parser.add_argument(
        "--dst-wordlist",
        type=str,
        required=True,
        help="the path of the wordlist file; if a file at this location exists it will be overwritten",
    )
    parser.add_argument(
        "--wiktextract-config",
        type=str,
        default="",
        help="the path of a wiktextract config file; default: %(default)s",
    )
    parser.add_argument(
        "--wiktextract-filter",
        type=str,
        default="root",
        help="the name of a wiktextract config exclude filter to use; default: %(default)s",
    )
    args = parser.parse_args()

    start_time = time.time()
    exclude_filters = parse_exclude_filters(args.wiktextract_config, args.wiktextract_filter)
    ret_code = generate_wordlist(args.src_dir, args.dst_wordlist, exclude_filters)
    if ret_code != os.EX_OK:
        sys.exit(ret_code)
    end_time = time.time()
    elapsed_time = end_time - start_time
    flutils.print_separator()
    print(f"Finished in {elapsed_time:.2f}s")
    sys.exit(os.EX_OK)


if __name__ == "__main__":
    main()
