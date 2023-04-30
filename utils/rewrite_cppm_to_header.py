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
import re
import sys
from typing import IO, Tuple


HPP_PREFIX = "#ifndef __{header_name}__\n#define __{header_name}__\n\n"
HPP_SUFFIX = "\n#endif\n"
CPP_PREFIX = '#include "{header_path}"\n\n'
CPP_SUFFIX = "\n"

EXTRACT_MODULE_AND_PARTITION_REGEX = re.compile(r"^(?:export\s*)?module\s*([a-zA-Z0-9_.]+)(?::([a-zA-Z0-9_]+))?;$")

REMOVE_WHOLE_LINE_REGEX = re.compile(r"^(?:(?:export)?\s*module.*?);$")
REWRITE_IMPORT_PARTITION_STATEMENTS = re.compile(r"^(?:export\s*)?import\s*:([a-zA-Z0-9_.]+);$")
REWRITE_IMPORT_MODULE_STATEMENTS = re.compile(r"^(?:export\s*)?import\s*([a-zA-Z0-9_.]+);$")
REMOVE_EXPORT_KEYWORD_REGEX = re.compile(r"^(?:export\s*(.*))$")


CPP_EXTRACT_COMMENTS = r"\/\*(?:.*?\n)*?(?:.*?)\*\/|(?<!https:)\/\/.*"
CPP_EXTRACT_CURLY_CONTENT = r"\s*{\s*((?:.|\n)*)\s*}\s*"
CPP_REMOVE_DEFAULT_ARGUMENT = r"\s+=.*?.(?=\s*,|\s*\))"

CPP_INCLUDE_DIRECTIVE = r"^#\s*include.*"
CPP_PREPROCESSOR_DIRECTIVE = r"^#\s*\w+.*"
CPP_GLOBAL_VARIABLE = r"^const.*=.*;"
CPP_NAMESPACE = r"^(?:inline\s+)?namespace\s+((?:\w|:)+)\s+"
CPP_FUNCTION = r"^(?P<ATTR>\[\[\w+\]\]\s+)?(?P<PREMOD>(?:(?:template\s*<.*?>|const|constexpr|virtual|inline|static|explicit)\s+)*)(?:(?:(?P<NAME>operator\s+\w+(?:<.*?>)?\s*[&*]+\s*)|(?P<RET>[\w:]+(?:<.*?>)?(?:\s*[&*]+\s*|\s+))(?P<NAME_ALT>operator(?:=|==|\(\))|\w+))\s*)(?P<PARAMS>\((?:\s*(?:const\s+)?[\w:]+(?:<.*?>)?\s*[&*]*\s*\w*(?:\s*=.*?)?,?)*\))(?P<POSTMOD>(?:\s*(?:noexcept|const))*(?:\s*=\s*(?:delete|default|0))?)(?P<OVERRIDE>\s+override)?(?P<IMPL>\s*{)?"
CPP_ENUM = r"^enum\s+(?:class\s+)(\w+)\s+{(?:.*\n)*?\s*};"
CPP_CLASS = r"^(?P<TEMPLATE>(?:template\s*<.*?>\s*)?(?:requires\s+.+?\s*)?)(?:class|struct)\s+(?P<NAME>\w+)(?:\s*<.*?>\s*)?(?:\s*:\s*(?:public|private)\s+\w+)?\s+"
CPP_CLASS_VISIBILITY_MOD = r"^(?:public|protected|private):"
CPP_CLASS_VARIABLE = r"^(?:(?:const|mutable)\s+)?(?:\w|[<>:,&*\s])+\s+\w+\s*(?:\n?=(?:.|\n)*?)?;"
CPP_CLASS_CONSTRUCTOR = r"^(?P<PREMOD>(?:virtual|explicit|constexpr)\s+)?(?P<DECL>~?\w+\s*\((?:\s*(?:const\s+)?[\w:]+(?:<.*?>)?(?:\s+\w+\s*|\s*[&*]+\s*\w+\s*|\s*[&*]+\s*)(?:\s*=(?:.|\n)*?)?,?)*\)(?:\s*noexcept)?(?:\s*=\s*(?:delete|default))?)(?P<INITLIST>\s*:(?:\s*\w+\s*\(.*?(?:\(.*?\))?\)\s*?,?)+)?(?P<IMPL>\s*{)?"


def extract_hmp(extract_match: re.Match[str]) -> Tuple[str, str, str]:
    module_name = extract_match.group(1).replace(".", "_")
    partition_name = f"_{extract_match.group(2)}" if extract_match.group(2) else ""
    return (f"{module_name}{partition_name}", module_name, partition_name)


def find_closing_brace_position(text: str, opening_brace_position: int) -> int:
    stack = []
    for i in range(opening_brace_position, len(text)):
        if text[i] == "{":
            stack.append(i)
        elif text[i] == "}":
            if not stack:
                raise ValueError("No opening brace found for closing brace at position " + str(i))
            stack.pop()
            if not stack:
                return i
    raise ValueError("No closing brace found for opening brace at position " + str(opening_brace_position))


