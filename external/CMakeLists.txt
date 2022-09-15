# External library: "hat-trie/hat-trie"
add_subdirectory(hat-trie)

# External library: "termbox/termbox2"
project(termbox)
add_custom_target(build_termbox ALL
                    COMMAND make libtermbox.a
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/termbox2"
                    BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/termbox2/libtermbox.a"
                    "${CMAKE_CURRENT_SOURCE_DIR}/termbox2/termbox.o")
add_library(termbox STATIC IMPORTED GLOBAL)
add_library(termbox::termbox ALIAS termbox)
add_dependencies(termbox build_termbox)
set_target_properties(termbox PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/termbox2/libtermbox.a")
target_include_directories(termbox INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/termbox2")

# External library: "bshoshany/thread-pool"
project(bshoshany_thread_pool)
add_library(bshoshany_thread_pool INTERFACE)
add_library(bshoshany::thread_pool ALIAS bshoshany_thread_pool)
target_include_directories(bshoshany_thread_pool INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/thread-pool")
target_sources(bshoshany_thread_pool INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/thread-pool/BS_thread_pool.hpp")
