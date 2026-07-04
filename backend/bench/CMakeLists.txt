include(FetchContent)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

add_executable(
    ryfmach_bench
    rhymes_bench.cpp
)

target_link_libraries(
    ryfmach_bench
    PRIVATE bel_lang_engine
            benchmark::benchmark
            benchmark::benchmark_main
            nlohmann_json::nlohmann_json
)

target_compile_options(ryfmach_bench PRIVATE "-O3")

target_include_directories(ryfmach_bench PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_definitions(ryfmach_bench PRIVATE
    RYFMACH_BENCH_RHYMES_DATASET="${PROJECT_SOURCE_DIR}/data/rhymes.jsonl"
    RYFMACH_BENCH_WORDS_DATASET="${PROJECT_SOURCE_DIR}/data/words.jsonl"
)
