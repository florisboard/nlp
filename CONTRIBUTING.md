# Contributing

First off, thanks for considering contributing to the NLP-subproject of FlorisBoard!

There are several ways to contribute to this subproject. This document provides some general guidelines and tries to answer most common questions. Should you still have questions don't hesistate to ask in the corresponding bug report/feature request issue or by opening a new one!

## Code contribution (NLP core, NLP tools and icuext)

You are encouraged to contribute code, either new code or improving exitsing implementations as long as they follow these basic guildelines:

- Each file must start with the appropiate license header, see literally any `.cppm` file for an example
- Each C++ source file in `nlpcore`/`nlptools` must
  1. Be written in C++20 standard
  2. Use a module syntax (no header/implementation fragmentation!)
  3. Have `.cppm` as the extension (except `main.cpp` for standalone binaries)

You are encouraged to install an EditorConfig extension in the IDE you use so everything regarding intendation etc. is correctly set up.

If you need to add external open-source libraries please first check if the license is compatible with this project (Apache 2.0) and that the library does not add too much to the binary size.

### NLP core

The NLP core is divided into submodules, where each submodule represents one prediction system. The exception is `common`, which is inteded to host code that is shared between prediction systems.

If you want to contribute to the Latin prediction system please add your changes within the `nlpcore/src/latin` submodule.

If you intend to implement a prediction system for an entirely different character system than Latin please create a new submodule in `nlpcore/src` and add your NlpProvider and related classes in there. This step ensures each prediction system is separated from each other and other contributors who want to read the code have an easier time navigating the projet.

### NLP tools

Similiarly to NLP core this part is also split into submodules, however it's not really clearly structued atm. Please check back with repository maintainers if you need help with choosing how to structure your new tool.

## Code contribution (utility scripts)

If you want to provide a utility script for preprocessing text corpus (e.g. wordlist preparation), please create a Python script for this in `utils`.

## Feature requests/bug reports

If you have tested the NLP core directly via the desktop NLP tools and have suggestions or want to submit a bug report please open an issue in this repository.

If you use the NLP core via the FlorisBoard APK please open a feature request/bug report in the [main FlorisBoard repo](https://github.com/florisboard/florisboard/issues) instead!

## Common questions

### Can I contribute my own preprocessed dictionary file?

Atm no - this is the very first preview of the new fldic file format and it is not stable. To prevent voiding community work, external contributions in this regard are currently not accepted. Additionally in the future dictionaries will be hosted in the community repo (even official ones), so this directory is only a temporary solution.
