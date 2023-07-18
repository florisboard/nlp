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
from devtools import icu4c, corpusdata


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

    corpusdata_parser = subparsers.add_parser(
        "corpusdata-download",
        help="helps downloading corpusdata",
        description="Utility which downloads Wiktextract and Google Ngram partition files from their respecitve sources and stores them in given dst dir.",
        epilog="See https://storage.googleapis.com/books/ngrams/books/datasetsv3.html for an overview of Google Ngram data.",
    )
    corpusdata_parser.add_argument(
        "--corpus-config",
        type=str,
        required=True,
        help="path for the corpus download config",
    )
    corpusdata_parser.add_argument(
        "--dst-dir",
        type=str,
        required=True,
        help="path for the dst corpus dir",
    )

    args = parser.parse_args()

    ret_code = os.EX_OK
    if args.command == "icu4c":
        ret_code = icu4c.upgrade(args.upgrade, args.y)
    elif args.command == "corpusdata-download":
        ret_code = corpusdata.download_all(args.corpus_config, args.dst_dir)
    else:
        raise ValueError("Unreachable")

    sys.exit(ret_code)


if __name__ == "__main__":
    main()
