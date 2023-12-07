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

###### Constants ######

cmake_min_version="3.28.0"
cmake_version="3.28.0"
llvm_min_version="16.0.0"
llvm_version="17.0.5"
buildtools_dir="$(realpath buildtools)"
dotenv_file=".env.buildtools"

###### Functions ######

# Checks if a given SemVer string is valid (major.minor.patch format)
#
# @param $1 The SemVer string to check
# @returns 0 if valid, 1 otherwise
semver_isvalid() {
    major=$(echo "$1" | awk -F '.' '{print $1}')
    minor=$(echo "$1" | awk -F '.' '{print $2}')
    patch=$(echo "$1" | awk -F '.' '{print $3}')
    if [ -z "$major" ] || [ -z "$minor" ] || [ -z "$patch" ]; then
        return 1 # invalid
    else
        return 0 # valid
    fi
}

# Compares 2 given SemVer strings and returns which is greater
# This function assumes that all given SemVer strings are valid, passing invalid SemVer strings is undefined behavior.
#
# @param $1 SemVer string #1
# @param $2 SemVer string #2
# @returns 1 if $1>$2, 2 if $2>$1, 0 if $1==$2
semver_compare_gt() {
    major1=$(echo "$1" | awk -F '.' '{print $1}')
    minor1=$(echo "$1" | awk -F '.' '{print $2}')
    patch1=$(echo "$1" | awk -F '.' '{print $3}')
    major2=$(echo "$2" | awk -F '.' '{print $1}')
    minor2=$(echo "$2" | awk -F '.' '{print $2}')
    patch2=$(echo "$2" | awk -F '.' '{print $3}')

    if [ "$major1" -ne "$major2" ]; then
        [ "$major1" -gt "$major2" ] && return 1 || return 2
    fi
    if [ "$minor1" -ne "$minor2" ]; then
        [ "$minor1" -gt "$minor2" ] && return 1 || return 2
    fi
    if [ "$patch1" -ne "$patch2" ]; then
        [ "$patch1" -gt "$patch2" ] && return 1 || return 2
    fi

    return 0
}

llvm_version_satisfied() {
    semver_compare_gt "$1" "$llvm_min_version"
    result=$?
    [ $result -eq 2 ] && return 1 || return 0
}

cmake_version_satisfied() {
    semver_compare_gt "$1" "$cmake_min_version"
    result=$?
    [ $result -eq 2 ] && return 1 || return 0
}

macos_version_atleast() {
    macos_version=$(sw_vers -productVersion)
    [ "$macos_version" = "$(printf "%s\n%s\n" "$macos_version" "$1" | sort -V | tail -n1)" ]
}

###### Script ######

cd "$(realpath "$(dirname "$0")")" || exit 1
rm "$dotenv_file" 2>/dev/null
touch "$dotenv_file"
mkdir -p "$buildtools_dir"

missing_pkg_list=$(mktemp)
dotenv_cmake_bin=""
dotenv_llvm_toolchain=""
if [ "$GITHUB_ACTIONS" = "true" ]; then
    wget_progress_flag=""
else
    wget_progress_flag="--show-progress"
fi

printf "\033[1m\033[1;35mLLVM toochain setup\033[0m\n\n"

