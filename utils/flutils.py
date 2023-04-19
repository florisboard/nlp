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

import subprocess


def download(url: str, to_file: str | None = None) -> subprocess.CompletedProcess[bytes]:
    if to_file is None:
        return subprocess.run(["wget", url, "-q", "--show-progress"])
    else:
        return subprocess.run(["wget", "-O", to_file, url, "-q", "--show-progress"])


def print_separator() -> None:
    print("-----")
