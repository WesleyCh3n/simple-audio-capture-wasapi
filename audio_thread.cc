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
  this->decibel_len_ = audio_fft_->GetOutputLen();
  this->decibel_ = new float[decibel_len_];
  for (int i = 0; i < decibel_len_; i++) {
    this->decibel_[i] = -120.0;
  }

#ifdef DEBUG
  w_writer_.Initialize(
      "test_1.wav",
      (audio_stream_->GetWaveFormat()->wFormatTag == WAVE_FORMAT_EXTENSIBLE));
#endif
}
AudioThread::~AudioThread() {
  this->Stop();
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

  // TODO: base on type cast to different type
  audio_fft_->GetDecibel((float *)raw_data, frame_len, this->decibel_);
}

uint32_t AudioThread::GetDecibelLen() { return this->decibel_len_; }

void AudioThread::GetDecibel(float *dst) {
  // std::unique_lock<std::mutex> locker(this->mutex_);
  std::copy(this->decibel_, this->decibel_ + this->decibel_len_, dst);
}

void AudioThread::GetFreqRange(float *dst) { audio_fft_->GetFreqRange(dst); }
