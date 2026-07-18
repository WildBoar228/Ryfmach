include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    FIND_PACKAGE_ARGS NAMES GTest
)

enable_testing()
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(
    ryfmach_tests
    bel_lang_engine/core_ut.cpp
    bel_lang_engine/morphemic_ut.cpp
    bel_lang_engine/phonetic_ut.cpp
    bel_lang_engine/rhyme_ut.cpp
    bel_lang_engine/slounik_ut.cpp
    ryfmach_service_ut.cpp
)

target_link_libraries(
    ryfmach_tests
    GTest::gtest_main
    GTest::gmock_main
    bel_lang_engine
    ryfmach_service
)

target_compile_options(ryfmach_tests PRIVATE "-fsanitize=address,leak,undefined" "-O3")
target_link_options(ryfmach_tests PRIVATE "-fsanitize=address,leak,undefined")

target_include_directories(ryfmach_tests PUBLIC ${PROJECT_SOURCE_DIR}/include)

include(GoogleTest)
gtest_discover_tests(ryfmach_tests)
