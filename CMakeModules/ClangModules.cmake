# Source: https://gitlab.kitware.com/cmake/cmake/-/blob/f1034acb02/.gitlab/ci/cxx_modules_rules_clang.cmake

# UUID from here (adjust version tag in URL if you use another version of CMake):
# https://github.com/Kitware/CMake/blob/v3.26.0-rc3/Help/dev/experimental.rst
set(CMake_TEST_CXXModules_UUID "2182bf5c-ef0d-489a-91da-49dbc3090d2a")

set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
string(CONCAT CMAKE_EXPERIMENTAL_CXX_SCANDEP_SOURCE
        "${CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS}"
        " -format=p1689 --p1689-targeted-file-name=<SOURCE> --p1689-targeted-output=<OBJECT> "
        " --p1689-makeformat-output=<DEP_FILE>"
        " --"
        " <DEFINES> <INCLUDES> <FLAGS> -x c++ <SOURCE>"
        " -MT <DYNDEP_FILE> -MD"
        " > <DYNDEP_FILE>")
set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "clang")
set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FLAG "@<MODULE_MAP_FILE>")

# Default to C++ extensions being off. Clang's modules support have trouble
# with extensions right now.
set(CMAKE_CXX_EXTENSIONS OFF)
