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
import re
import subprocess
import sys

HTML_LINK_SCRAPING_REGEX = r"<li><a href=\"(.*?)\">.*<\/a><\/li>"

def print_usage() -> None:
    print(f"""
Utility which downloads unigram/ngram partition files from a Google Ngram Viewer export and stores them in given path.

Usage: ./{__file__} [--help] <GOOGLE_NGRAM_PATH> <OUTPUT_DIRECTORY_PATH>

Arguments:
  <GOOGLE_NGRAM_URL>    The URL of the Google Ngram data HTML page.
                        e.g.: https://storage.googleapis.com/books/ngrams/books/20200217/eng/eng-1-ngrams_exports.html
  <OUTPUT_DIR_PATH>     The output directory path. If it does not exist it will be created.
  --help                Prints this help message and quits.

---
See https://storage.googleapis.com/books/ngrams/books/datasetsv3.html for an overview of Google Ngram data.
""".strip())

def download_with_wget(index_file_url: str, dst_dir: str) -> None:
    if os.path.isfile(dst_dir):
        print(f"FATAL: Given output directory path '{dst_dir}' is a file! Aborting.")
        return
    os.makedirs(dst_dir)

    index_name = index_file_url.split("/")[-1]
    index_file_path = f"{dst_dir}/{index_name}"
    indexed_links: list[str] = []

    wget = subprocess.run(["wget", "-O", index_file_path, index_file_url])
    if wget.returncode != 0:
        print(f"Index file download failed with error code {wget.returncode}!")
        return

    with open(index_file_path, "r") as index_file:
        for line in index_file:
            link_match = re.search(HTML_LINK_SCRAPING_REGEX, line)
            if link_match:
                link = link_match.group(1)
                indexed_links.append(link)
    os.remove(index_file_path)

    for link in indexed_links:
        partition_name = link.split("/")[-1]
        partition_path = f"{dst_dir}/{partition_name}"
        wget = subprocess.run(["wget", "-O", partition_path, link])


def main() -> None:
    if sys.argv.count("--help") > 0:
        print_usage()
    else:
        download_with_wget(sys.argv[1], sys.argv[2])

if __name__ == "__main__":
    main()
