# External library: "hat-trie/hat-trie"
add_subdirectory(hat-trie)

# External library: "bshoshany/thread-pool"
project(bshoshany_thread_pool)
add_library(bshoshany_thread_pool INTERFACE)
add_library(bshoshany::thread_pool ALIAS bshoshany_thread_pool)
target_include_directories(bshoshany_thread_pool INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/thread-pool")
target_sources(bshoshany_thread_pool INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/thread-pool/BS_thread_pool.hpp")
