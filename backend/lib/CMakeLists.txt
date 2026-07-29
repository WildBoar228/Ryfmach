include(FetchContent)

FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.3
)
FetchContent_MakeAvailable(SQLiteCpp)

set(BEL_LANG_ENGINE_PATH "${CMAKE_CURRENT_LIST_DIR}/bel_lang_engine")
set(BEL_LANG_SRC "${BEL_LANG_ENGINE_PATH}/src")

add_library(
    bel_lang_engine
    ${BEL_LANG_SRC}/language.cpp
    ${BEL_LANG_SRC}/morphemics.cpp
    ${BEL_LANG_SRC}/rhyme_likes.cpp
    ${BEL_LANG_SRC}/rhyme_ranking.cpp
    ${BEL_LANG_SRC}/rhymes.cpp
    ${BEL_LANG_SRC}/slounik.cpp
    ${BEL_LANG_SRC}/sounds.cpp
    ${BEL_LANG_SRC}/transcription.cpp
    ${BEL_LANG_SRC}/utils/hash.cpp
)

target_include_directories(
    bel_lang_engine
    PUBLIC
    bel_lang_engine/include
)

target_link_libraries(
    bel_lang_engine
    PUBLIC
    SQLiteCpp
)

target_compile_options(bel_lang_engine PRIVATE "-O3")
