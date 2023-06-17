set_property(GLOBAL PROPERTY __current_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
set_property(GLOBAL PROPERTY __current_binary_dir ${CMAKE_CURRENT_BINARY_DIR})
function(target_module_sources target_name target_type)
    get_property(current_source_dir GLOBAL PROPERTY __current_source_dir)
    get_property(current_binary_dir GLOBAL PROPERTY __current_binary_dir)
    set(header_dir ${current_binary_dir}/rewritten-headers)

    list(LENGTH ARGV num_args)
    math(EXPR num_args_minus_one "${num_args} - 1")
    foreach(index RANGE 2 ${num_args_minus_one})
        list(GET ARGV ${index} source)
        get_filename_component(source_path ${source} REALPATH)

        # Auto prepend some hackery to all source files
        execute_process(
            COMMAND ${Python_EXECUTABLE} ${current_source_dir}/utils/rewrite_cppm_to_header.py --cppm ${source_path} --output-dir ${header_dir}
            RESULT_VARIABLE header_rewrite_result
            OUTPUT_VARIABLE cpp_path
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if (NOT header_rewrite_result EQUAL 0)
            message(FATAL_ERROR "Failed to rewrite source file ${source_path} with error code '${header_rewrite_result}' and message '${cpp_path}'")
        endif()

        target_include_directories(${target_name} ${target_type} ${header_dir})
        target_sources(${target_name} ${target_type} ${cpp_path})
    endforeach()
endfunction()
