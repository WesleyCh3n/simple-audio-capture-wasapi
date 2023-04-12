#include "audio_thread.h"

#include <cassert>
#include <iomanip>
#include <stdexcept>

AudioThread::AudioThread() : stop_(false), pause_(false) {
  this->audio_stream_ = new AudioStream();
  this->audio_fft_ =
      new AudioFFT(this->audio_stream_->GetWaveFormat()->nSamplesPerSec / 100,
                   this->audio_stream_->GetWaveFormat());
  w_writer_.Initialize(
      "test_1.wav",
      (audio_stream_->GetWaveFormat()->wFormatTag == WAVE_FORMAT_EXTENSIBLE));
  this->decibel_ = new float[audio_fft_->GetOutputLen()];
}
AudioThread::~AudioThread() { this->Stop(); }

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

    w_writer_.FinalizeHeader(audio_stream_->GetWaveFormat(), total_frame_len_);
    delete this->audio_stream_;
    delete this->audio_fft_;
    this->audio_stream_ = nullptr;
    this->audio_fft_ = nullptr;
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

void AudioThread::ProcessBuffer(uint8_t *data, uint32_t frame_len) {
  // pause this thread
  if (this->pause_) {
    std::unique_lock<std::mutex> locker(this->mutex_);
    this->cv_.wait(locker);
    locker.unlock();
  }

  // TEST: write buffer to wav file
  total_frame_len_ += frame_len;
  w_writer_.WriteWaveData(
      data, frame_len * audio_stream_->GetWaveFormat()->nBlockAlign);
  // TODO: base on type cast to different type
  this->decibel_ = audio_fft_->GetDecibel((float *)data, frame_len);
}

float *AudioThread::GetDecibel(uint32_t *out_len) {
  std::unique_lock<std::mutex> locker(this->mutex_);
  *out_len = this->audio_fft_->GetOutputLen();
  return this->decibel_;
}
