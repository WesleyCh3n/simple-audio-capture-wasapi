#include <iostream>

#define _USE_MATH_DEFINES
#include <cmath>

#include "kiss_fftr.h"

int main() {
  const int N = 512;
  kiss_fft_scalar in[N];
  kiss_fft_cpx out[N / 2 + 1];

  auto config = kiss_fftr_alloc(N, false, nullptr, nullptr);

  const float max_time = 0.5;
  const float dT = max_time / float(N);

  int max = 0, min = -INFINITY;
  for (int i = 0; i < N; i++) {
    float t = float(i) * dT;
    in[i] = 2 * (float)sin(40 * (2 * M_PI) *
                           t); // + 0.5 * sin(90 * (2 * M_PI) * t);
    if (in[i] >= max) {
      max = in[i];
    }
    if (in[i] <= min) {
      min = in[i];
    }
  }

  std::cout << "max: " << max << '\n';
  std::cout << "min: " << min << '\n';

  kiss_fftr(config, in, out);

  for (int i = 0; i < (size_t)N / 2; i++) {
    kiss_fft_scalar Hz = (float)(i * (1.0 / dT) / (float)N);
    kiss_fft_scalar amplitude = std::hypot(out[i].r, out[i].i) / ((float)N / 2);
    if (amplitude < 0.001) {
      continue;
    }
    printf("%.2f   %.8f\n", Hz, amplitude);
  }
  return 0;
}
