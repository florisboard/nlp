cmake_minimum_required(VERSION 3.22)

project(FlorisNLP VERSION 0.1.0)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# External libraries
add_subdirectory(external)
add_subdirectory(icu4c)

# Internal libraries
add_subdirectory(core)
add_subdirectory(corefrontend)
add_subdirectory(preprocessing)
