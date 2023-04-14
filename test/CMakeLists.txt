cmake_minimum_required(VERSION 3.10)

project(audio-capture)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_SOURCE_DIR}/libfft/ build_libfft)
add_subdirectory(${CMAKE_SOURCE_DIR}/libwav/ build_libwav)
include_directories(${CMAKE_SOURCE_DIR})

add_executable(scratch ./scratch.cc)
target_link_libraries(scratch PUBLIC libfft libwav)
target_include_directories(scratch PUBLIC libfft libwav)

add_executable(fftr_test ./fftr_test.cc)
target_link_libraries(fftr_test PUBLIC libfft)
target_include_directories(fftr_test PUBLIC libfft)

add_executable(audio_stream_test ./audio_stream_test.cc)
target_link_libraries(audio_stream_test PUBLIC libwav)
target_include_directories(audio_stream_test PUBLIC libwav)

add_executable(audio_thread_test
  ./audio_thread_test.cc
  ${CMAKE_SOURCE_DIR}/audio_thread.cc
)
target_link_libraries(audio_thread_test PUBLIC libwav libfft)
target_include_directories(audio_thread_test PUBLIC libwav libfft)
