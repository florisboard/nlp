add_library(fl_nlp_tools_prep)
add_library(fl::nlp::tools::prep ALIAS fl_nlp_tools_prep)
target_module_sources(fl_nlp_tools_prep PUBLIC
        prep.cppm
        wiktextract.cppm
        )
target_link_libraries(fl_nlp_tools_prep PUBLIC
        fl::nlp::core
        fl::string
        fl::nlp::tools::common
        argparse::argparse
        )
