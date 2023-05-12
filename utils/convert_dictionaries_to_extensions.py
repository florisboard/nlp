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

import flutils
import os
import sys
import time
import zipfile
import build_dictionary


EXTENSION_ID_TEMPLATE = "org.florisboard.dictionaries.{lang_code}"
EXTENSION_JSON_TEMPLATE = """{{
  "$": "ime.extension.dictionary",
  "meta": {{
    "id": "{ext_id}",
    "version": "0.1.0_v0~draft1",
    "title": "Dictionaries for {lang_code}",
    "homepage": "https://github.com/florisboard/nlp",
    "issueTracker": "https://github.com/florisboard/nlp/issues",
    "maintainers": ["Patrick Goldinger <patrick@patrickgold.dev>"],
    "license": "apache-2.0"
  }},
  "dictionaries": [
    {{
      "id": "{lang_code}",
      "label": "Base dictionary",
      "authors": ["Patrick Goldinger <patrick@patrickgold.dev>"],
      "locale": "{lang_code}"
    }}
  ]
}}"""


def convert_dictionaries_to_extensions(data_path: str) -> None:
    os.makedirs(data_path, exist_ok=True)
    ext_json_path = os.path.join(data_path, "~extension.json")
    for lang_code in build_dictionary.LANGUAGE_MAPPING.keys():
        print(f"-> LANGUAGE: {lang_code}")
        ext_id = EXTENSION_ID_TEMPLATE.format(lang_code=lang_code)
        with open(ext_json_path, "wt") as ext_json_file:
            ext_json_file.write(EXTENSION_JSON_TEMPLATE.format(ext_id=ext_id, lang_code=lang_code))
        ext_path = os.path.join(data_path, f"{ext_id}.flex")
        dict_path = os.path.join(data_path, f"words_{lang_code}.fldic")
        with zipfile.ZipFile(ext_path, "w") as ext_file:
            ext_file.write(ext_json_path, "extension.json")
            ext_file.write(dict_path, f"dictionaries/{lang_code}.fldic")
    os.remove(ext_json_path)


def main() -> None:
    start_time = time.time()
    convert_dictionaries_to_extensions(sys.argv[1])
    end_time = time.time()
    elapsed_time = end_time - start_time
    flutils.print_large_separator()
    print(f"(Total) Finished in {elapsed_time:.2f}s")
    sys.exit(os.EX_OK)


if __name__ == "__main__":
    main()
