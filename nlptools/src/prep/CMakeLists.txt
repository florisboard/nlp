add_library(fl_nlp_tools_prep_wiktextract)
add_library(fl::nlp::tools::prep_wiktextract ALIAS fl_nlp_tools_prep_wiktextract)
target_module_sources(fl_nlp_tools_prep_wiktextract PUBLIC
        prep.cppm
        wiktextract.cppm
        )
target_link_libraries(fl_nlp_tools_prep_wiktextract PUBLIC
        fl::nlp::core
        fl::nlp::string
        fl::nlp::tools::common
        argparse::argparse
        )
