# fl::nlp::tools
add_subdirectory(core_ui)
add_subdirectory(prep_wiktextract)

add_executable(fl_nlp_tools)
add_executable(fl::nlp::tools ALIAS fl_nlp_tools)
target_sources(fl_nlp_tools PUBLIC main.cpp)
target_link_libraries(fl_nlp_tools PUBLIC fl::nlp::tools::core_ui fl::nlp::tools::prep_wiktextract)
set_target_properties(fl_nlp_tools PROPERTIES OUTPUT_NAME "nlptools")
