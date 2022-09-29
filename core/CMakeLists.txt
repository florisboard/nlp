add_library(floris_nlpcore)
add_library(floris::nlpcore ALIAS floris_nlpcore)

target_include_directories(floris_nlpcore PUBLIC "include")
target_link_icu4c_libraries(floris_nlpcore PUBLIC FlorisICU::uc FlorisICU::data)
target_link_libraries(floris_nlpcore PUBLIC tsl::hat_trie)
target_sources(floris_nlpcore PRIVATE
    "src/core/dictionary.cpp"
    "src/core/dictionary_session.cpp"
    "src/core/string.cpp"
    "src/core/udata.cpp")
