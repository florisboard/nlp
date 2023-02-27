# fl::nlp::tools
add_subdirectory(common)
add_subdirectory(core_ui)
add_subdirectory(prep_wiktextract)

add_executable(fl_nlp_tools)
add_executable(fl::nlp::tools ALIAS fl_nlp_tools)
target_sources(fl_nlp_tools PUBLIC main.cpp)
target_link_libraries(fl_nlp_tools PUBLIC
        fl::nlp::tools::core_ui
        fl::nlp::tools::prep_wiktextract
        argparse::argparse
        )
target_compile_definitions(fl_nlp_tools PUBLIC PROGRAM_NAME="nlptools" PROGRAM_VERSION="${CMAKE_PROJECT_VERSION}")
set_target_properties(fl_nlp_tools PROPERTIES OUTPUT_NAME "nlptools")
