project(floris_nlptools)

add_executable(nlptools)
add_executable(floris::nlptools ALIAS nlptools)

target_link_icu4c_libraries(nlptools PUBLIC floris::nlpcore)
target_link_libraries(nlptools PUBLIC bshoshany::thread_pool nlohmann_json::nlohmann_json termbox::termbox)
target_include_directories(nlptools PUBLIC "include")
target_sources(nlptools PRIVATE "main.cpp" "src/tools/core_ui.cpp" "src/tools/prep_wiktextract.cpp")
