# fl::nlp::icuext
add_library(fl_icuext)
add_library(fl::icuext ALIAS fl_icuext)
target_module_sources(fl_icuext PUBLIC
        icuext.cppm
        brkiter.cppm
        udata.cppm
        utext.cppm
        )
target_link_libraries(fl_icuext PUBLIC FlorisICU::uc FlorisICU::data)
target_link_libraries(fl_icuext PUBLIC nlohmann_json::nlohmann_json)