def get_sub_source(source: str, begin_read_from: int) -> Tuple[str, int]:
    next_curly_close = find_closing_brace_position(source, begin_read_from)
    len_read = next_curly_close + 1
    sub_source = source[begin_read_from:len_read]
    return (re.match(CPP_EXTRACT_CURLY_CONTENT, sub_source).group(1), len_read)


def rewrite_cppm_to_header(cppm_path: str, output_dir: str, is_debug: bool) -> int:
    if not os.path.isfile(cppm_path):
        print(f"FATAL: Given cppm path '{cppm_path}' is not a file! Aborting.")
        return os.EX_USAGE
    os.makedirs(output_dir, exist_ok=True)

    with open(cppm_path, "r") as cppm_file:
        header_name, module_name, partion_name = (None, None, None)
        for line in cppm_file:
            result = re.match(EXTRACT_MODULE_AND_PARTITION_REGEX, line)
            if result:
                (header_name, module_name, partion_name) = extract_hmp(result)
                break
        assert header_name is not None, "Given cppm file is not a module or module partiton file"
        cppm_file.seek(0)

        tmp_path = os.path.join(output_dir, f"{header_name}.tmp")
        with open(tmp_path, "w") as tmp_file:
            for line in cppm_file:
                if re.match(REMOVE_WHOLE_LINE_REGEX, line):
                    continue

                result = re.match(REWRITE_IMPORT_PARTITION_STATEMENTS, line.strip())
                if result:
                    incl_partiton_name = result.group(1)
                    tmp_file.write(f'#include "{module_name}_{incl_partiton_name}.hpp"\n')
                    continue

                result = re.match(REWRITE_IMPORT_MODULE_STATEMENTS, line)
                if result:
                    incl_module_name = result.group(1).replace(".", "_")
                    tmp_file.write(f'#include "{incl_module_name}.hpp"\n')
                    continue

                result = re.match(REMOVE_EXPORT_KEYWORD_REGEX, line)
                if result:
                    tmp_file.write(result.group(1))
                    tmp_file.write("\n")
                    continue

                tmp_file.write(line)

    intermediate_file_to_hpp_cpp(output_dir, header_name, is_debug)

    return os.EX_OK


def intermediate_file_to_hpp_cpp(output_dir: str, header_name: str, is_debug: bool) -> int:
    tmp_path = os.path.join(output_dir, f"{header_name}.tmp")
    hpp_path = os.path.join(output_dir, f"{header_name}.hpp")
    cpp_path = os.path.join(output_dir, f"{header_name}.cpp")
    print(os.path.abspath(cpp_path))

    with open(tmp_path, "r") as tmp_file, open(hpp_path, "w") as hpp_file, open(cpp_path, "w") as cpp_file:
        hpp_file.write(HPP_PREFIX.format(header_name=header_name.upper()))
        cpp_file.write(CPP_PREFIX.format(header_path=f"{header_name}.hpp"))

        source = re.sub(CPP_EXTRACT_COMMENTS, "", tmp_file.read())
        parse_cppm_to_hpp_cpp(source, "", False, hpp_file, cpp_file, is_debug)

        hpp_file.write(HPP_SUFFIX)
        cpp_file.write(CPP_SUFFIX)

    os.remove(tmp_path)

    return os.EX_OK


