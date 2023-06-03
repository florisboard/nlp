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
import re
import subprocess


ICU4C_RELEASE_TAG_REGEX = re.compile(r"^release-(?P<MAJOR>\d+)-(?P<MINOR>\d+)$")
ICU4C_VERSION_REGEX = re.compile(r"^(?P<MAJOR>\d+)\.(?P<MINOR>\d+)$")
ICU4C_CMAKE_VERSION_MAJOR_REGEX = re.compile(r"set\(ICU_VERSION_MAJOR\s+\d+\)")
ICU4C_CMAKE_VERSION_MINOR_REGEX = re.compile(r"set\(ICU_VERSION_MINOR\s+\d+\)")
ICU4C_CMAKE_VERSION_MAJOR_TEMPLATE = "set(ICU_VERSION_MAJOR {version})"
ICU4C_CMAKE_VERSION_MINOR_TEMPLATE = "set(ICU_VERSION_MINOR {version})"

ICU4C_MODULE_DIR = os.path.join(os.path.dirname(flutils.dir_of(__file__)), "../icu4c")
ICU4C_SOURCE_DIR = os.path.join(ICU4C_MODULE_DIR, "icu")
ICU4C_CMAKE_FILE = os.path.join(ICU4C_MODULE_DIR, "CMakeLists.txt")


def get_current_tag(cwd: str) -> str | None:
    result = subprocess.run(
        ["git", "log", "-n1", "--pretty='%h'"],
        capture_output=True,
        text=True,
        cwd=cwd,
    )
    if result.returncode != 0:
        return None
    result = subprocess.run(
        ["git", "describe", "--exact-match", "--tags", result.stdout.strip()[1:-1]],
        capture_output=True,
        text=True,
        cwd=cwd,
    )
    if result.returncode != 0:
        return None
    tag_name = result.stdout.strip()
    return tag_name


def upgrade(new_version: str, skip_confirm: bool) -> int:
    result = re.match(ICU4C_VERSION_REGEX, new_version.strip())
    if result and result.group("MAJOR") and result.group("MINOR"):
        new_version_major = int(result.group("MAJOR"))
        new_version_minor = int(result.group("MINOR"))
        new_icu4c_tag = f"release-{new_version_major}-{new_version_minor}"
    else:
        print(f"FATAL: Given version string '{new_version}' is not a valid version! Aborting.")
        return

    if not os.path.exists(os.path.join(ICU4C_SOURCE_DIR, ".git")):
        print(
            f"FATAL: ICU4C source directory '{ICU4C_SOURCE_DIR}' is not a git repo! Did you forget to checkout the \
              git submodules? Aborting."
        )
        return

    old_icu4c_tag = get_current_tag(ICU4C_SOURCE_DIR)
    if old_icu4c_tag is None:
        print(
            "FATAL: ICU4C source directory is not checked out to a tag. Abort to prevent losing potentially unsaved changes."
        )
        return

    result = re.match(ICU4C_RELEASE_TAG_REGEX, old_icu4c_tag)
    if result and result.group("MAJOR") and result.group("MINOR"):
        old_version_major = int(result.group("MAJOR"))
        old_version_minor = int(result.group("MINOR"))
    else:
        print("FATAL: ICU4C repo checked out at '{icu4c_tag}' is not a valid tag for upgrading! Aborting.")
        return

    print(f"Begin upgrading ICU4C")
    print(f"  Repository:      {ICU4C_SOURCE_DIR}")
    print(f"  Current version: {old_icu4c_tag}")
    print(f"  New version:     {new_icu4c_tag}")

    if new_version_major == old_version_major and new_version_minor == old_version_minor:
        print("FATAL: Current version same as version to upgrade to! Aborting.")
        return
    if (
        new_version_major < old_version_major
        or new_version_major == old_version_major
        and new_version_minor < old_version_minor
    ):
        print("FATAL: Current version more recent than version to upgrade to! Aborting.")
        return

    if not skip_confirm:
        flutils.print_separator()
        result = flutils.ask_yes_no_question("Please check the upgrade info. Proceed upgrading?")
        if result:
            print("Proceeding.")
        else:
            print("Aborting.")
            return os.EX_NOINPUT

    flutils.print_separator()
    print(f"Fetching updated repository info...")
    result = subprocess.run(["git", "fetch"], cwd=ICU4C_SOURCE_DIR)
    if result.returncode != 0:
        print(f"FATAL: Git fetch failed with exit code {result.returncode}! Aborting.")

    flutils.print_separator()
    print(f"Upgrading...")
    result = subprocess.run(["git", "checkout", new_icu4c_tag], cwd=ICU4C_SOURCE_DIR)
    if result.returncode != 0:
        print(f"FATAL: Git checkout failed with exit code {result.returncode}! Aborting.")

    flutils.print_separator()
    print(f"Adjusting version numbers in '{ICU4C_CMAKE_FILE}'...")
    with open(ICU4C_CMAKE_FILE, "rt") as cmake_file:
        cmake_contents = cmake_file.read()
    cmake_contents = re.sub(
        ICU4C_CMAKE_VERSION_MAJOR_REGEX,
        ICU4C_CMAKE_VERSION_MAJOR_TEMPLATE.format(version=new_version_major),
        cmake_contents,
    )
    cmake_contents = re.sub(
        ICU4C_CMAKE_VERSION_MINOR_REGEX,
        ICU4C_CMAKE_VERSION_MINOR_TEMPLATE.format(version=new_version_minor),
        cmake_contents,
    )
    with open(ICU4C_CMAKE_FILE, "wt") as cmake_file:
        cmake_file.write(cmake_contents)

    flutils.print_separator()
    print("Completed! Don't forget to commit the new submodule ref so it persists!")

    return os.EX_OK
