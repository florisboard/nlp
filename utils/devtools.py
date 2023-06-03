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
import sys
from devtools import icu4c


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Devtools which help manage this repository.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    icu4c_parser = subparsers.add_parser("icu4c", help="helps manage ICU4C sources and build configuration")
    icu4c_parser.add_argument(
        "--upgrade",
        type=str,
        required=True,
        help="upgrades to a new ICU4C version",
    )
    icu4c_parser.add_argument(
        "-y",
        action="store_true",
        help="if specified skips the confirmation dialog (for script mode)",
    )

    args = parser.parse_args()

    ret_code = os.EX_OK
    if args.command == "icu4c":
        ret_code = icu4c.upgrade(args.upgrade, args.y)
    else:
        raise ValueError("Unreachable")

    sys.exit(ret_code)


if __name__ == "__main__":
    main()
