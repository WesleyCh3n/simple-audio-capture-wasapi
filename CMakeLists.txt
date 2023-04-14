cmake_minimum_required(VERSION 3.10)

project(audio-capture)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

if(NOT DEFINED BUILD_TEST)
  set(BUILD_TEST TRUE)
endif(NOT DEFINED BUILD_TEST)

if(${BUILD_TEST})
  message("BUILD TEST DIRECTORY: ${CMAKE_SOURCE_DIR}/test/")
  add_subdirectory(${CMAKE_SOURCE_DIR}/test/)
endif(${BUILD_TEST})
