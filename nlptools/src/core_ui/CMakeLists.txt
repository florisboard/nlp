add_library(fl_nlp_tools_core_ui)
add_library(fl::nlp::tools::core_ui ALIAS fl_nlp_tools_core_ui)
target_module_sources(fl_nlp_tools_core_ui PUBLIC
        core_ui.cppm
        )
target_link_icu4c_libraries(fl_nlp_tools_core_ui PUBLIC FlorisICU::uc)
target_link_libraries(fl_nlp_tools_core_ui PUBLIC
        fl::icuext
        fl::nlp::core
        fl::string
        fl::nlp::tools::common
        argparse::argparse
        fmt::fmt
        termbox::termbox
        )
