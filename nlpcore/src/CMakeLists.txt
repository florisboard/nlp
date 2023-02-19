# fl::nlp::string
add_library(fl_nlp_string)
add_library(fl::nlp::string ALIAS fl_nlp_string)
target_sources(fl_nlp_string PUBLIC FILE_SET string_cxx_modules TYPE CXX_MODULES FILES string.cppm)
target_link_icu4c_libraries(fl_nlp_string PUBLIC FlorisICU::uc FlorisICU::data)

# fl::nlp::core
add_library(fl_nlp_core)
add_library(fl::nlp::core ALIAS fl_nlp_core)
target_sources(fl_nlp_core PUBLIC FILE_SET core_cxx_modules TYPE CXX_MODULES FILES
        core.cppm
        core/common.cppm
        core/dictionary.cppm
        core/key_proximity_map.cppm
        core/nlp_session.cppm
        core/trie_map.cppm
        )
target_link_icu4c_libraries(fl_nlp_core PUBLIC FlorisICU::uc FlorisICU::data)
target_link_libraries(fl_nlp_core PUBLIC fl::nlp::string)
target_link_libraries(fl_nlp_core PUBLIC nlohmann_json::nlohmann_json)

# fl::nlp::icuext
add_library(fl_nlp_icuext icuext.cppm)
add_library(fl::nlp::icuext ALIAS fl_nlp_icuext)
target_sources(fl_nlp_icuext PUBLIC FILE_SET icuext_cxx_modules TYPE CXX_MODULES FILES icuext.cppm)
target_link_icu4c_libraries(fl_nlp_icuext PUBLIC FlorisICU::uc FlorisICU::data)
