cmake_minimum_required(VERSION 3.22)

project(FlorisNLP VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_subdirectory(external)
add_subdirectory(core)
add_subdirectory(corefrontend)
add_subdirectory(preprocessing)
