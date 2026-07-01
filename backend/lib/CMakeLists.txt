set(BEL_LANG_ENGINE_PATH "${CMAKE_CURRENT_LIST_DIR}/bel_lang_engine")
set(BEL_LANG_SRC "${BEL_LANG_ENGINE_PATH}/src")

add_library(
    bel_lang_engine
    ${BEL_LANG_SRC}/language.cpp
    ${BEL_LANG_SRC}/sounds.cpp
    ${BEL_LANG_SRC}/transcription.cpp
)

target_include_directories(
    bel_lang_engine
    PUBLIC
    bel_lang_engine/include
)
