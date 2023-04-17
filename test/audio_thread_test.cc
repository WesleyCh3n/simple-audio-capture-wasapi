#include "audio_thread.h"
#include <iostream>
#include <thread>

#define LOGV(x)                                                                \
  {                                                                            \
    for (auto &e : x) {                                                        \
      std::cout << e << ' ';                                                   \
    }                                                                          \
    std::cout << '\n';                                                         \
  }

int main(int argc, char *argv[]) {
  std::cout << "Start main...\n";

  AudioThread w(100);
  std::vector<float> data;
  w.Start();

  auto db_len = w.GetDecibelLen();
  float *vec = new float[db_len];
  /* std::cout << "Press enter to quit...\n\n";
  getchar(); */
  for (int i = 0; i < 10; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    w.GetDecibel(vec);
    for (int j = 0; j < db_len; j++) {
      std::cout << vec[j] << ' ';
    }
    std::cout << '\n';
  }

  w.Stop();
  std::cout << "Exit main...\n";
  return 0;
}

// std::this_thread::sleep_for(std::chrono::seconds(2));
/* for (int i = 0; i < 50; i++) {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  v = w.GetFrequency();
  LOGV(v)
}

w.Pause();
std::this_thread::sleep_for(std::chrono::seconds(2));
w.Resume(); */
