# fl::nlp::string
add_library(fl_nlp_string)
add_library(fl::nlp::string ALIAS fl_nlp_string)
target_module_sources(fl_nlp_string PUBLIC string.cppm)
target_link_icu4c_libraries(fl_nlp_string PUBLIC FlorisICU::uc FlorisICU::data)

# fl::nlp::utils
add_library(fl_nlp_utils)
add_library(fl::nlp::utils ALIAS fl_nlp_utils)
target_module_sources(fl_nlp_utils PUBLIC utils.cppm)

# fl::nlp::core
add_subdirectory(common)
add_subdirectory(latin)
add_library(fl_nlp_core)
add_library(fl::nlp::core ALIAS fl_nlp_core)
target_module_sources(fl_nlp_core PUBLIC core.cppm)
target_link_icu4c_libraries(fl_nlp_core PUBLIC FlorisICU::uc FlorisICU::data)
target_link_libraries(fl_nlp_core PUBLIC fl::nlp::core::common fl::nlp::core::latin)
target_link_libraries(fl_nlp_core PUBLIC nlohmann_json::nlohmann_json)
