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

add_executable(test_fftr
  ./fftr_test.cc
  ./kiss_fftr.c
  ./kiss_fft.c
)

add_executable(test_audio_thread
  ./audio_thread_test.cc

  ./audio_thread.cc
  ./audio_stream.hpp
  ./audio_fft.hpp

  ./kiss_fftr.c
  ./kiss_fft.c

  ./WaveWriter.cc
)

add_executable(audio_test
  ./audio_stream_test.cc
  ./audio_stream.hpp
  ./WaveWriter.cc
)
