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
#include "kiss_fft.h"
#include "kiss_fftr.h"

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
  void AudioInit();
  void FFTInit();
  void Run();
  void ProcessFunction();
  std::optional<std::thread> thread_;
  std::atomic_bool stop_;
  std::atomic_bool pause_;
  std::condition_variable cv_;
  std::mutex mutex_;

  IAudioClient *audio_client_ = nullptr;
  IAudioCaptureClient *capture_client_ = nullptr;
  WAVEFORMATEX *wave_format_ = nullptr;

  uint32_t frame_max_ = 0;
  uint32_t packet_len_ = 0;
  uint32_t frame_num_ = 0;
  uint8_t *raw_data_ = nullptr;

  kiss_fftr_cfg fft_cfg_;
  uint32_t fft_win_len_;
  std::vector<float> fft_input_;
  std::vector<kiss_fft_cpx> fft_output_;
  std::vector<float> amplitude_;
  std::vector<float> db_;
};

#endif
