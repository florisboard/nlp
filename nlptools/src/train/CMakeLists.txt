add_library(fl_nlp_tools_train)
add_library(fl::nlp::tools::train ALIAS fl_nlp_tools_train)
target_module_sources(fl_nlp_tools_train PUBLIC
        train.cppm
        raw_text.cppm
        )
target_link_icu4c_libraries(fl_nlp_tools_train PUBLIC FlorisICU::uc)
target_link_libraries(fl_nlp_tools_train PUBLIC
        fl::icuext
        fl::nlp::core
        fl::string
        fl::nlp::tools::common;
        argparse::argparse
        fmt::fmt
        )
