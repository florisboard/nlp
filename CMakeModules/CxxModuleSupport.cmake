# CxxModuleSupport
# Author: Patrick Goldinger <patrick@patrickgold.dev>
# ----------------------------------------------------
#
# This CMakeModule adds experimental support for C++ modules when compiling with clang on Linux (and with libc++
# instead of libstdc++). GCC and/or MSVC are NOT supported.
#
# To properly use this module the variable PREBUILT_MODULE_PATH must be set to a valid path.
#
# Note: as of the state of clang/CMake in Jan 2023 module compilation may require 5-10 re-runs of the compilation until
# all modules are compiled, only then the actual executable/library is built. This is definitely sub-optimal but
# something we have to currently live with. This should be fixed once proper module dependency support comes to clang
# (without using a custom-patched version of clang and with enabling super-experimental support for it in CMake).
#
# Roadmap:
# - Investigate if libc++ forcing is really needed, or if we can just fall back to the system headers or whatever the
#   build system provides as the default
# - Replace and/or remove this CMakeModule once CMake and clang have proper dependency scanning and compilation
#   support for C++ modules
#
# This resource was extremely useful in providing documentation on how to use C++ modules as of Jan 2023:
# https://clang.llvm.org/docs/StandardCPlusPlusModules.html
#
# It additionally provides some examples on how to precompile C++ standard lib into modules (to allow for importing
# them as header units), however for now we are already happy that module compilation works as it does.
#
# Additional resources which helped in creating this CMakeModule:
# https://stackoverflow.com/questions/57300495/how-to-use-c20-modules-with-cmake/62499857#62499857
#

function(target_add_module target_name module_name source_file)
    # Check requirements
    if (NOT ${CMAKE_CXX_STANDARD} GREATER_EQUAL 20)
        message(FATAL_ERROR "C++ standard must be >= 20!")
    endif ()

    # Set up module_path
    set(module_path ${PREBUILT_MODULE_PATH}/${target_name})
    file(MAKE_DIRECTORY ${module_path})

    # Set up target properties
    target_compile_options(${target_name} PRIVATE -fprebuilt-module-path=${module_path})
    target_link_libraries(${target_name} PUBLIC -lc++)
    target_sources(${target_name} PUBLIC ${source_file})

    # Add custom target for pcm file
    # TODO: is -Xclang -emit-module-interface still needed?
    add_custom_target(${module_name}.pcm
            COMMAND
            ${CMAKE_CXX_COMPILER}
            -std=c++${CMAKE_CXX_STANDARD}
            -stdlib=libc++
            --precompile ${CMAKE_CURRENT_SOURCE_DIR}/${source_file}
            -Xclang -emit-module-interface
            -o ${module_path}/${module_name}.pcm
            "-I$<JOIN:$<TARGET_PROPERTY:${target_name},INCLUDE_DIRECTORIES>,;-I>"
            "$<TARGET_PROPERTY:${target_name},COMPILE_OPTIONS>"
            "-D$<JOIN:$<TARGET_PROPERTY:${target_name},COMPILE_DEFINITIONS>,;-D>"
            COMMAND_EXPAND_LISTS
            )

    # Require the target_name to depend on it
    add_dependencies(${target_name} ${module_name}.pcm)
endfunction()

function(target_link_modules_library target_name type modules_library)
    # TODO: what if target_name is not an alias?
    get_target_property(original_modules_library ${modules_library} ALIASED_TARGET)
    target_link_libraries(${target_name} ${type} ${original_modules_library})
    target_compile_options(${target_name} PRIVATE
            -stdlib=libc++
            -fprebuilt-module-path=${PREBUILT_MODULE_PATH}/${original_modules_library})
endfunction()
