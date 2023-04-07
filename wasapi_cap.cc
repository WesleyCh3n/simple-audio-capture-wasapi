#include "wasapi_cap.h"

#include <stdexcept>

AudioThread::AudioThread() : stop_(false), pause_(false) {}
AudioThread::~AudioThread() { this->Stop(); }

void AudioThread::Initialize() {
  //
}

void AudioThread::Start() {
  if (!this->thread_) {
    LOG("Start thread")
    this->thread_ = std::thread(&AudioThread::Run, this);
    this->stop_ = false;
    this->pause_ = false;
  } else {
    LOG("Already Start the thread. Skip!");
  }
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
    LOG("Stop thread")
    this->pause_ = false;
    this->stop_ = true;
    this->cv_.notify_all();
    this->thread_->join();
    this->thread_ = {};
  }
}

void AudioThread::Run() {
  LOG("Enter Thread: " << std::this_thread::get_id());
  while (!this->stop_) {
    this->ProcessFunction();

    if (this->pause_) {
      // pause this thread
      std::unique_lock<std::mutex> locker(this->mutex_);
      this->cv_.wait(locker);
      locker.unlock();
    }
  }
  LOG("Exit Thread: " << std::this_thread::get_id());
}

void AudioThread::ProcessFunction() {
  LOG("Processing Function")
  static int i = 0;
  {
    std::unique_lock<std::mutex> locker(this->mutex_);
    this->freq_.emplace_back((float)i);
  }
  i++;
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

std::vector<float> AudioThread::GetFrequency() {
  std::unique_lock<std::mutex> locker(this->mutex_);
  return this->freq_;
}
