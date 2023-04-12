#ifndef AUDIO_FFT_HPP
#define AUDIO_FFT_HPP
#include <cassert>
#include <mmdeviceapi.h>
#include <stdint.h>

#include <cmath>

#include "kiss_fft.h"
#include "kiss_fftr.h"

#define DEL_ARR(v)                                                             \
  delete[] v;                                                                  \
  v = nullptr;

class AudioFFT {
public:
  AudioFFT(uint32_t len, WAVEFORMATEX *wf)
      : len_(len), cfg_(nullptr), wave_format_(wf), input_(nullptr),
        output_(nullptr), amplitude_(nullptr), decibel_(nullptr) {
    cfg_ = kiss_fftr_alloc(len_, false, nullptr, nullptr);
    input_ = new float[len_]{0.0f};
    out_len_ = len_ / 2 + 1;
    output_ = new kiss_fft_cpx[out_len_];

    freq_range_ = new float[out_len_]{0.0f};
    amplitude_ = new float[out_len_]{0.0f};
    decibel_ = new float[out_len_]{0.0f};

    assert(sizeof(float) == wf->wBitsPerSample / 8);
    channel_datum_ = new float[wf->nChannels];
  }
  ~AudioFFT() {
    DEL_ARR(input_)
    DEL_ARR(output_)
    DEL_ARR(amplitude_)
    DEL_ARR(decibel_)
    kiss_fft_cleanup();
  }

  uint32_t GetOutputLen() { return this->out_len_; }
  void GetFreqRange(float *arr) {
    for (uint32_t i = 0; i < out_len_; i++) {
      arr[i] = i * wave_format_->nSamplesPerSec / float(len_);
    }
  }
  template <typename T> float *GetDecibel(T *data, uint32_t &frame_len) {
    for (uint32_t i = 0; i < frame_len; i++, w_ptr_++) {
      // TODO: base on channel and type -> different mono function
      T mono_sum = 0;
      for (uint16_t c = 0; c < wave_format_->nChannels; c++) {
        mono_sum += data[(i * wave_format_->nChannels) + c];
      }
      input_[w_ptr_] = mono_sum / wave_format_->nChannels;

      if (w_ptr_ == len_ - 1) {
        // calculate fft
        kiss_fftr(this->cfg_, this->input_, this->output_);
        for (uint32_t i = 0; i < this->out_len_; i++) {
          this->amplitude_[i] =
              std::hypot(output_[i].r, output_[i].i) * 2 / (float)len_;
          this->decibel_[i] = 20 * log10(amplitude_[i] / 1);
        }
        w_ptr_ = 0;
      }
    }
    return this->decibel_;
  }

private:
  template <typename T> T *Test(T *ptr) {}
  uint32_t len_;
  uint32_t out_len_;

  WAVEFORMATEX *wave_format_;
  float *channel_datum_;
  kiss_fftr_cfg cfg_;

  float *input_;
  uint32_t w_ptr_;

  float *freq_range_;
  kiss_fft_cpx *output_;
  float *amplitude_;
  float *decibel_;
};
#endif
