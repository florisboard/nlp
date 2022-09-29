project(floris_nlpcorefrontend)

add_executable(floris_nlpcorefrontend main.cpp)
add_executable(floris::nlpcorefrontend ALIAS floris_nlpcorefrontend)

target_link_icu4c_libraries(floris_nlpcorefrontend PUBLIC floris::nlpcore)
target_link_libraries(
    floris_nlpcorefrontend
    PUBLIC

    tsl::hat_trie
    termbox::termbox)
