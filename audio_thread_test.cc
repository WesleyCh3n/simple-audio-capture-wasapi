#include "audio_thread.h"
#include <iostream>

#define LOGV(x)                                                                \
  {                                                                            \
    for (auto &e : x) {                                                        \
      std::cout << e << ' ';                                                   \
    }                                                                          \
    std::cout << '\n';                                                         \
  }

int main(int argc, char *argv[]) {
  std::cout << "Start main...\n";
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

  AudioThread w(240);
  std::vector<float> data;
  w.Start();

  /* while (true) {
    data = w.GetFrequency();
  } */
  std::cout << "Press enter to quit...\n\n";
  getchar();

  w.Stop();
  CoUninitialize();
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