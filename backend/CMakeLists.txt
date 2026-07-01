cmake_minimum_required(VERSION 3.20)

project(ryfmach LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(lib)
add_subdirectory(app)

enable_testing()
add_subdirectory(tests)
