add_library(fl_nlp_core_common)
add_library(fl::nlp::core::common ALIAS fl_nlp_core_common)
target_module_sources(fl_nlp_core_common PUBLIC
        common.cppm
        dictionary.cppm
        key_proximity_checker.cppm
        suggestion_request_flags.cppm
        trie_map.cppm
        )
target_link_libraries(fl_nlp_core_common PUBLIC fl::nlp::string)
target_link_libraries(fl_nlp_core_common PUBLIC nlohmann_json::nlohmann_json)
target_link_icu4c_libraries(fl_nlp_core_common PUBLIC FlorisICU::uc FlorisICU::data)
