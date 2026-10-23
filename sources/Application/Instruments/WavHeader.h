/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#ifndef _WAV_HEADER_WRITER_H_
#define _WAV_HEADER_WRITER_H_

#include "Externals/etl/include/etl/expected.h"
#include "System/FileSystem/FileSystem.h"
#include "WavFileErrors.h"
#include <cstdint>

struct WavHeaderInfo {
  uint32_t riffChunkSize = 0;
  uint32_t fmtChunkSize = 0;
  uint16_t audioFormat = 0;
  uint16_t numChannels = 0;
  uint32_t sampleRate = 0;
  uint32_t byteRate = 0;
  uint16_t blockAlign = 0;
  uint16_t bitsPerSample = 0;
  uint16_t bytesPerSample = 0;
  uint32_t dataChunkSize = 0;
  uint32_t dataChunkSizeOffset = 0;
  uint32_t dataOffset = 0;
};

class WavHeaderWriter {
public:
  // Write WAV header to I_File
  static bool WriteHeader(I_File *file, uint32_t sampleRate = 44100,
                          uint16_t channels = 2, uint16_t bytesPerSample = 2);

  // Update file size in WAV header for I_File
  static bool UpdateFileSize(I_File *file, uint32_t sampleCount,
                             uint16_t channels = 2,
                             uint16_t bytesPerSample = 2);
  static bool UpdateFileSize(I_File *file, uint32_t sampleCount,
                             const WavHeaderInfo &headerInfo);

  static etl::expected<WavHeaderInfo, WAVEFILE_ERROR> ReadHeader(I_File *file);
};

#endif
