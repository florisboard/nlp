if (NOT ANDROID)
    cmake_minimum_required(VERSION 3.26)
else()
    cmake_minimum_required(VERSION 3.22)
endif()

project(FlorisNLP VERSION 0.1.0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(EnforceSystemEnv)
if (NOT ANDROID)
    include(CxxModules)
else()
    include(RewriteCppmToHeader)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Get llvm toolchain path
get_filename_component(LLVM_TOOLCHAIN ${CMAKE_CXX_COMPILER} DIRECTORY)
get_filename_component(LLVM_TOOLCHAIN ${LLVM_TOOLCHAIN} DIRECTORY)

# We MUST disable json ranges support else this project will not compile properly with clang-16.
# See also: external/nlohmann-json/docs/mkdocs/docs/api/macros/json_has_ranges.md
if (NOT ANDROID)
    set(FLORIS_COMPILE_OPTIONS -stdlib=libc++ -DJSON_HAS_RANGES=0)
    set(FLORIS_LINK_OPTIONS -lc++)
else()
    set(FLORIS_COMPILE_OPTIONS -DJSON_HAS_RANGES=0)
    set(FLORIS_LINK_OPTIONS
        -Wl,--gc-sections,-rpath-link=${LLVM_TOOLCHAIN}/sysroot/usr/lib
        -lc -lstdc++)
endif()

# Alternative approach of not using system headers at all, currently not really working though:
#if (NOT ANDROID)
#    set(FLORIS_COMPILE_OPTIONS
#        -nostdinc++ -nostdlib++
#        -isystem ${LLVM_TOOLCHAIN}/include/c++/v1
#        -DJSON_HAS_RANGES=0)
#    set(FLORIS_LINK_OPTIONS
#        -L ${LLVM_TOOLCHAIN}/lib
#        -Wl,-rpath,${LLVM_TOOLCHAIN}/lib
#        -lc++)
#else()
#    set(FLORIS_COMPILE_OPTIONS
#        -nostdinc++ -nostdlib++
#        -isystem ${LLVM_TOOLCHAIN}/sysroot/usr/include/c++/v1
#        -DJSON_HAS_RANGES=0)
#    set(FLORIS_LINK_OPTIONS
#        -L ${LLVM_TOOLCHAIN}/sysroot/usr/lib
#        -Wl,--gc-sections,-rpath-link=${LLVM_TOOLCHAIN}/sysroot/usr/lib
#        -lc -lstdc++)
#endif()

add_compile_options(${FLORIS_COMPILE_OPTIONS})
add_link_options(${FLORIS_LINK_OPTIONS})

# Uncomment below line if Valgrind needs to be used
# add_compile_options(-gdwarf-4)

# Set up static/shared library vars
if (BUILD_SHARED_LIBS)
    set(FLORIS_LIBRARY_TYPE SHARED)
    set(FLORIS_LIBRARY_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
    set(FLORIS_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(FLORIS_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
else ()
    set(FLORIS_LIBRARY_TYPE STATIC)
    set(FLORIS_LIBRARY_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
    set(FLORIS_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
    if (NOT ANDROID)
        set(FLORIS_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    else()
        set(FLORIS_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    endif()
endif ()

# External libraries
add_subdirectory(external)
add_subdirectory(icu4c)

# Internal libraries
add_subdirectory(icuext)
add_subdirectory(nlpcore)
if (NOT ANDROID)
    add_subdirectory(nlptools)
endif()
