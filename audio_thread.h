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
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <Windows.h>

// FFT
#include "audio_stream.hpp"
#include "kiss_fft.h"
#include "kiss_fftr.h"

#include "WaveWriter.h"

#define LOG(x) std::cout << x << '\n';

class AudioThread {
public:
  AudioThread(uint32_t fft_win_len);
  ~AudioThread();
  void Start();
  void Pause();
  void Resume();
  void Stop();

  std::vector<float> GetFrequency();

private:
  void FFTInit();
  void Run();
  void ProcessBuffer(uint8_t *data, uint32_t frame_len);
  std::optional<std::thread> thread_;
  std::atomic_bool stop_;
  std::atomic_bool pause_;
  std::condition_variable cv_;
  std::mutex mutex_;

  AudioStream *audio_stream_;

  // TEST: for wav writing purpose
  WaveWriter w_writer_;
  uint32_t total_frame_len_ = 0;

  kiss_fftr_cfg fft_cfg_;
  uint32_t fft_win_len_;
  std::vector<float> fft_input_;
  std::vector<kiss_fft_cpx> fft_output_;
  std::vector<float> amplitude_;
  std::vector<float> db_;
};

#endif
