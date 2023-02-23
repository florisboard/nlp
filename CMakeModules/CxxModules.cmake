# UUID from here (adjust version tag in URL if you use another version of CMake):
# https://github.com/Kitware/CMake/blob/v3.26.0-rc3/Help/dev/experimental.rst
set(CMake_TEST_CXXModules_UUID "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API ${CMake_TEST_CXXModules_UUID})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    include(ClangModules)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Unsupported compiler: GCC")
endif()

function(target_module_sources target_name target_type mod_src1)
    target_sources(${target_name} ${target_type} FILE_SET cxx_modules TYPE CXX_MODULES FILES ${mod_src1} ${ARGN})
endfunction()
