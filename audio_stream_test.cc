#include "audio_stream.hpp"

#include "WaveWriter.h"

int main(int argc, char *argv[]) {
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  assert(hr == 0);

  WaveWriter waveWriter;
  AudioStream *audio = new AudioStream();
  waveWriter.Initialize("test.wav", (audio->GetWaveFormat()->wFormatTag ==
                                     WAVE_FORMAT_EXTENSIBLE));

  audio->StartService();
  int counter = 0;
  uint8_t *data = nullptr;
  uint32_t total_frame_len = 0;
  while (counter < 500) {
    audio->GetBuffer([&]() { return counter < 500; },
                     [&](uint8_t *data, uint32_t frame_len) {
                       waveWriter.WriteWaveData(
                           data,
                           frame_len * audio->GetWaveFormat()->nBlockAlign);
                       total_frame_len += frame_len;
                     });
    counter++;
  }
  waveWriter.FinalizeHeader(audio->GetWaveFormat(), total_frame_len);
  delete audio;
  CoUninitialize();
  return 0;
}
