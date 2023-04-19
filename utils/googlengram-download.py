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
import re
import time

HTML_LINK_SCRAPING_REGEX = r"<li><a href=\"(.*?)\">.*<\/a><\/li>"


def download_ngram_data(index_file_url: str, dst_dir: str) -> None:
    if os.path.isfile(dst_dir):
        print(f"FATAL: Given output directory path '{dst_dir}' is a file! Aborting.")
        return
    os.makedirs(dst_dir, exist_ok=True)

    index_name = index_file_url.split("/")[-1]
    index_file_path = os.path.join(dst_dir, index_name)
    indexed_links: list[str] = []

    print(f"Download ngram index")
    result = flutils.download(url=index_file_url, to_file=index_file_path)
    if result.returncode != 0:
        print(f"Index file download failed with error code {result.returncode}!")
        return

    with open(index_file_path, "r") as index_file:
        for line in index_file:
            link_match = re.search(HTML_LINK_SCRAPING_REGEX, line)
            if link_match:
                link = link_match.group(1)
                indexed_links.append(link)
    os.remove(index_file_path)
    print(f"Discovered and queued {len(indexed_links)} partition files to be downloaded")
    flutils.print_separator()

    for link in indexed_links:
        partition_name = link.split("/")[-1]
        partition_path = os.path.join(dst_dir, partition_name)
        if os.path.exists(partition_path):
            print(f"Skip {partition_name} (already exists)")
            continue
        print(f"Download {partition_name}")
        result = flutils.download(url=link, to_file=partition_path)
        if result.returncode != 0:
            print(f"Failed to complete download (error code {result.returncode})")
    flutils.print_separator()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Utility which downloads unigram/ngram partition files from a Google Ngram Viewer export and stores them in given path.",
        epilog="See https://storage.googleapis.com/books/ngrams/books/datasetsv3.html for an overview of Google Ngram data.",
    )
    parser.add_argument(
        "--url",
        type=str,
        required=True,
        help="the URL of the Google Ngram data HTML page, e.g.: https://storage.googleapis.com/books/ngrams/books/20200217/eng/eng-1-ngrams_exports.html",
    )
    parser.add_argument(
        "--dst-dir",
        type=str,
        required=True,
        help="the output directory path, will be created if it does not exist",
    )
    args = parser.parse_args()

    start_time = time.time()
    download_ngram_data(args.url, args.dst_dir)
    end_time = time.time()
    elapsed_time = end_time - start_time
    print(f"Finished in {elapsed_time:.2f}s")


if __name__ == "__main__":
    main()
