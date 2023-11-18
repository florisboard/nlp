#!/usr/bin/env sh

# Copyright 2023 Patrick Goldinger
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

dotenv_file="$(realpath .env.buildtools)"

cd "$(realpath "$(dirname "$0")")" || exit 1

if [ -f "$dotenv_file" ]; then
    # shellcheck source=/dev/null
    . "$dotenv_file"
    cmake_bin="${CMAKE_BIN:="cmake"}"
    llvm_toolchain="${LLVM_TOOLCHAIN:=""}"
else
    cmake_bin="cmake"
    llvm_toolchain=""
fi

is_build_command=0
for arg in "$@"; do
    if [ "$arg" = "--build" ]; then
        is_build_command=1
        break
    fi
done

if [ "$#" -eq 0 ]; then
    "$cmake_bin" --version
elif [ $is_build_command -eq 1 ]; then
    "$cmake_bin" "$@"
else
    "$cmake_bin" -DLLVM_TOOLCHAIN="$llvm_toolchain" "$@"
fi
exit $?
