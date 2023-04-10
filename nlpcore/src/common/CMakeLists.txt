add_library(fl_nlp_core_common)
add_library(fl::nlp::core::common ALIAS fl_nlp_core_common)
target_module_sources(fl_nlp_core_common PUBLIC
        common.cppm
        dictionary.cppm
        key_proximity_checker.cppm
        spelling.cppm
        suggestion.cppm
        syllable_matcher.cppm
        trie_map.cppm
        )
target_link_libraries(fl_nlp_core_common PUBLIC fl::nlp::string fl::nlp::utils)
target_link_libraries(fl_nlp_core_common PUBLIC fmt::fmt nlohmann_json::nlohmann_json)
target_link_icu4c_libraries(fl_nlp_core_common PUBLIC FlorisICU::uc FlorisICU::data)
