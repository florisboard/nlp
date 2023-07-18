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

import json
import os
import re
import time
from devtools import flutils
from devtools.corpusconfig import *


HTML_LINK_SCRAPING_REGEX = r"<li><a href=\"(.*?)\">.*<\/a><\/li>"


def filter_json_data(corpus_config: CorpusConfig, json_data):
    filtered_json_data = {
        key: value for key, value in json_data.items() if key not in corpus_config.wiktextract.excluded_source_elements
    }
    if "senses" in filtered_json_data:
        filtered_json_data["senses"] = [
            {
                key: value
                for key, value in sense.items()
                if key not in corpus_config.wiktextract.excluded_source_elements
            }
            for sense in filtered_json_data["senses"]
        ]
    return filtered_json_data


def filter_wiktextract_file(corpus_config: CorpusConfig, kaikki_path: str, filtered_kaikki_path: str) -> None:
    with open(kaikki_path, "r") as kaikki_file:
        with open(filtered_kaikki_path, "w") as filtered_kaikki_file:
            for line in kaikki_file:
                json_data = json.loads(line)
                filtered_json_data = filter_json_data(corpus_config, json_data)
                filtered_kaikki_file.write(json.dumps(filtered_json_data))
                filtered_kaikki_file.write("\n")


def download_wiktextract(corpus_config: CorpusConfig, dst_dir: str) -> int:
    flutils.print_header("DOWNLOAD WIKTEXTRACT")
    for lang_tag, source in corpus_config.wiktextract.sources.items():
        print(f"[{lang_tag}]")
        kaikki_path = os.path.join(dst_dir, source.original_file)
        filtered_kaikki_path = os.path.join(dst_dir, source.filtered_file)
        if os.path.exists(kaikki_path):
            print(f"Skip {kaikki_path} (already exists)")
        else:
            print(f"Download {kaikki_path}")
            ret_code = flutils.download(url=source.url, to_file=kaikki_path)
            if ret_code != 0:
                print(f"WARN: Failed to complete download (error code {ret_code})")
                os.remove(kaikki_path)
                flutils.print_separator()
                continue
        print(f"Filtering {kaikki_path}")
        filter_wiktextract_file(corpus_config, kaikki_path, filtered_kaikki_path)
        flutils.print_separator()

    return os.EX_OK


def download_googlengram_specific_data(index_file_url: str, dst_dir: str) -> int:
    if os.path.isfile(dst_dir):
        print(f"FATAL: Given output directory path '{dst_dir}' is a file! Aborting.")
        return os.EX_USAGE
    os.makedirs(dst_dir, exist_ok=True)

    index_name = index_file_url.split("/")[-1]
    index_file_path = os.path.join(dst_dir, index_name)
    indexed_links: list[str] = []

    print(f"Download ngram index")
    ret_code = flutils.download(url=index_file_url, to_file=index_file_path)
    if ret_code != os.EX_OK:
        print(f"FATAL: Index file download failed with error code {ret_code}! Aborting.")
        return ret_code

    with open(index_file_path, "r") as index_file:
        for line in index_file:
            link_match = re.search(HTML_LINK_SCRAPING_REGEX, line)
            if link_match:
                link = link_match.group(1)
                indexed_links.append(link)
    os.remove(index_file_path)
    print(f"Discovered and queued {len(indexed_links)} partition files to be downloaded")

    for link in indexed_links:
        partition_name = link.split("/")[-1]
        partition_path = os.path.join(dst_dir, partition_name)
        if os.path.exists(partition_path):
            print(f"Skip {partition_name} (already exists)")
            continue
        print(f"Download {partition_name}")
        ret_code = flutils.download(url=link, to_file=partition_path)
        if ret_code != 0:
            print(f"WARN: Failed to complete download (error code {ret_code})")

    return os.EX_OK


def download_googlengram_data(corpus_config: CorpusConfig, dst_dir: str) -> int:
    flutils.print_header("DOWNLOAD GOOGLENGRAM")
    for lang_tag, source in corpus_config.googlengram.sources.items():
        print(f"[{lang_tag}]")
        lang_specific_dir = os.path.join(dst_dir, lang_tag)
        download_googlengram_specific_data(source.url_1gram, lang_specific_dir)
        # download_googlengram_specific_data(source.url_2gram, lang_specific_dir)
        # download_googlengram_specific_data(source.url_3gram, lang_specific_dir)
        # download_googlengram_specific_data(source.url_4gram, lang_specific_dir)
        # download_googlengram_specific_data(source.url_5gram, lang_specific_dir)
        flutils.print_separator()

    return os.EX_OK


def download_all(corpus_config_path: str, corpusdata_dir: str) -> int:
    corpus_config = parse_json_to_corpus_config(corpus_config_path)

    wiktextract_dir = os.path.join(corpusdata_dir, WiktextractCorpusConfig.ID)
    os.makedirs(wiktextract_dir, exist_ok=True)
    googlengram_dir = os.path.join(corpusdata_dir, GoogleNgramCorpusConfig.ID)
    os.makedirs(googlengram_dir, exist_ok=True)

    start_time = time.time()
    download_wiktextract(corpus_config, wiktextract_dir)
    end_time = time.time()
    elapsed_time = end_time - start_time
    print(f"Finished in {elapsed_time:.2f}s")

    flutils.print_large_separator()

    start_time = time.time()
    download_googlengram_data(corpus_config, googlengram_dir)
    end_time = time.time()
    elapsed_time = end_time - start_time
    print(f"Finished in {elapsed_time:.2f}s")

    return os.EX_OK
