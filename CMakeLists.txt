if (NOT ANDROID)
    cmake_minimum_required(VERSION 3.26)
else()
    cmake_minimum_required(VERSION 3.22)
endif()

project(FlorisNLP VERSION 0.1.0)

if (NOT ANDROID)
    include(CxxModules)
else()
    include(CMakeModules/RewriteCppmToHeader.cmake)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force clang to use libc++ instead of libstdc++
if (NOT ANDROID)
    add_compile_options(-stdlib=libc++)
    link_libraries(c++)
endif()

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
