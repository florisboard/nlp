cmake_minimum_required(VERSION 3.26)

project(FlorisNLP VERSION 0.1.0)

include(CxxModules)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force clang to use libc++ instead of libstdc++
add_compile_options(-stdlib=libc++)
link_libraries(c++)

# Uncomment below line if Valgrind needs to be used
#add_compile_options(-gdwarf-4)

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
    set(FLORIS_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
endif ()

# External libraries
add_subdirectory(external)
add_subdirectory(icu4c)

# Internal libraries
add_subdirectory(nlpcore)
add_subdirectory(nlptools)
