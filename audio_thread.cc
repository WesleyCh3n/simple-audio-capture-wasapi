#include "audio_thread.h"

#include <cassert>
#include <iomanip>
#include <stdexcept>

AudioThread::AudioThread(uint32_t fft_win_len)
    : stop_(false), pause_(false), fft_win_len_(fft_win_len) {
  this->audio_stream_ = new AudioStream();
  std::cout << "float" << sizeof(float) << '\n';
  w_writer_.Initialize(
      "test_1.wav",
      (audio_stream_->GetWaveFormat()->wFormatTag == WAVE_FORMAT_EXTENSIBLE));
  this->FFTInit();
}
AudioThread::~AudioThread() { this->Stop(); }

void AudioThread::FFTInit() {
  this->fft_cfg_ = kiss_fftr_alloc(this->fft_win_len_, false, nullptr, nullptr);
  this->fft_input_ = std::vector<float>(this->fft_win_len_, 0.0f);
  this->fft_output_ = std::vector<kiss_fft_cpx>(this->fft_win_len_ / 2 + 1);
  this->amplitude_ = std::vector<float>(this->fft_win_len_ / 2 + 1, 0.0f);
  this->db_ = std::vector<float>(this->fft_win_len_ / 2 + 1, 0.0f);
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

    w_writer_.FinalizeHeader(audio_stream_->GetWaveFormat(), total_frame_len_);
    delete this->audio_stream_;
    this->audio_stream_ = nullptr;
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

  uint32_t ptr = 0;
  for (int i = 0; i < frame_len; i++, ptr++) {
    float left = 0, right = 0;
    std::memcpy(&left, (data + i * 8), sizeof(float));
    std::memcpy(&right, (data + i * 8 + 4), sizeof(float));
    this->fft_input_[ptr] = (left + right) / 2;

    if (ptr == fft_win_len_ - 1) {
      // calculate fft
      kiss_fftr(this->fft_cfg_, this->fft_input_.data(),
                this->fft_output_.data());
      for (int i = 0; i < this->fft_win_len_ / 2; i++) {
        this->amplitude_[i] = std::hypot(fft_output_[i].r, fft_output_[i].i) *
                              2 / (float)fft_win_len_;
        this->db_[i] = 20 * log10(amplitude_[i] / 1);
      }
      ptr = 0;
    }
  }
}

std::vector<float> AudioThread::GetFrequency() {
  std::unique_lock<std::mutex> locker(this->mutex_);
  return this->amplitude_;
}
