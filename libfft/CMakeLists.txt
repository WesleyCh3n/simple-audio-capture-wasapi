cmake_minimum_required(VERSION 3.10)

project(libfft)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME}
  ./kiss_fft.c
  ./kiss_fftr.c
)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
