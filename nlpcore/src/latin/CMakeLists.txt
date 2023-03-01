add_library(fl_nlp_core_latin)
add_library(fl::nlp::core::latin ALIAS fl_nlp_core_latin)
target_module_sources(fl_nlp_core_latin PUBLIC
        latin.cppm
        dictionary.cppm
        nlp_session.cppm
        prediction_weights.cppm
        )
target_link_libraries(fl_nlp_core_latin PUBLIC fl::nlp::core::common fl::icuext fl::nlp::string)
target_link_icu4c_libraries(fl_nlp_core_latin PUBLIC FlorisICU::uc FlorisICU::data)