printf "\033[1mSearching for CMake installations on this system\033[0m\n"
cmake_version_installed=$(cmake --version | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
cmake_require_download=0
if [ -n "$cmake_version_installed" ]; then
    echo "  Found CMake $cmake_version_installed"
    if cmake_version_satisfied "$cmake_version_installed"; then
        echo "    CMake is compatible with project"
        dotenv_cmake_bin="cmake"
    else
        echo "    CMake is not compatible with project -> require precompiled download"
        cmake_require_download=1
    fi
else
    echo "  No CMake found on this system"
    echo "  Checking package manager for compatible versions"
    cmake_version_pkg_manager=$(apt-cache policy cmake | grep 'Candidate:' | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
    if [ -n "$cmake_version_pkg_manager" ]; then
        echo "    Found candidate package CMake $cmake_version_pkg_manager"
        if cmake_version_satisfied "$cmake_version_installed"; then
            echo "    Candidate is compatible with project -> marking for install"
            echo "cmake" >> "$missing_pkg_list"
            dotenv_cmake_bin="cmake"
        else
            echo "    Candidate is not compatible with project -> require precompiled download"
            cmake_require_download=1
        fi
    fi
fi
echo

if [ $cmake_require_download -eq 1 ]; then
    printf "\033[1mDownloading CMake %s\033[0m\n" "$cmake_version"
    if [ "$(uname)" = "Linux" ]; then
        if [ "$(uname -m)" = "x86_64" ]; then
            cmake_target="linux-x86_64"
        elif [ "$(uname -m)" = "aarch64" ]; then
            cmake_target="linux-aarch64"
        else
            echo "  FATAL: Unsupported Linux arch: $(uname -m)" >&2
            rm "$missing_pkg_list"
            exit 1
        fi
    elif [ "$(uname)" = "Darwin" ]; then
        if macos_version_atleast "10.10"; then
            cmake_target="macos10.10-universal"
        else
            cmake_target="macos-universal"
        fi
    else
        echo "  FATAL: Unsupported host OS: $(uname)" >&2
        rm "$missing_pkg_list"
        exit 1
    fi
    cmake_installation_name="cmake-$cmake_version-$cmake_target"
    cmake_tar_gz_name="$cmake_installation_name.tar.gz"
    cmake_download_link="https://github.com/Kitware/CMake/releases/download/v$cmake_version/$cmake_tar_gz_name"
    echo "  Source: $cmake_download_link"
    echo "  Destination: $buildtools_dir/$cmake_tar_gz_name"
    wget -q $wget_progress_flag -nc -P "$buildtools_dir" "$cmake_download_link"
    echo "  Extracting $buildtools_dir/$cmake_tar_gz_name"
    rm "$buildtools_dir/$cmake_installation_name" 2>/dev/null
    tar -xzf "$buildtools_dir/$cmake_tar_gz_name" -C "$buildtools_dir"
    echo "  CMake $cmake_version is now available at $buildtools_dir/$cmake_installation_name"
    dotenv_cmake_bin="$buildtools_dir/$cmake_installation_name/bin/cmake"
    echo
fi

printf "\033[1mSearching for LLVM installations on this system\033[0m\n"
sel_llvm_tc=""
sel_llvm_tc_v="0.0.0"
candidates=$(mktemp)
find /usr -type f -name 'clang' > "$candidates" 2>/dev/null
while read -r clang_candidate; do
    llvm_tc=$(realpath "$(dirname "$clang_candidate")/..")
    echo "  Found candidate for LLVM toolchain: $llvm_tc"
    llvm_tc_v=$($clang_candidate --version | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
    if ! semver_isvalid "$llvm_tc_v"; then
        echo "  Version: failed to identify, skipping..."
        continue
    fi
    echo "    Version: $llvm_tc_v"
    semver_compare_gt "$llvm_tc_v" "$sel_llvm_tc_v"
    result=$?
    if [ $result -eq 1 ]; then
        if [ -z "$sel_llvm_tc" ]; then
            echo "    Marking candidate"
        else
            echo "    Marking candidate (overwriting previous)"
        fi
        sel_llvm_tc="$llvm_tc"
        sel_llvm_tc_v="$llvm_tc_v"
    elif [ $result -eq 2 ]; then
        echo "    Skipping candidate"
    fi
done < "$candidates"
rm "$candidates"
echo

llvm_properly_configured=0
if llvm_version_satisfied "$sel_llvm_tc_v"; then
    printf "\033[1mSelect candidate toolchain %s\033[0m\n" "$sel_llvm_tc"
    echo "  Checking installed features:"
    feat_clang_tools=0
    feat_libcxx_dev=0
    feat_libcxxabi_dev=0
    printf "    clang-tools: "
    if [ -f "$sel_llvm_tc/bin/clang-scan-deps" ]; then
        echo "installed"
        feat_clang_tools=1
    else
        echo "missing"
    fi
    printf "    libc++-dev:  "
    if [ -d "$sel_llvm_tc/include/c++" ]; then
        echo "installed"
        feat_libcxx_dev=1
    else
        echo "missing"
    fi
    printf "    libc++-abi:  "
    if [ -f "$sel_llvm_tc/include/cxxabi.h" ]; then
        echo "installed"
        feat_libcxxabi_dev=1
    else
        echo "missing"
    fi
    if [ $feat_clang_tools -eq 1 ] && [ $feat_libcxx_dev -eq 1 ] && [ $feat_libcxxabi_dev -eq 1 ]; then
        echo "  LLVM toolchain $sel_llvm_tc is properly configured!"
        llvm_properly_configured=1
        dotenv_llvm_toolchain="$sel_llvm_tc"
    else
        echo "  LLVM toolchain $sel_llvm_tc misses features, need to lookup in package manager!"
    fi
fi
echo

llvm_require_download=0
missing_pkg_str=$(tr '\n' ' ' < "$missing_pkg_list")
if [ "$llvm_properly_configured" -eq 0 ]; then
    printf "\033[1mPerform loopuk for LLVM toolchain in package manager\033[0m\n"
    min_llvm_v_major=$(echo "$llvm_min_version" | awk -F '.' '{print $1}')
    llvm_v_major=$(echo "$sel_llvm_tc_v" | awk -F '.' '{print $1}')
    if [ -n "$sel_llvm_tc_v" ] && [ "$llvm_v_major" -ge "$min_llvm_v_major" ]; then
        echo "  Found toolchain $sel_llvm_tc"
        echo "    Selecting major version $llvm_v_major (toolchain version)"
    else
        llvm_v_major=$((min_llvm_v_major - 1))
        next_llvm_v_major=$min_llvm_v_major
        echo "  No toolchain found which satisfies >=$min_llvm_v_major major version requirement"
        echo "    Searching for best version in package manager:"
        while true; do
            policy=$(apt-cache policy "clang-$next_llvm_v_major")
            is_available=$(echo "$policy" | grep 'Candidate:' | grep -v '(None)')
            if [ -z "$is_available" ]; then
                echo "    clang-$next_llvm_v_major: not available -> stopping search"
                break
            else
                echo "    clang-$next_llvm_v_major: available -> continuing search"
                llvm_v_major=$((llvm_v_major + 1))
                next_llvm_v_major=$((next_llvm_v_major + 1))
            fi
        done
        if [ "$llvm_v_major" -ge "$min_llvm_v_major" ]; then
            echo "    Selecting major version $llvm_v_major"
        else
            echo "    No suitable version found -> require prebuilt download"
            llvm_require_download=1
        fi
    fi
    if [ $llvm_require_download -eq 0 ]; then
        required_pkg_list=$(mktemp)
        echo "clang-$llvm_v_major
        clang-tools-$llvm_v_major
        libc++-$llvm_v_major-dev
        libc++abi-$llvm_v_major-dev" > "$required_pkg_list"
        echo "  Checking required packages:"
        while read -r required_pkg; do
            is_installed=$(apt-cache policy "$required_pkg" | grep 'Installed:' | grep -v '(none)')
            if [ -n "$is_installed" ]; then
                echo "    $required_pkg: installed"
            else
                echo "    $required_pkg: missing -> marking for installation"
                echo "$required_pkg" >> "$missing_pkg_list"
            fi
        done < "$required_pkg_list"
        rm "$required_pkg_list"
        missing_pkg_str=$(tr '\n' ' ' < "$missing_pkg_list")
        if [ -z "$missing_pkg_str" ]; then
            echo "  All required packages installed!"
            echo "    LLVM toolchain is properly configured"
            llvm_properly_configured=1
        else
            echo "  Some required packages are missing!"
            echo "    LLVM toolchain is not properly configured"
        fi
        dotenv_llvm_toolchain="/usr/lib/llvm-$llvm_v_major"
    fi
    echo
fi

if [ -n "$missing_pkg_str" ]; then
    printf "\033[1mInstall missing packages using package manager\033[0m\n"
    echo "The following packages will be installed: $missing_pkg_str"
    sudo -v
    fail_count=0
    while read -r missing_pkg; do
        printf "  Attempt to install %s..." "$missing_pkg"
        if sudo apt-get -qq install -y "$missing_pkg" >/dev/null; then
            echo " Success"
        else
            echo " Failure ($?)"
            fail_count=$((fail_count + 1))
        fi
    done < "$missing_pkg_list"
    if [ $fail_count -eq 0 ]; then
        echo "  All missing packages successfully installed!"
    else
        echo "  Failed to install $fail_count packages, toolchain is incomplete!"
        llvm_require_download=1
    fi
    echo
fi
rm "$missing_pkg_list"

if [ $llvm_require_download -eq 1 ]; then
    printf "\033[1mDownloading LLVM %s\033[0m\n" "$llvm_version"
    if [ "$(uname)" = "Linux" ]; then
        if [ "$(uname -m)" = "x86_64" ]; then
            llvm_target="x86_64-linux-gnu-ubuntu-22.04"
        elif [ "$(uname -m)" = "aarch64" ]; then
            llvm_target="aarch64-linux-gnu"
        else
            echo "  FATAL: Unsupported Linux arch: $(uname -m)" >&2
            exit 1
        fi
    elif [ "$(uname)" = "Darwin" ]; then
        cmake_target="arm64-apple-darwin22.0"
    else
        echo "  FATAL: Unsupported host OS: $(uname)" >&2
        exit 1
    fi
    llvm_installation_name="clang+llvm-$llvm_version-$llvm_target"
    llvm_tar_xz_name="$llvm_installation_name.tar.xz"
    llvm_download_link="https://github.com/llvm/llvm-project/releases/download/llvmorg-$llvm_version/$llvm_tar_xz_name"
    echo "  Source: $llvm_download_link"
    echo "  Destination: $buildtools_dir/$llvm_tar_xz_name"
    wget -q $wget_progress_flag -nc -P "$buildtools_dir" "$llvm_download_link"
    echo "  Extracting $buildtools_dir/$llvm_tar_xz_name"
    rm "$buildtools_dir/$llvm_installation_name" 2>/dev/null
    tar -xf "$buildtools_dir/$llvm_tar_xz_name" -C "$buildtools_dir"
    echo "  LLVM $llvm_version is now available at $buildtools_dir/$llvm_installation_name"
    dotenv_llvm_toolchain="$buildtools_dir/$llvm_installation_name"
    echo
fi

{
    echo "#!/usr/bin/env sh"
    echo "# Auto-generated by ./setup-toolchain.sh"
    echo "CMAKE_BIN=$dotenv_cmake_bin"
    echo "LLVM_TOOLCHAIN=$dotenv_llvm_toolchain"
} >> "$dotenv_file"
