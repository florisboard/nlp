add_library(floris_nlpcore)
add_library(floris::nlpcore ALIAS floris_nlpcore)

target_include_directories(floris_nlpcore PUBLIC "include")
target_link_icu4c_libraries(floris_nlpcore PUBLIC floris::icuext)
target_link_libraries(floris_nlpcore PUBLIC tsl::hat_trie)
target_sources(floris_nlpcore PRIVATE
    "src/nlp/spelling_result.cpp")
