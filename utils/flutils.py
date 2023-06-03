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

import os
import subprocess


def dir_of(path: str) -> str:
    return os.path.dirname(os.path.abspath(path))


def download(url: str, to_file: str | None = None) -> int:
    if to_file is None:
        return subprocess.run(["wget", url, "-q", "--show-progress"]).returncode
    else:
        return subprocess.run(["wget", "-O", to_file, url, "-q", "--show-progress"]).returncode


def prep_wiktextract(wiktextract_path: str, dict_path: str, config_path: str, filter_name: str, stats_path: str) -> int:
    nlptools_executable = os.path.join(dir_of(__file__), "../build/release/bin/nlptools")
    return subprocess.run(
        [
            nlptools_executable,
            "prep-wiktextract",
            "--src",
            wiktextract_path,
            "--dst",
            dict_path,
            "--config",
            config_path,
            "--filter",
            filter_name,
            "--stats",
            stats_path,
        ]
    ).returncode


def train_wordscores(dict_path: str, wordlist_path: str, score_threshold: str) -> int:
    nlptools_executable = os.path.join(dir_of(__file__), "../build/release/bin/nlptools")
    return subprocess.run(
        [
            nlptools_executable,
            "train-word-scores",
            "--dictionary",
            dict_path,
            "--wordlist",
            wordlist_path,
            "--score-threshold",
            score_threshold,
        ]
    ).returncode


def print_header(title: str) -> None:
    print(f">>> {title} <<<")
    print("-" * (len(title) + 8))


def print_separator() -> None:
    print("-----")


def print_large_separator() -> None:
    print("\n=====\n")


def ask_yes_no_question(question) -> bool:
    while True:
        response = input(f"{question} (yes/no): ").lower()
        if response == "yes" or response == "y":
            return True
        elif response == "no" or response == "n":
            return False
        else:
            print("Invalid response. Please enter 'yes' or 'no'.")
