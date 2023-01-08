# fl::nlp::string
add_library(fl_nlp_string)
add_library(fl::nlp::string ALIAS fl_nlp_string)
target_add_module(fl_nlp_string "fl.nlp.string" string.cppm)
target_link_icu4c_libraries(fl_nlp_string PUBLIC FlorisICU::uc FlorisICU::data)

# fl::nlp::core
add_library(fl_nlp_core)
add_library(fl::nlp::core ALIAS fl_nlp_core)
target_add_module(fl_nlp_core "fl.nlp.core" core.cppm)
target_add_module(fl_nlp_core "fl.nlp.core.common" core/common.cppm)
target_add_module(fl_nlp_core "fl.nlp.core.dictionary" core/dictionary.cppm)
target_add_module(fl_nlp_core "fl.nlp.core.key_proximity_map" core/key_proximity_map.cppm)
target_add_module(fl_nlp_core "fl.nlp.core.nlp_session" core/nlp_session.cppm)
target_add_module(fl_nlp_core "fl.nlp.core.trie_map" core/trie_map.cppm)
target_link_icu4c_libraries(fl_nlp_core PUBLIC FlorisICU::uc FlorisICU::data)
target_link_modules_library(fl_nlp_core PUBLIC fl::nlp::string)
target_link_libraries(fl_nlp_core PUBLIC nlohmann_json::nlohmann_json)

# fl::nlp::icuext
add_library(fl_nlp_icuext icuext.cppm)
add_library(fl::nlp::icuext ALIAS fl_nlp_icuext)
target_add_module(fl_nlp_icuext "fl.nlp.icuext" icuext.cppm)
target_link_icu4c_libraries(fl_nlp_icuext PUBLIC FlorisICU::uc FlorisICU::data)
