project(floris_nlp_core)

add_library(floris_nlp_core INTERFACE)
add_library(floris::nlp_core ALIAS floris_nlp_core)

target_include_directories(floris_nlp_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_sources(floris_nlp_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/ngram/dictionary.hpp"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/ngram/ngram.hpp"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/ngram/spelling_result.hpp"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/stdext/map.hpp"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/stdext/string.hpp")

target_link_libraries(floris_nlp_core INTERFACE tsl::hat_trie)
