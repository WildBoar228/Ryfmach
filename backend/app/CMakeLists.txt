add_library(
    ryfmach_service
    ryfmach_service.cpp
)

target_link_libraries(ryfmach_service PUBLIC bel_lang_engine)
target_include_directories(ryfmach_service PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(
    ryfmach
    app.cpp
    server.cpp
    log.cpp
)

target_link_libraries(ryfmach PUBLIC
    ryfmach_service
    httplib::httplib
    nlohmann_json::nlohmann_json
    spdlog::spdlog)

target_include_directories(ryfmach PUBLIC ${BEL_LANG_ENGINE_PATH}/include)
