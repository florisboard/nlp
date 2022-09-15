project(floris_nlp_corefrontend)

add_executable(floris_nlp_corefrontend main.cpp)
add_executable(floris::nlp_corefrontend ALIAS floris_nlp_corefrontend)

target_link_libraries(floris_nlp_corefrontend PUBLIC tsl::hat_trie)
target_link_libraries(floris_nlp_corefrontend PUBLIC floris::nlp_core)
