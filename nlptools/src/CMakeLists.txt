# fl::nlp::tools
add_subdirectory(prep_wiktextract)

add_executable(fl_nlp_tools)
add_executable(fl::nlp::tools ALIAS fl_nlp_tools)
target_sources(fl_nlp_tools PUBLIC FILE_SET tools_cxx_modules TYPE CXX_MODULES FILES tools/core_ui.cppm)
target_sources(fl_nlp_tools PUBLIC main.cpp)
target_link_icu4c_libraries(fl_nlp_tools PUBLIC FlorisICU::uc)
target_link_libraries(fl_nlp_tools PUBLIC fl::nlp::core)
target_link_libraries(fl_nlp_tools PUBLIC fl::nlp::icuext)
target_link_libraries(fl_nlp_tools PUBLIC fl::nlp::string)
target_link_libraries(fl_nlp_tools PUBLIC fl::nlp::tools::prep_wiktextract)
target_link_libraries(fl_nlp_tools PUBLIC bshoshany::thread_pool nlohmann_json::nlohmann_json termbox::termbox)
set_target_properties(fl_nlp_tools PROPERTIES OUTPUT_NAME "nlptools")
