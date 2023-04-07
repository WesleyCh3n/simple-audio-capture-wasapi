#ifndef WASAPI_CAP_H
#define WASAPI_CAP_H

#include <iostream>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#define LOG(x) std::cout << x << '\n';

class AudioThread {
public:
  AudioThread();
  ~AudioThread();
  void Start();
  void Pause();
  void Resume();
  void Stop();

  void Initialize();
  std::vector<float> GetFrequency();

private:
  void Run();
  void ProcessFunction();
  std::optional<std::thread> thread_;
  std::atomic_bool stop_;
  std::atomic_bool pause_;
  std::condition_variable cv_;
  std::mutex mutex_;
  std::vector<float> freq_;
};

#endif
