# nlp

Tools for preprocessing raw word data into dictionary files and n-gram models.

This repository is currently in alpha and will move along with the 0.4.0 FlorisBoard development cycle.

## Building the project

### Requirements

- Host system: UNIX-like (Windows is NOT supported)
- CMake 3.22+
- Ninja 1.10+
- GNU make 3.80+
- Clang (any version with proper C++20 support)
- Git (only to clone and to intialize the submodules)

### Seting up and building locally

```shell
# One-time setup
git clone https://github.com/florisboard/nlp.git
cd nlp
git submodule update --init --recursive

# Building the project
cmake --preset=debug .
cmake --build --preset=debug
```

## External libraries used

- [bshoshany/thread-pool](https://github.com/bshoshany/thread-pool) (MIT) by [Barak Shoshany](https://github.com/bshoshany)
- [termbox/termbox2](https://github.com/termbox/termbox2) (MIT) by [termbox](https://github.com/termbox)
- [Tessil/hat-trie](https://github.com/Tessil/hat-trie) (MIT) by [Thibaut Goetghebuer-Planchon](https://github.com/Tessil)

## License

```
Copyright 2022 Patrick Goldinger

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
