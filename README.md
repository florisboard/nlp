# FlorisBoard NLP

This repository is the heart of the NLP functionality of [FlorisBoard](https://github.com/florisboard/florisboard). It
consists mainly of 2 major components:

- [nlpcore](nlpcore): The Core NLP library for FlorisBoard, which is responsible for managing dictionaries and for
  generating word suggestions/performing spell check for given input data. Compiles for both desktop and Android
  targets.
- [nlptools](nlptools): Debug tools for preprocessing raw word data into dictionary files and a debug UI for testing the
  core library without having to compile the full FlorisBoard project. Compiles only for desktop targets.

This repository is currently in alpha and will move along with the 0.4.0 FlorisBoard development cycle.

If you want to contribute to this repository please see [CONTRIBUTING](CONTRIBUTING.md)!

## Building the project

### Requirements

- Host system: UNIX-like
- CMake 3.26+
- Ninja 1.11+
- GNU make 3.80+
    - MUST be GNU make and not some other variation of make or the ICU build will fail!!
- Clang 16.x+ (see below if your distro does not have version 16 yet)
- Package `libc++-dev` and `libc++abi-dev` (version 16.x+)
- Git (only to clone and to initialize the submodules)

Note for Windows 10/11 users: Native Windows compilation is not supported and probably will never be, however you can
fully compile this project using WSL2 (Windows Subsystem for Linux 2). It has been tested with an Ubuntu 22.10 instance
and everything compiled fine. See the [official docs](https://learn.microsoft.com/en-us/windows/wsl/) for more info.

### Set up clang compiler

Before you can build this project you need to set up the clang compiler. First check which version you have installed:

```shell
clang -v
```

<details>
<summary>The reported version is 16.x or newer</summary>

Great! You do not have to set up anything else, and you can skip to the project build section!
</details>

<details>
<summary>The reported version is 15.x or older</summary>

In this case you do not have a supported version of clang installed and we need to build and integrate the compiler
manually.

To properly support C++ modules we need a pre-release version of clang-16 as of March 2023, which can be found here:
https://github.com/llvm/llvm-project/tree/release/16.x

To compile the pre-release version of clang-16, issue the following commands:

```shell
# Get the source code
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout origin/release/16.x

# Set up build directory
mkdir build
cd build

# Build clang (may take a while, depending on the CPU anything from 15-60 mins)
cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -G Ninja ../llvm && ninja
```

Then we need to change the compiler path to the custom compiled one, else the build will fail. To change it, open
[CMakePresets.json](CMakePresets.json) in a text editor and change the C/CXX compiler vars like so:

```
    ...
    "CMAKE_C_COMPILER": "/path/to/custom/compiled/llvm-project/build/bin/clang",
    "CMAKE_CXX_COMPILER": "/path/to/custom/compiled/llvm-project/build/bin/clang++",
    ...
```

</details>

### Initializing the local source repository

```shell
# One-time setup
git clone https://github.com/florisboard/nlp.git
cd nlp
git submodule update --init --recursive
```

### Adjusting the `CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API` UUID

Atm we need to adjust the `CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API` UUID right below the compiler path fields in
[CMakePresets.json](CMakePresets.json), depending on the CMake version you use (UUID committed in this branch is for
CMake 3.26.x). See https://github.com/Kitware/CMake/blob/v3.26.0-rc6/Help/dev/experimental.rst (adjust the version tag
in the URL) for the correct UUID. This step is also temporary and once C++ module support is stable in CMake this will
not be needed to be adjusted anymore.

### Building the project

```shell
# Initialize CMake
cmake --preset=release .

# Build the project
cmake --build --preset=release
```

### Running the NLP tools binary

The NLP tools binary is intended to run on a desktop PC for debugging the core library and for preprocessing various
data sources into dictionary files.

```shell
./build/release/bin/nlptools
```

TODO: documentation

## Known issues

- Memory usage of the NLP core trie map is high:
  - This is indeed a big issue right now, but unlike with the previous NLP attempt we are not restricted by Java's heap space restrictions anymore, only by natively available RAM, so for now we have to bite the bullet (or reduce the entries in the preprocessed dictionaries). If you think you have an idea on how to decrease the memory usage significantly (without overcomplicating the codebase) I am all ears!
- Size of preprocessed dictionaries is quite large:
  - While this is also true for now this poses less of an issue as these preprocessed dictionaries are not included in the APK and thus do not contribute towards the strict max size of the FlorisBoard APK.
- The suggestion ranking is weird for some inputs
  - The weighting system of the suggestions needs a lot of refinement - If you have expertise in this field and want to help I would gladly appreciate it :)
- There's little documentation for parts of this code base
  - I am aware and will work on this bit by bit in the future.
- There aren't schemas available for all json files in this project / schemas.florisboard.org does not work yet
  - I am aware and will work on this in the near future.

## External libraries used

- [bshoshany/thread-pool](https://github.com/bshoshany/thread-pool)
    - Author: [Barak Shoshany](https://github.com/bshoshany)
    - License: MIT
- [nlohmann/json](https://github.com/nlohmann/json)
    - Author: [Niels Lohmann](https://github.com/nlohmann)
    - License: MIT
- [termbox/termbox2](https://github.com/termbox/termbox2)
    - Author: [termbox](https://github.com/termbox)
    - License: MIT
- [unicode-org/icu](https://github.com/unicode-org/icu) (Only ICU4C)
    - Author: [The Unicode Consortium](https://github.com/unicode-org)
    - License: ICU4C License
- [google/googletest](https://github.com/google/googletest) (Testing only)
    - Author: [Google](https://github.com/google)
    - License: BSD-3-Clause
- [p-ranav/argparse](https://github.com/p-ranav/argparse)
    - Author: [Pranav](https://github.com/p-ranav)
    - License: MIT
- [fmtlib/fmt](https://github.com/fmtlib/fmt)
    - Author: [fmtlib](https://github.com/fmtlib)
    - License: MIT

## License

```
Copyright 2022-2023 Patrick Goldinger

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
