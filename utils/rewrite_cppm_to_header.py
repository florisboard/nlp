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
import os
import re
import sys
from typing import Tuple


HEADER_PREFIX = "#ifndef __{header_name}__\n#define __{header_name}__\n\n"
HEADER_SUFFIX = "\n#endif\n"

EXTRACT_MODULE_AND_PARTITION_REGEX = re.compile(r"^(?:export\s*)?module\s*([a-zA-Z0-9_.]+)(?::([a-zA-Z0-9_]+))?;$")

REMOVE_WHOLE_LINE_REGEX = re.compile(r"^(?:(?:export)?\s*module.*?);$")
REWRITE_IMPORT_PARTITION_STATEMENTS = re.compile(r"^(?:export\s*)?import\s*:([a-zA-Z0-9_.]+);$")
REWRITE_IMPORT_MODULE_STATEMENTS = re.compile(r"^(?:export\s*)?import\s*([a-zA-Z0-9_.]+);$")
REMOVE_EXPORT_KEYWORD_REGEX = re.compile(r"^(?:export\s*(.*))$")


def extract_hmp(extract_match: re.Match[str]) -> Tuple[str, str, str]:
    module_name = extract_match.group(1).replace(".", "_")
    partition_name = f"_{extract_match.group(2)}" if extract_match.group(2) else ""
    return (f"{module_name}{partition_name}", module_name, partition_name)


def rewrite_cppm_to_header(cppm_path: str, header_dir: str) -> int:
    if not os.path.isfile(cppm_path):
        print(f"FATAL: Given cppm path '{cppm_path}' is not a file! Aborting.")
        return os.EX_USAGE
    os.makedirs(header_dir, exist_ok=True)

    with open(cppm_path, "r") as cppm_file:
        header_name, module_name, partion_name = (None, None, None)
        for line in cppm_file:
            result = re.match(EXTRACT_MODULE_AND_PARTITION_REGEX, line)
            if result:
                (header_name, module_name, partion_name) = extract_hmp(result)
                break
        assert header_name is not None, "Given cppm file is not a module or module partiton file"
        cppm_file.seek(0)

        header_path = os.path.join(header_dir, f"{header_name}.hpp")
        print(os.path.abspath(header_path))
        with open(header_path, "w") as header_file:
            header_file.write(HEADER_PREFIX.format(header_name=header_name.upper()))
            for line in cppm_file:
                if re.match(REMOVE_WHOLE_LINE_REGEX, line):
                    continue

                result = re.match(REWRITE_IMPORT_PARTITION_STATEMENTS, line.strip())
                if result:
                    incl_partiton_name = result.group(1)
                    header_file.write(f'#include "{module_name}_{incl_partiton_name}.hpp"\n')
                    continue

                result = re.match(REWRITE_IMPORT_MODULE_STATEMENTS, line)
                if result:
                    incl_module_name = result.group(1).replace(".", "_")
                    header_file.write(f'#include "{incl_module_name}.hpp"\n')
                    continue

                result = re.match(REMOVE_EXPORT_KEYWORD_REGEX, line)
                if result:
                    header_file.write(result.group(1))
                    header_file.write("\n")
                    continue

                header_file.write(line)

            header_file.write(HEADER_SUFFIX)

    return os.EX_OK


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Utility which rewrites a cppm file to a header file for compilation in Android NDK.",
    )
    parser.add_argument(
        "--cppm",
        type=str,
        required=True,
        help="the original cppm file path to rewrite",
    )
    parser.add_argument(
        "--header-dir",
        type=str,
        required=True,
        help="the output directory in which the header file will be written",
    )
    args = parser.parse_args()

    ret_code = rewrite_cppm_to_header(args.cppm, args.header_dir)
    if ret_code != os.EX_OK:
        sys.exit(ret_code)


if __name__ == "__main__":
    main()
