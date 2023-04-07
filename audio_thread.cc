#include "audio_thread.h"

#include <cassert>
#include <iomanip>
#include <stdexcept>

#define REFTIMES_PER_SEC 5000000 // 100 nanosecond => 10^-7 second
#define REFTIMES_PER_MILLISEC 10000

#define SAFE_RELEASE(punk)                                                     \
  if ((punk) != nullptr) {                                                     \
    (punk)->Release();                                                         \
    (punk) = nullptr;                                                          \
  }

#define PRETTY_LOG(label, var)                                                 \
  std::cout << std::setw(16) << std::left << label << ": " << var << '\n';

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

AudioThread::AudioThread(uint32_t fft_win_len)
    : stop_(false), pause_(false), fft_win_len_(fft_win_len) {
  this->AudioInit();
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

void AudioThread::AudioInit() {
  if (this->thread_) {
    LOG("Already Start the thread. Skip!");
    return;
  }
  HRESULT hr;
  IMMDeviceEnumerator *device_num = NULL;
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                        IID_IMMDeviceEnumerator, (void **)&device_num);
  assert(hr == 0);
  IMMDevice *device = NULL;
  hr = device_num->GetDefaultAudioEndpoint(eRender, eConsole, &device);
  assert(hr == 0);

  hr = device->Activate(IID_IAudioClient, CLSCTX_ALL, NULL,
                        (void **)&this->audio_client_);
  assert(hr == 0);

  hr = this->audio_client_->GetMixFormat(&this->wave_format_);
  assert(hr == 0);
  PRETTY_LOG("nSamplesPerSec", this->wave_format_->nSamplesPerSec)
  PRETTY_LOG("nChannels", this->wave_format_->nChannels)
  PRETTY_LOG("wBitsPerSample", this->wave_format_->wBitsPerSample)
  PRETTY_LOG("cbSize", this->wave_format_->cbSize)
  PRETTY_LOG("nAvgBytesPerSec", this->wave_format_->nAvgBytesPerSec)
  PRETTY_LOG("nBlockAlign", this->wave_format_->nBlockAlign)
  PRETTY_LOG("Format", "0x" << std::hex << std::uppercase
                            << this->wave_format_->wFormatTag << std::dec)
  if (this->wave_format_->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
    if (((WAVEFORMATEXTENSIBLE *)this->wave_format_)->SubFormat ==
        KSDATAFORMAT_SUBTYPE_PCM) {
      PRETTY_LOG("SubFormat", "KSDATAFORMAT_SUBTYPE_PCM")
    } else if (((WAVEFORMATEXTENSIBLE *)this->wave_format_)->SubFormat ==
               KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
      PRETTY_LOG("SubFormat", "KSDATAFORMAT_SUBTYPE_IEEE_FLOAT")
    }
  }

  hr = this->audio_client_->Initialize(
      AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, REFTIMES_PER_SEC,
      0, this->wave_format_, NULL);
  assert(hr == 0);

  hr = this->audio_client_->GetBufferSize(&this->frame_max_);
  assert(hr == 0);
  PRETTY_LOG("Max Frame num", this->frame_max_)

  SAFE_RELEASE(device_num)
  SAFE_RELEASE(device)
}

void AudioThread::Start() {
  if (this->thread_) {
    LOG("Already Start the thread. Skip!");
    return;
  }

  HRESULT hr;
  hr = this->audio_client_->GetService(IID_IAudioCaptureClient,
                                       (void **)&this->capture_client_);
  assert(hr == 0);
  hr = this->audio_client_->Start();
  assert(hr == 0);

  LOG("Start thread")
  this->thread_ = std::thread(&AudioThread::Run, this);
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

    this->audio_client_->Stop();
    CoTaskMemFree(this->wave_format_);
    SAFE_RELEASE(this->audio_client_)
    SAFE_RELEASE(this->capture_client_)

    LOG("Thread Stopped")
  }
}

void AudioThread::Run() {
  LOG("--> Enter Thread ID: " << std::this_thread::get_id());
  while (!this->stop_) {
    this->ProcessFunction();
  }
  LOG("--> Exit Thread ID: " << std::this_thread::get_id());
}

void AudioThread::ProcessFunction() {
  std::this_thread::sleep_for(std::chrono::milliseconds(
      REFTIMES_PER_SEC * this->frame_max_ / this->wave_format_->nSamplesPerSec /
      REFTIMES_PER_MILLISEC / 2));
  this->capture_client_->GetNextPacketSize(&this->packet_len_);
  LOG("    --> Get packet len: " << this->packet_len_)
  DWORD flags = 0;
  while (!this->stop_ && this->packet_len_ != 0) {
    if (this->pause_) {
      // pause this thread
      std::unique_lock<std::mutex> locker(this->mutex_);
      this->cv_.wait(locker);
      locker.unlock();
    }

    this->capture_client_->GetBuffer(&this->raw_data_, &this->frame_num_,
                                     &flags, nullptr, nullptr);

    LOG("        --> Get frame num: " << this->frame_num_)

    // fill buffer
    uint32_t ptr = 0;
    for (int i = 0; i < frame_num_; i++, ptr++) {
      float left = 0, right = 0;
      std::memcpy(&left, (this->raw_data_ + i * 8), sizeof(float));
      std::memcpy(&right, (this->raw_data_ + i * 8 + 4), sizeof(float));
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
    /* if (this->fft_win_len_ <= this->frame_num_) {
      for (int i = 0; i < this->fft_win_len_; i++) {
        float left = 0, right = 0;
        std::memcpy(&left, (this->raw_data_ + i * 8), sizeof(float));
        std::memcpy(&right, (this->raw_data_ + i * 8 + 4), sizeof(float));
        this->fft_input_[i] = (left + right) / 2;
      }
    } */

    this->capture_client_->ReleaseBuffer(this->frame_num_);
    this->capture_client_->GetNextPacketSize(&this->packet_len_);
  }
}

std::vector<float> AudioThread::GetFrequency() {
  std::unique_lock<std::mutex> locker(this->mutex_);
  return this->amplitude_;
}
