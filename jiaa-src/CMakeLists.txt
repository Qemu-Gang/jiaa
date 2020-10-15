cmake_minimum_required(VERSION 3.0)

project(jiaa C)

include_directories(${CMAKE_SOURCE_DIR}/../memflow-win32-ffi)
include_directories(${CMAKE_SOURCE_DIR}/../memflow-ffi)

add_executable(jiaa)

target_sources(jiaa PRIVATE main.c )
target_sources(jiaa PRIVATE sigscanner.c )
target_sources(jiaa PRIVATE inputsystem.c )
target_sources(jiaa PRIVATE egggame.c)

target_sources(jiaa PRIVATE peeper/client/peeper.c)

target_link_libraries(jiaa PRIVATE m dl pthread rt)
target_link_libraries(jiaa PRIVATE ${CMAKE_SOURCE_DIR}/../target/release/libmemflow_win32_ffi.a)