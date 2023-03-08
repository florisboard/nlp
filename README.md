# FlorisBoard NLP

This repository is the heart of the NLP functionality of [FlorisBoard](https://github.com/florisboard/florisboard). It
consists mainly of 2 major components:

- [nlpcore](nlpcore): The Core NLP library for FlorisBoard, which is responsible for managing dictionaries and for
  generating word suggestions/performing spell check for given input data. Compiles for both desktop and Android
  targets.
- [nlptools](nlptools): Debug tools for preprocessing raw word data into dictionary files and a debug UI for testing the
  core library without having to compile the full FlorisBoard project. Compiles only for desktop targets.

This repository is currently in alpha and will move along with the 0.4.0 FlorisBoard development cycle.

## Building the project

### Requirements

- Host system: UNIX-like
- CMake 3.26+
- Ninja 1.11+
- GNU make 3.80+
    - MUST be GNU make and not some other variation of make or the ICU build will fail!!
- Clang 16.x+ (needs to be custom compiled atm, see below)
- Package `libc++-dev` and `libc++abi-dev` (version 15.x+)
- Git (only to clone and to initialize the submodules)

Note for Windows 10/11 users: Native Windows compilation is not supported and probably will never be, however you can
fully compile this project using WSL2 (Windows Subsystem for Linux 2). It has been tested with an Ubuntu 22.10 instance
and everything compiled fine. See the [official docs](https://learn.microsoft.com/en-us/windows/wsl/) for more info.

### (Temporary) Compile pre-release version of clang-16

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

This step is only temporary, once clang-16 is released we can use a pre-compiled clang version and this step is
obsolete.

### Setting up and building locally

```shell
# One-time setup
git clone https://github.com/florisboard/nlp.git
cd nlp
git submodule update --init --recursive

# Building the project (See steps below before first build!!)
cmake --preset=debug .
cmake --build --preset=debug
```

NOTE: atm we need to change the compiler path to the custom compiled one, else the build will fail. To change it, open
[CMakePresets.json](CMakePresets.json) in a text editor and change the C/CXX compiler vars like so:

```
    ...
    "CMAKE_C_COMPILER": "/path/to/custom/compiled/llvm-project/build/bin/clang",
    "CMAKE_CXX_COMPILER": "/path/to/custom/compiled/llvm-project/build/bin/clang++",
    ...
```

Additionally, you might need to adjust the `CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API` UUID right below the compiler path
fields, depending on the CMake version you use (UUID committed in this branch is for CMake 3.26.x). See
https://github.com/Kitware/CMake/blob/v3.26.0-rc4/Help/dev/experimental.rst (adjust the version tag in the URL) for the
correct UUID. This step is also temporary and once C++ module support is stable in CMake this will not be needed to be
adjusted anymore.

### Running the NLP tools binary

The NLP tools binary is intended to run on a desktop PC for debugging the core library and for preprocessing various
data sources into dictionary files.

```shell
./build/debug/bin/nlptools
```

TODO: documentation

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
