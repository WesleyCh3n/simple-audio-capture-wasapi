#ifndef AUDIO_H
#define AUDIO_H

#include <cassert>
#include <functional> // std::function
#include <iomanip>    // setw
#include <iostream>   //
#include <sstream>    // ostringstream
#include <thread>     // sleep_for

// Wasapi
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <Windows.h>

#define PRETTY_LOG(label, var)                                                 \
  std::cout << std::setw(16) << std::left << label << ": " << var << '\n';

#define CHECK_HR(hr, msg)                                                      \
  if (hr != 0) {                                                               \
    std::ostringstream oss;                                                    \
    oss << msg << "\r\n"                                                       \
        << "Error Code: " << hr << "\r\nAbort!";                               \
    MessageBox(nullptr, oss.str().c_str(), "HRESULT != 0",                     \
               MB_OK | MB_ICONERROR);                                          \
    exit(hr);                                                                  \
  }

inline void PrintWaveFormat(WAVEFORMATEX *wf);

class AudioStream {
#define REFTIMES_PER_SEC 10000000 // 100 nanosecond => 10^-7 second
#define REFTIMES_PER_MILLISEC 10000
  const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
  const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
  const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
  const IID IID_IAudioClient = __uuidof(IAudioClient);

public:
  AudioStream()
      : audio_client_(nullptr), capture_client_(nullptr), wave_format_(nullptr),
        raw_data_(nullptr), frame_max_(0), frame_num_(0), packet_len_(0) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    this->Initialize();
  }
  ~AudioStream() {
    std::cout << "AudioStream dtor called\n";
    this->audio_client_->Stop();
    if (this->audio_client_) {
      this->audio_client_->Release();
    }
    if (this->capture_client_) {
      this->capture_client_->Release();
    }
    CoTaskMemFree(this->wave_format_);
    CoUninitialize();
  }

  void StartService() {
    HRESULT hr;
    hr = this->audio_client_->GetService(IID_IAudioCaptureClient,
                                         (void **)&this->capture_client_);
    CHECK_HR(hr, "AudioClient GetService Failed")
    hr = this->audio_client_->Start();
    CHECK_HR(hr, "AudioClient Start Failed")
    this->sleep_time_ = REFTIMES_PER_SEC * this->frame_max_ /
                        this->wave_format_->nSamplesPerSec /
                        REFTIMES_PER_MILLISEC / 2;
  }
  void StopService() {
    HRESULT hr;
    hr = this->audio_client_->Stop();
    CHECK_HR(hr, "AudioClient Stop Failed")
  }

  using StopFn = std::function<bool()>;
  using CallbackFn =
      std::function<void(uint8_t *, uint32_t)>; // data, frame count
  void GetBuffer(StopFn stop, CallbackFn callback) {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_));
    this->capture_client_->ReleaseBuffer(this->frame_num_);
    this->capture_client_->GetNextPacketSize(&this->packet_len_);
    while (this->packet_len_ != 0 && !stop()) {
      this->capture_client_->GetBuffer(&this->raw_data_, &this->frame_num_,
                                       (DWORD *)&this->buffer_flag, nullptr,
                                       nullptr);
      // AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY
      if (this->buffer_flag != AUDCLNT_BUFFERFLAGS_SILENT) {
        callback(this->raw_data_, this->frame_num_);
      }
      this->capture_client_->ReleaseBuffer(this->frame_num_);
      this->capture_client_->GetNextPacketSize(&this->packet_len_);
    }
  }

  WAVEFORMATEX *GetWaveFormat() { return this->wave_format_; }

private:
  void Initialize() {

    HRESULT hr;
    IMMDeviceEnumerator *device_num = NULL;
    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          IID_IMMDeviceEnumerator, (void **)&device_num);
    CHECK_HR(hr, "CoCreateInstance Failed")
    IMMDevice *device = NULL;
    hr = device_num->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    CHECK_HR(hr, "GetDefaultAudioEndpoint Failed")

    hr = device->Activate(IID_IAudioClient, CLSCTX_ALL, NULL,
                          (void **)&this->audio_client_);
    CHECK_HR(hr, "Device Activate Failed")

    hr = this->audio_client_->GetMixFormat(&this->wave_format_);
    CHECK_HR(hr, "AudioClient GetMixFormat Failed")

    hr = this->audio_client_->Initialize(
        AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
        REFTIMES_PER_SEC, 0, this->wave_format_, NULL);
    CHECK_HR(hr, "AudioClient Initialize Failed")

    hr = this->audio_client_->GetBufferSize(&this->frame_max_);
    CHECK_HR(hr, "AudioClient GetBufferSize Failed")

    PrintWaveFormat(this->wave_format_);
    PRETTY_LOG("Max Frame num", this->frame_max_)

    device_num->Release();
    device->Release();
  }

  IAudioClient *audio_client_;
  IAudioCaptureClient *capture_client_;
  WAVEFORMATEX *wave_format_;
  uint64_t sleep_time_;

  uint32_t buffer_flag;
  uint32_t frame_max_;
  uint32_t packet_len_;
  uint32_t frame_num_;

  uint8_t *raw_data_;
};

inline void PrintWaveFormat(WAVEFORMATEX *wf) {
  PRETTY_LOG("nSamplesPerSec", wf->nSamplesPerSec)
  PRETTY_LOG("nChannels", wf->nChannels)
  PRETTY_LOG("wBitsPerSample", wf->wBitsPerSample) // single channel
  PRETTY_LOG("cbSize", wf->cbSize)
  PRETTY_LOG(
      "nAvgBytesPerSec",
      wf->nAvgBytesPerSec) // = nSamplesPerSec * nChannels * wBitsPerSample
  PRETTY_LOG("nBlockAlign", wf->nBlockAlign) // total channel bytes
  PRETTY_LOG("Format",
             "0x" << std::hex << std::uppercase << wf->wFormatTag << std::dec)
  if (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
    if (((WAVEFORMATEXTENSIBLE *)wf)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
      PRETTY_LOG("SubFormat", "KSDATAFORMAT_SUBTYPE_PCM")
    } else if (((WAVEFORMATEXTENSIBLE *)wf)->SubFormat ==
               KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
      PRETTY_LOG("SubFormat", "KSDATAFORMAT_SUBTYPE_IEEE_FLOAT")
    }
  }
}

#endif
