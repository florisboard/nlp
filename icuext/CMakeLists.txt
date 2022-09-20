project(icuext)

add_library(icuext STATIC)
add_library(floris::icuext ALIAS icuext)

target_include_directories(icuext PUBLIC include)
target_sources(icuext PRIVATE src/data.cpp src/string.cpp)
target_link_libraries(icuext PUBLIC FlorisICU::uc FlorisICU::data)
