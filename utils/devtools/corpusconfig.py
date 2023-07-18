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


class CorpusConfig:
    def __init__(self) -> None:
        self.wiktextract = WiktextractCorpusConfig()
        self.googlengram = GoogleNgramCorpusConfig()


class WiktextractCorpusConfig:
    ID: str = "wiktextract"

    def __init__(self) -> None:
        self.excluded_source_elements: list[str] = []
        self.sources: dict[str, WiktextractCorpusConfig.SourceInfo] = {}

    def data_file(self, lang: str) -> str | None:
        if lang in self.sources:
            return f"{self.ID}/{self.sources[lang].filtered_file}"
        return None

    class SourceInfo:
        def __init__(self, url: str, original_file: str, filtered_file: str) -> None:
            self.url: str = url
            self.original_file: str = original_file
            self.filtered_file: str = filtered_file


class GoogleNgramCorpusConfig:
    ID: str = "googlengram"

    def __init__(self) -> None:
        self.sources: dict[str, GoogleNgramCorpusConfig.SourceInfo] = {}

    def data_dir(self, lang: str) -> str | None:
        if lang in self.sources:
            return f"{self.ID}/{lang}"
        return None

    class SourceInfo:
        def __init__(self, url_1gram: str, url_2gram: str, url_3gram: str, url_4gram: str, url_5gram: str) -> None:
            self.url_1gram: str = url_1gram
            self.url_2gram: str = url_2gram
            self.url_3gram: str = url_3gram
            self.url_4gram: str = url_4gram
            self.url_5gram: str = url_5gram


def parse_json_to_corpus_config(file_path) -> CorpusConfig:
    with open(file_path) as f:
        data = json.load(f)

    config = CorpusConfig()

    # Parse wiktextract data
    wiktextract_data = data.get(WiktextractCorpusConfig.ID)
    if wiktextract_data:
        excluded_source_elements: list[str] | None = wiktextract_data.get("excludedSourceElements")
        if excluded_source_elements:
            config.wiktextract.excluded_source_elements = excluded_source_elements

        sources = wiktextract_data.get("sources")
        if sources:
            for source_name, source_info in sources.items():
                url = source_info.get("url")
                original_file = source_info.get("originalFile")
                filtered_file = source_info.get("filteredFile")
                if url and original_file and filtered_file:
                    config.wiktextract.sources[source_name] = WiktextractCorpusConfig.SourceInfo(
                        url, original_file, filtered_file
                    )

    # Parse googlengram data
    googlengram_data = data.get(GoogleNgramCorpusConfig.ID)
    if googlengram_data:
        sources = googlengram_data.get("sources")
        if sources:
            for source_name, source_info in sources.items():
                url_1gram = source_info.get("url_1gram")
                url_2gram = source_info.get("url_2gram")
                url_3gram = source_info.get("url_3gram")
                url_4gram = source_info.get("url_4gram")
                url_5gram = source_info.get("url_5gram")

                if url_1gram and url_2gram and url_3gram and url_4gram and url_5gram:
                    config.googlengram.sources[source_name] = GoogleNgramCorpusConfig.SourceInfo(
                        url_1gram, url_2gram, url_3gram, url_4gram, url_5gram
                    )

    return config
