//----------------------------------------------------------------------------------------------
// MFWaveWriter.h
//----------------------------------------------------------------------------------------------
#ifndef WAVEWRITER_H
#define WAVEWRITER_H

#define CLOSE_HANDLE_IF(h)                                                     \
  if (h != INVALID_HANDLE_VALUE) {                                             \
    CloseHandle(h);                                                            \
    h = INVALID_HANDLE_VALUE;                                                  \
  }

#define IF_ERROR_RETURN(b)                                                     \
  if (b == FALSE) {                                                            \
    return b;                                                                  \
  }

#include <Windows.h>
#include <assert.h>
#include <fileapi.h>
#include <mmreg.h>

const UINT32 WAVE_HEAD_LEN = 44;
const UINT32 WAVE_HEAD_EXT_LEN = 80;

#define SWAP32(val)                                                            \
  (UINT32)((((UINT32)(val)) & 0x000000FF) << 24 |                              \
           (((UINT32)(val)) & 0x0000FF00) << 8 |                               \
           (((UINT32)(val)) & 0x00FF0000) >> 8 |                               \
           (((UINT32)(val)) & 0xFF000000) >> 24)

#pragma pack(push, 1)

struct RIFFCHUNK {
  UINT32 fcc;
  UINT32 cb;
};

struct RIFFLIST {
  UINT32 fcc;
  UINT32 cb;
  UINT32 fccListType;
};

struct WAVEFORM {
  UINT32 fcc;
  UINT32 cb;
  UINT16 wFormatTag;
  UINT16 nChannels;
  UINT32 nSamplesPerSec;
  UINT32 nAvgBytesPerSec;
  UINT16 nBlockAlign;
  UINT16 wBitsPerSample;
};

struct WAVEFORM_EXT : public WAVEFORM {
  UINT16 cbSize;
  UINT16 wValidBitsPerSample;
  UINT32 dwChannelMask;
  GUID SubFormat;
};

struct FACT {
  UINT32 fcc;
  UINT32 cb;
  UINT32 lenght;
};

#pragma pack(pop)

class WaveWriter {
public:
  WaveWriter() : m_hFile(INVALID_HANDLE_VALUE) {}
  ~WaveWriter() { CLOSE_HANDLE_IF(m_hFile); }

  BOOL Initialize(const CHAR *, const BOOL);
  BOOL WriteWaveData(const BYTE *, const DWORD);
  BOOL FinalizeHeader(WAVEFORMATEX *, const UINT32);

private:
  HANDLE m_hFile;
  bool m_extFormat;

  BOOL SetWaveHeader(const WAVEFORMATEX *, const UINT32, BYTE *);
  BOOL SetWaveHeaderExt(WAVEFORMATEX *, const UINT32, BYTE *);
};

#endif
