project(floris_nlppreprocessing)

add_executable(floris_nlppreprocessing main.cpp google/Ngram.cpp)
add_executable(floris::nlppreprocessing ALIAS floris_nlppreprocessing)

target_link_libraries(floris_nlppreprocessing PUBLIC tsl::hat_trie)
target_link_libraries(floris_nlppreprocessing PUBLIC bshoshany::thread_pool)
target_link_libraries(floris_nlppreprocessing PUBLIC floris::nlpcore)
target_include_directories(floris_nlppreprocessing PUBLIC google stdext)
