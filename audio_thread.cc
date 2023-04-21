#include "audio_thread.h"

#include <cassert>
#include <iomanip>
#include <stdexcept>

AudioThread::AudioThread(uint32_t hz_gap) : stop_(false), pause_(false) {
  this->audio_stream_ = new AudioStream();
  uint32_t fft_win =
      this->audio_stream_->GetWaveFormat()->nSamplesPerSec / hz_gap;
  this->audio_fft_ = new AudioFFT(fft_win % 2 == 0 ? fft_win : fft_win - 1,
                                  this->audio_stream_->GetWaveFormat());
  this->amplitude_len_ = audio_fft_->GetOutputLen();
  this->amplitude_ = new float[amplitude_len_];
  for (int i = 0; i < amplitude_len_; i++) {
    this->amplitude_[i] = -120.0;
  }

  auto channels = this->audio_stream_->GetWaveFormat()->nChannels;

  this->raws_ = new float *[channels];
  for (int c = 0; c < channels; c++) {
    this->raws_[c] = new float[fft_win];
    for (int i = 0; i < fft_win; i++) {
      this->raws_[c][i] = 0.0;
    }
  }
  this->raw_len_ = fft_win; // This could be anything else
  this->raw_ptr_ = 0;

#ifdef DEBUG
  w_writer_.Initialize(
      "test_1.wav",
      (audio_stream_->GetWaveFormat()->wFormatTag == WAVE_FORMAT_EXTENSIBLE));
#endif
}
AudioThread::~AudioThread() {
  this->Stop();
  delete[] this->amplitude_;
  for (int c = 0; c < this->audio_stream_->GetWaveFormat()->nChannels; c++) {
    delete[] this->raws_[c];
  }
  delete[] this->raws_;

  delete this->audio_stream_;
  delete this->audio_fft_;
  this->audio_stream_ = nullptr;
  this->audio_fft_ = nullptr;
}

void AudioThread::Start() {
  if (this->thread_) {
    LOG("Already Start the thread. Skip!");
    return;
  }
  this->audio_stream_->StartService();

  LOG("Start thread")
  this->thread_ = std::thread(std::bind(&AudioThread::Run, this));
  this->stop_ = false;
  this->pause_ = false;
}

void AudioThread::Pause() {
  if (this->thread_) {
    LOG("Pause thread")
    this->pause_ = true;
  }
}

void AudioThread::Resume() {
  if (this->thread_) {
    LOG("Resume thread")
    this->pause_ = false;
    this->cv_.notify_all();
  }
}

void AudioThread::Stop() {
  if (this->thread_ && !this->stop_) {
    this->pause_ = false;
    this->stop_ = true;
    this->cv_.notify_all();

    this->thread_->join();
    this->thread_ = {};

#ifdef DEBUG
    w_writer_.FinalizeHeader(audio_stream_->GetWaveFormat(), total_frame_len_);
#endif
    this->audio_stream_->StopService();
    LOG("Thread Stopped")
  }
}

void AudioThread::Run() {
  LOG("--> Enter Thread ID: " << std::this_thread::get_id());
  while (!this->stop_) {
    this->audio_stream_->GetBuffer([&] { return this->stop_.load(); },
                                   [&](uint8_t *data, uint32_t frame_len) {
                                     this->ProcessBuffer(data, frame_len);
                                   });
  }
  LOG("--> Exit Thread ID: " << std::this_thread::get_id());
}

void AudioThread::ProcessBuffer(uint8_t *raw_data, uint32_t frame_len) {
  // pause this thread
  if (this->pause_) {
    std::unique_lock<std::mutex> locker(this->mutex_);
    this->cv_.wait(locker);
    locker.unlock();
  }

#ifdef DEBUG
  // write buffer to wav file
  total_frame_len_ += frame_len;
  w_writer_.WriteWaveData(
      data, frame_len * audio_stream_->GetWaveFormat()->nBlockAlign);
#endif

  // Get raw data to each channels
  float *tmp_raw_data = (float *)raw_data;
  for (uint32_t i = 0; i < frame_len; i++, raw_ptr_++) {
    float mono_sum = 0;
    for (uint16_t c = 0; c < audio_stream_->GetWaveFormat()->nChannels; c++) {
      raws_[c][raw_ptr_] =
          tmp_raw_data[(i * audio_stream_->GetWaveFormat()->nChannels) + c];
    }
    if (raw_ptr_ == raw_len_ - 1) {
      raw_ptr_ = 0;
    }
  }

  // TODO: base on type cast to different type
  audio_fft_->GetAmplitude((float *)raw_data, frame_len, this->amplitude_);
}

uint32_t AudioThread::GetAmplitudeLen() { return this->amplitude_len_; }

void AudioThread::GetAmplitude(float *dst) {
  // std::unique_lock<std::mutex> locker(this->mutex_);
  std::copy(this->amplitude_, this->amplitude_ + this->amplitude_len_, dst);
}

void AudioThread::GetFreqRange(float *dst) { audio_fft_->GetFreqRange(dst); }

uint16_t AudioThread::GetChannels() {
  return this->audio_stream_->GetWaveFormat()->nChannels;
}

uint32_t AudioThread::GetRawLen() { return this->raw_len_; }

void AudioThread::GetRaw(float *dst, uint16_t c) {
  std::copy(this->raws_[c], this->raws_[c] + this->raw_len_, dst);
}
