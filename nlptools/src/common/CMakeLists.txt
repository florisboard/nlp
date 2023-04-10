add_library(fl_nlp_tools_common)
add_library(fl::nlp::tools::common ALIAS fl_nlp_tools_common)
target_module_sources(fl_nlp_tools_common PUBLIC
        common.cppm
        action_config.cppm
        dictionary_args_utils.cppm
        program.cppm
        )
target_link_libraries(fl_nlp_tools_common PUBLIC
        fl::string
        argparse::argparse
        )