def parse_cppm_to_hpp_cpp(
    original_source: str, prefix: str, is_template_ctx: bool, hpp_file: IO, cpp_file: IO, is_debug: bool
) -> None:
    source = original_source
    source_len_read = 0

    while True:
        source = source[source_len_read:].strip()
        source_len_read = 0
        if len(source) <= 0:
            break

        result = re.match(CPP_INCLUDE_DIRECTIVE, source)
        if result:
            if is_debug:
                print(f"[INCLUDE_DIRECTIVE]: {result.group()}")
            hpp_file.write(result.group() + "\n")
            source_len_read = len(result.group())
            continue

        result = re.match(CPP_PREPROCESSOR_DIRECTIVE, source)
        if result:
            if is_debug:
                print(f"[PREPROCESSOR_DIRECTIVE]: {result.group()}")
            hpp_file.write(result.group() + "\n")
            cpp_file.write(result.group() + "\n")
            source_len_read = len(result.group())
            continue

        result = re.match(CPP_GLOBAL_VARIABLE, source)
        if result:
            if is_debug:
                print(f"[GLOBAL_VARIABLE]: {result.group()}")
            hpp_file.write(result.group() + "\n")
            source_len_read = len(result.group())
            continue

        result = re.match(CPP_NAMESPACE, source)
        if result:
            if is_debug:
                print(f"[NAMESPACE]: {result.group()}")
            hpp_file.write(result.group() + "{\n")
            cpp_file.write(result.group() + "{\n")
            [sub_source, len_read] = get_sub_source(source, len(result.group()))
            parse_cppm_to_hpp_cpp(sub_source, prefix, False, hpp_file, cpp_file, is_debug)
            hpp_file.write("}\n")
            cpp_file.write("}\n")
            source_len_read = len_read
            continue

        result = re.match(CPP_ENUM, source)
        if result:
            if is_debug:
                print(f"[ENUM]: {result.group()}")
            hpp_file.write(result.group() + "\n")
            source_len_read = len(result.group())
            continue

        result = re.match(CPP_CLASS, source)
        if result:
            is_class_template = result.group("TEMPLATE").find("template") != -1
            class_name = result.group("NAME")
            if is_debug:
                print(f"[CLASS]: {result.group()}")
            hpp_file.write(result.group() + "{\n")
            [sub_source, len_read] = get_sub_source(source, len(result.group()))
            parse_cppm_to_hpp_cpp(
                sub_source, prefix + f"{class_name}::", is_class_template, hpp_file, cpp_file, is_debug
            )
            hpp_file.write("};\n")
            source_len_read = len_read + 1
            continue

        result = re.match(CPP_CLASS_VISIBILITY_MOD, source)
        if result:
            if is_debug:
                print(f"[CLASS_VISIBILITY]: {result.group()}")
            hpp_file.write(result.group() + "\n")
            source_len_read = len(result.group())
            continue

        result = re.match(CPP_CLASS_CONSTRUCTOR, source)
        if result:
            if is_debug:
                print(f"[CLASS_CONSTRUCTOR]: {result.group()}")
            c_premod = result.group("PREMOD") or ""
            c_premod_cpp = c_premod.replace("explicit", "")
            c_decl = result.group("DECL")
            c_decl_cpp = re.sub(CPP_REMOVE_DEFAULT_ARGUMENT, "", c_decl)
            c_initlist = result.group("INITLIST") or ""
            c_impl = result.group("IMPL")
            hpp_file.write(f"{c_premod}{c_decl};\n")
            if c_impl:
                [sub_source, len_read] = get_sub_source(source, len(result.group()) - 1)
                cpp_file.write(f"{c_premod_cpp}{prefix}{c_decl_cpp}{c_initlist} {{{sub_source}}}\n")
                source_len_read = len_read + 1
            else:
                source_len_read = len(result.group()) + 1
            continue

        result = re.match(CPP_FUNCTION, source)
        if result:
            if is_debug:
                print(f"[FUNCTION]: {result.group()}")
            f_attr = result.group("ATTR") or ""
            f_premod = result.group("PREMOD") or ""
            f_premod_cpp = f_premod.replace("static ", "")
            f_ret_type = result.group("RET") or ""
            f_name = result.group("NAME") or result.group("NAME_ALT")
            f_params = result.group("PARAMS")
            f_params_wo_def_values = re.sub(CPP_REMOVE_DEFAULT_ARGUMENT, "", f_params)
            f_postmod = result.group("POSTMOD") or ""
            f_override = result.group("OVERRIDE") or ""
            f_impl = result.group("IMPL")
            hpp_file.write(f"{f_attr}{f_premod}{f_ret_type}{f_name}{f_params}{f_postmod}{f_override}")
            if f_impl:
                is_inline_or_template = f_premod.find("inline") != -1 or f_premod.find("template") != -1
                if is_template_ctx or is_inline_or_template:
                    [sub_source, len_read] = get_sub_source(source, len(result.group()) - 1)
                    hpp_file.write(" {\n" + sub_source + "}\n")
                    source_len_read = len_read + 1
                else:
                    hpp_file.write(";\n")
                    cpp_file.write(
                        f"{f_attr}{f_premod_cpp}{f_ret_type}{prefix}{f_name}{f_params_wo_def_values}{f_postmod}"
                    )
                    [sub_source, len_read] = get_sub_source(source, len(result.group()) - 1)
                    cpp_file.write(" {\n" + sub_source + "}\n")
                    source_len_read = len_read + 1
            else:
                hpp_file.write(";\n")
                source_len_read = len(result.group()) + 1
            continue

        result = re.match(CPP_CLASS_VARIABLE, source)
        if result:
            if is_debug:
                print(f"[CLASS_VARIABLE]: {result.group()}")
            hpp_file.write(result.group() + "\n")
            source_len_read = len(result.group())
            continue

        raise ValueError(f"Encountered unexpected source code:\n{source[:128]}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Utility which rewrites a cppm file to a header/source file for compilation in Android NDK.",
    )
    parser.add_argument(
        "--cppm",
        type=str,
        required=True,
        help="the original cppm file path to rewrite",
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        required=True,
        help="the output directory in which the header/source file will be written",
    )
    parser.add_argument("--debug", action="store_true", help="enable debug mode (DO NOT USE WITH CMAKE!!)")
    args = parser.parse_args()

    ret_code = rewrite_cppm_to_header(args.cppm, args.output_dir, args.debug)
    if ret_code != os.EX_OK:
        sys.exit(ret_code)


if __name__ == "__main__":
    main()
