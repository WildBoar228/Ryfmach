add_executable(
    ryfmach
    app.cpp
)

target_link_libraries(ryfmach PUBLIC bel_lang_engine)
target_include_directories(ryfmach PUBLIC ${BEL_LANG_ENGINE_PATH}/include)
