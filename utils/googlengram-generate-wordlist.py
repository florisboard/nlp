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

import gzip
import os
import sys
from typing import Tuple

def print_usage() -> None:
    print(f"""
Utility which reads unigram partition files from a Google Ngram Viewer export and generates a wordlist from the data.

Usage: ./{__file__} [--help] <UNIGRAM_DIR_PATH> <WORDLIST_PATH>

Arguments:
  <UNIGRAM_DIR_PATH>    The path of the directory where unigram partitions should be read.
  <WORDLIST_PATH>       The path of the wordlist file. If a file at this location exists it will be overwritten.
  --help                Prints this help message and quits.

---
See https://storage.googleapis.com/books/ngrams/books/datasetsv3.html for an overview of Google Ngram data.
""".strip())

def parse_line(line: str) -> Tuple[str, int]:
    line_components = line.split("\t")
    word = "".join(line_components[0].split("_")[:-1])
    if len(line_components) == 0:
        return (word, 0)
    word_count: int = 0
    for data_point in line_components[1:]:
        data_point_components = data_point.split(",")
        if len(data_point_components) != 3:
            continue
        word_count += int(data_point_components[1])
    return (word, word_count)

def generate_wordlist(unigram_dir: str, wordlist_path: str) -> None:
    if not os.path.exists(unigram_dir):
        print(f"FATAL: Given unigram directory path '{unigram_dir}' does not exist! Aborting.")
    if not os.path.isdir(unigram_dir):
        print(f"FATAL: Given unigram directory path '{unigram_dir}' is a file! Aborting.")

    for partition_file_name in os.listdir(unigram_dir):
        if partition_file_name.startswith("1-") and partition_file_name.endswith(".gz"):
            print(f"Parse {partition_file_name}")
            with gzip.open(f"{unigram_dir}/{partition_file_name}", "rt") as partition_file:
                with open(wordlist_path, "w") as wordlist_file:
                    for line in partition_file:
                        [word, word_count] = parse_line(line)
                        if len(word) > 0:
                            wordlist_file.write(f"{word}\t{word_count}\n")
        else:
            print(f"Skip {partition_file_name}")
    print(f"Written result to {wordlist_path}")


def main() -> None:
    if sys.argv.count("--help") > 0:
        print_usage()
    else:
        generate_wordlist(sys.argv[1], sys.argv[2])

if __name__ == "__main__":
    main()
