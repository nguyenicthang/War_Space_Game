cmake_minimum_required(VERSION 3.5)
project(safeSave)


add_library(safeSave)
target_include_directories(safeSave PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_sources(safeSave PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/safeSave.cpp")
