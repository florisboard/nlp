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
import build_dictionary
import convert_dictionaries_to_extensions


def main() -> None:
    start_time = time.time()
    for lang_code in build_dictionary.LANGUAGE_MAPPING.keys():
        print(f"-> LANGUAGE: {lang_code}")
        build_dictionary.build_dictionary(lang_code)
        flutils.print_large_separator()
    convert_dictionaries_to_extensions.convert_dictionaries_to_extensions(build_dictionary.DATA_DIR)
    end_time = time.time()
    elapsed_time = end_time - start_time
    flutils.print_large_separator()
    print(f"(Total) Finished in {elapsed_time:.2f}s")
    sys.exit(os.EX_OK)


if __name__ == "__main__":
    main()
