project(floris_icuext)

add_library(floris_icuext)
add_library(floris::icuext ALIAS floris_icuext)

target_include_directories(floris_icuext PUBLIC "include")
target_link_icu4c_libraries(floris_icuext PUBLIC FlorisICU::uc FlorisICU::data FlorisICU::i18n FlorisICU::io)
target_sources(floris_icuext PRIVATE "src/icuext/data.cpp" "src/icuext/string.cpp")
