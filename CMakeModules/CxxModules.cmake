set(CMake_TEST_CXXModules_UUID ${CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    include(ClangModules)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Unsupported compiler: GCC")
endif()

function(target_module_sources target_name target_type mod_src1)
    target_sources(${target_name} ${target_type} FILE_SET cxx_modules TYPE CXX_MODULES FILES ${mod_src1} ${ARGN})
endfunction()