project(floris_nlp_corefrontend)

add_executable(floris_nlp_corefrontend main.cpp)
add_executable(floris::nlp_corefrontend ALIAS floris_nlp_corefrontend)

target_link_libraries(
    floris_nlp_corefrontend
    PRIVATE

    tsl::hat_trie
    termbox::termbox
    floris::icuext
    floris::nlp_core)
