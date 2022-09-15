# External library: "hat-trie/hat-trie"
add_subdirectory(hat-trie)

# External library: "bshoshany/thread-pool"
project(termbox_termbox2)
add_library(termbox_termbox2 INTERFACE)
add_library(termbox::termbox2 ALIAS termbox_termbox2)
target_include_directories(termbox_termbox2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/termbox2")
target_sources(termbox_termbox2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/termbox2/termbox.h")

# External library: "bshoshany/thread-pool"
project(bshoshany_thread_pool)
add_library(bshoshany_thread_pool INTERFACE)
add_library(bshoshany::thread_pool ALIAS bshoshany_thread_pool)
target_include_directories(bshoshany_thread_pool INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/thread-pool")
target_sources(bshoshany_thread_pool INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/thread-pool/BS_thread_pool.hpp")
