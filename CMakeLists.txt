cmake_minimum_required(VERSION 3.10)

project(audio-capture)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_executable(render_capture
  ./render_capture.cc
  ./WaveWriter.cc
  ./kiss_fftr.c
  ./kiss_fft.c
)

add_executable(fftr_test
  ./fftr_test.cc
  ./kiss_fftr.c
  ./kiss_fft.c
)
