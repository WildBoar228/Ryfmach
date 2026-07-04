cmake_minimum_required(VERSION 3.20)

project(ryfmach LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(lib)
add_subdirectory(app)

enable_testing()
add_subdirectory(tests)

add_subdirectory(bench)
