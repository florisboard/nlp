if (NOT ANDROID)
    cmake_minimum_required(VERSION 3.26)
else()
    cmake_minimum_required(VERSION 3.22)
endif()

project(FlorisNLP VERSION 0.1.0)

if (NOT ANDROID)
    include(CxxModules)
else()
    set_property(GLOBAL PROPERTY __current_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
    set_property(GLOBAL PROPERTY __current_binary_dir ${CMAKE_CURRENT_BINARY_DIR})
    function(target_module_sources target_name target_type)
        get_property(current_source_dir GLOBAL PROPERTY __current_source_dir)
        get_property(current_binary_dir GLOBAL PROPERTY __current_binary_dir)
        list(LENGTH ARGV num_args)
        math(EXPR num_args_minus_one "${num_args} - 1")
        foreach(index RANGE 2 ${num_args_minus_one})
            list(GET ARGV ${index} source)
            get_filename_component(source_path ${source} REALPATH)
            get_filename_component(include_dir ${source_path} DIRECTORY)

            # Auto prepend some hackery to all source files
            execute_process(
                COMMAND python3 ${current_source_dir}/utils/rewrite_cppm_to_header.py --cppm ${source_path} --header-dir ${current_binary_dir}
                RESULT_VARIABLE header_rewrite_result
                OUTPUT_VARIABLE header_path
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if (NOT header_rewrite_result EQUAL 0)
                message(FATAL_ERROR "Failed to rewrite source file ${source_path} with error code '${header_rewrite_result}' and message '${header_path}'")
            endif()

            get_filename_component(module_name "${header_path}" NAME_WE)
            set(dummy_source_path "${current_binary_dir}/${module_name}.cpp")
            file(WRITE "${dummy_source_path}" "#include \"${header_path}\"\n")
            #set_source_files_properties("${header_path}" PROPERTIES GENERATED TRUE)
            target_include_directories(${target_name} ${target_type} ${current_binary_dir})
            target_sources(${target_name} ${target_type} ${dummy_source_path})
        endforeach()
    endfunction()
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
