set(BEL_LANG_ENGINE_PATH "${CMAKE_CURRENT_LIST_DIR}/bel_lang_engine")
set(BEL_LANG_SRC "${BEL_LANG_ENGINE_PATH}/src")

add_library(
    bel_lang_engine
    ${BEL_LANG_SRC}/language.cpp
    ${BEL_LANG_SRC}/rhymes.cpp
    ${BEL_LANG_SRC}/sounds.cpp
    ${BEL_LANG_SRC}/transcription.cpp
    ${BEL_LANG_SRC}/utils/hash.cpp
)

target_include_directories(
    bel_lang_engine
    PUBLIC
    bel_lang_engine/include
)

target_compile_definitions(
    bel_lang_engine
    PRIVATE
    RYFMACH_SOUND_COMPATIBILITY_PATH="${BEL_LANG_ENGINE_PATH}/data/sound_compatibility.tsv"
)
