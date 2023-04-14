//-----------------------------------------------------------
// Record an audio stream from the default audio render
// device. The RecordAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data from the
// render device. The main loop runs every 1/2 second.
//-----------------------------------------------------------

#define COBJMACROS
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <Windows.h>

#include <comdef.h>
#include <iomanip>

#define _USE_MATH_DEFINES
#include <cmath>

#include <fstream>
#include <iostream>
#include <vector>

#include "kiss_fft.h"
#include "kiss_fftr.h"
#include "wave_writer.h"

#define REFTIMES_PER_SEC 5000000 // 100 nanosecond => 10^-7 second
#define REFTIMES_PER_MILLISEC 10000

#define FFTWINSIZE 480

#define SAFE_RELEASE(punk)                                                     \
  if ((punk) != NULL) {                                                        \
    (punk)->Release();                                                         \
    (punk) = NULL;                                                             \
  }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

// https://en.wikipedia.org/wiki/Hann_function
void hann_window(float *data, float *out, size_t length) {
  for (int i = 0; i < length; i++) {
    double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (length - 1)));
    out[i] = multiplier * data[i];
  }
}

HRESULT RecordAudioStream(const char *file) {
  HRESULT hr;
  IMMDeviceEnumerator *pEnumerator = NULL;
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                        IID_IMMDeviceEnumerator, (void **)&pEnumerator);
  assert(hr == 0);

  // get default device handler
  IMMDevice *pDevice = NULL;
  hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
  assert(hr == 0);

  // Create audio buffer
  IAudioClient *pAudioClient = NULL;
  hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL,
                         (void **)&pAudioClient);
  assert(hr == 0);

  // Get audio format for WASAPI shared mode
  WAVEFORMATEX *pWf = NULL;
  hr = pAudioClient->GetMixFormat(&pWf);
  assert(hr == 0);
  std::cout << "nSamplesPerSec: " << pWf->nSamplesPerSec << "\n"
            << "Channels: " << pWf->nChannels << "\n"
            << "wBitsPerSample: " << pWf->wBitsPerSample << "\n"
            << "cbSize: " << pWf->cbSize << "\n" // size of the extension
            << "nAvgBytesPerSec: " << pWf->nAvgBytesPerSec << "\n"
            << "nBlockAlign: " << pWf->nBlockAlign << "\n"
            << "Format: 0x" << std::hex << std::uppercase << pWf->wFormatTag
            << std::dec << '\n';

  if (pWf->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
    if (((WAVEFORMATEXTENSIBLE *)pWf)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
      std::cout << "SubFormat: KSDATAFORMAT_SUBTYPE_PCM\n";
    } else if (((WAVEFORMATEXTENSIBLE *)pWf)->SubFormat ==
               KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
      std::cout << "SubFormat: KSDATAFORMAT_SUBTYPE_IEEE_FLOAT\n";
    }
  }

  // Set buffer parameters
  REFERENCE_TIME dur = REFTIMES_PER_SEC;
  hr =
      pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                               AUDCLNT_STREAMFLAGS_LOOPBACK, dur, 0, pWf, NULL);
  assert(hr == 0);

  // Clean up
  SAFE_RELEASE(pEnumerator)
  SAFE_RELEASE(pDevice)

  UINT32 bufferFrameCount;
  UINT32 numFramesAvailable;
  hr = pAudioClient->GetBufferSize(&bufferFrameCount);
  std::cout << "bufferFrameCount: " << bufferFrameCount << '\n';
  assert(hr == 0);

  IAudioCaptureClient *pCaptureClient = NULL;
  hr = pAudioClient->GetService(IID_IAudioCaptureClient,
                                (void **)&pCaptureClient);
  assert(hr == 0);

  // Calculate the actual duration of the allocated buffer.
  REFERENCE_TIME hnsActualDuration;
  hnsActualDuration = REFTIMES_PER_SEC * bufferFrameCount / pWf->nSamplesPerSec;
  std::cout << "hnsActualDuration: " << hnsActualDuration << '\n';

  hr = pAudioClient->Start();
  assert(hr == 0);

  WaveWriter waveWriter;
  waveWriter.Initialize(file, (pWf->wFormatTag == WAVE_FORMAT_EXTENSIBLE));
  UINT32 uiFileLength = 0;

  uint32_t frame = 0;
  char loading[4] = {'/', '|', '\\', '-'};
  UINT32 packetLength = 0;
  DWORD flags = 0;
  BYTE *pData;

  std::vector<float> left_raw_win(FFTWINSIZE, 0.0f);
  std::vector<float> left_hann_win(FFTWINSIZE, 0.0f);
  kiss_fftr_cfg cfg = kiss_fftr_alloc(FFTWINSIZE, false, nullptr, nullptr);
  kiss_fft_cpx out[FFTWINSIZE / 2 + 1];
  std::vector<float> bins(FFTWINSIZE / 2 + 1);
  std::vector<float> dbs(FFTWINSIZE / 2 + 1);

  std::ofstream outFile("left_channel.csv");
  std::ofstream binFile("bin.csv");
  std::ofstream dbFile("db.csv");

  for (int i = 0; i < (FFTWINSIZE / 2); i++) {
    binFile << i * pWf->nSamplesPerSec / float(FFTWINSIZE) << ',';
  }
  binFile << '\n';

  assert(sizeof(float) == pWf->wBitsPerSample / 8);
  while (TRUE) {
    // Sleep for half the buffer duration.
    Sleep((DWORD)hnsActualDuration / REFTIMES_PER_MILLISEC / 2);
    pCaptureClient->GetNextPacketSize(&packetLength);

    while (packetLength != 0) {
      pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL,
                                NULL);
      for (int i = 0; i < FFTWINSIZE; i++) {
        std::memcpy(&left_raw_win[i], (pData + i * 4), sizeof(float));
        outFile << left_raw_win[i] << '\n';
      }
      // hann_window(left_raw_win.data(), left_hann_win.data(), FFTWINSIZE);

      kiss_fftr(cfg, left_raw_win.data(), out);
      for (int i = 0; i < FFTWINSIZE / 2; i++) {
        kiss_fft_scalar amp =
            std::hypot(out[i].r, out[i].i) * 2 / (float)FFTWINSIZE;
        bins[i] = amp;
        dbs[i] = 20 * log10(amp / 1);

        binFile << bins[i] << ',';
        dbFile << dbs[i] << ',';
      }

      binFile << '\n';
      dbFile << '\n';

      waveWriter.WriteWaveData(pData, numFramesAvailable * pWf->nBlockAlign);
      uiFileLength += numFramesAvailable;
      pCaptureClient->ReleaseBuffer(numFramesAvailable);
      pCaptureClient->GetNextPacketSize(&packetLength);
    }

    std::cout << loading[int(frame / 10) % 4] << '\r' << std::flush;
    frame++;
    if (frame == 1000) {
      break;
    }
  }

  kiss_fftr_free(cfg);
  outFile.close();
  hr = pAudioClient->Stop(); // Stop recording.
  assert(hr == 0);
  waveWriter.FinalizeHeader(pWf, uiFileLength);

  CoTaskMemFree(pWf);
  SAFE_RELEASE(pAudioClient)
  SAFE_RELEASE(pCaptureClient)

  return hr;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("No output file.\n");
    return -1;
  }
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  assert(hr == 0);
  RecordAudioStream(argv[1]);
  CoUninitialize();
  return 0;
}
