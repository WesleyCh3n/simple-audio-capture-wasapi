#ifndef WASAPI_CAP_H
#define WASAPI_CAP_H

#include <iostream>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <stdint.h>
#include <thread>
#include <vector>

// Wasapi
#include <Windows.h>

#include "audio_fft.hpp"
#include "audio_stream.hpp"

#ifdef DEBUG
#include "wave_writer.h"
#endif

#define LOG(x) std::cout << x << '\n';

class AudioThread {
public:
  AudioThread(uint32_t hz_gap);
  ~AudioThread();
  void Start();
  void Pause();
  void Resume();
  void Stop();

  uint16_t GetChannels();
  uint32_t GetAmplitudeLen();
  uint32_t GetRawLen();

  void GetAmplitude(float *dst);
  void GetFreqRange(float *dst);
  void GetRaw(float *dst, uint16_t c);

private:
  void Run();
  void ProcessBuffer(uint8_t *data, uint32_t frame_len);
  std::optional<std::thread> thread_;
  std::atomic_bool stop_;
  std::atomic_bool pause_;
  std::condition_variable cv_;
  std::mutex mutex_;

  AudioStream *audio_stream_;
  AudioFFT *audio_fft_;

#ifdef DEBUG
  // for wav writing purpose
  WaveWriter w_writer_;
  uint32_t total_frame_len_ = 0;
#endif

  float *amplitude_;
  uint32_t amplitude_len_;

  float **raws_;
  uint32_t raw_len_;
  uint32_t raw_ptr_;
};

#endif
