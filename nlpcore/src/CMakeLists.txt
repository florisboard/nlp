# fl::string
add_library(fl_string)
add_library(fl::string ALIAS fl_string)
target_module_sources(fl_string PUBLIC string.cppm)
target_link_icu4c_libraries(fl_string PUBLIC FlorisICU::uc FlorisICU::data)

# fl::utils
add_library(fl_utils)
add_library(fl::utils ALIAS fl_utils)
target_module_sources(fl_utils PUBLIC utils.cppm)

# fl::nlp::core
add_subdirectory(common)
add_subdirectory(latin)
add_library(fl_nlp_core)
add_library(fl::nlp::core ALIAS fl_nlp_core)
target_module_sources(fl_nlp_core PUBLIC core.cppm)
target_link_icu4c_libraries(fl_nlp_core PUBLIC FlorisICU::uc FlorisICU::data)
target_link_libraries(fl_nlp_core PUBLIC fl::nlp::core::common fl::nlp::core::latin)
target_link_libraries(fl_nlp_core PUBLIC nlohmann_json::nlohmann_json)
