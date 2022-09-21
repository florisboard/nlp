# External library: "hat-trie/hat-trie"
add_subdirectory(hat-trie)

# External library: "termbox/termbox2"
project(termbox)
set(TERMBOX_LIB_FILE ${FLORIS_LIBRARY_PREFIX}termbox${FLORIS_LIBRARY_SUFFIX})
set(TERMBOX_INTERMEDIATE_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/termbox2/${TERMBOX_LIB_FILE})
set(TERMBOX_INTERMEDIATE_OBJ_PATH ${CMAKE_CURRENT_SOURCE_DIR}/termbox2/termbox.o)
set(TERMBOX_LIB_PATH ${FLORIS_LIBRARY_OUTPUT_DIRECTORY}/${TERMBOX_LIB_FILE})
add_custom_target(build_termbox ALL
    COMMAND make ${TERMBOX_LIB_FILE} && ${CMAKE_COMMAND} -E copy_if_different ${TERMBOX_INTERMEDIATE_LIB_PATH} ${TERMBOX_LIB_PATH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/termbox2
    BYPRODUCTS ${TERMBOX_INTERMEDIATE_LIB_PATH} ${TERMBOX_INTERMEDIATE_OBJ_PATH} ${TERMBOX_LIB_PATH})
add_library(termbox ${FLORIS_LIBRARY_TYPE} IMPORTED GLOBAL)
add_library(termbox::termbox ALIAS termbox)
add_dependencies(termbox build_termbox)
set_target_properties(termbox PROPERTIES IMPORTED_LOCATION ${TERMBOX_LIB_PATH} IMPORTED_NO_SONAME ON)
target_include_directories(termbox INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/termbox2)

# External library: "bshoshany/thread-pool"
project(bshoshany_thread_pool)
add_library(bshoshany_thread_pool INTERFACE)
add_library(bshoshany::thread_pool ALIAS bshoshany_thread_pool)
target_include_directories(bshoshany_thread_pool INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/thread-pool)
target_sources(bshoshany_thread_pool INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/thread-pool/BS_thread_pool.hpp)
