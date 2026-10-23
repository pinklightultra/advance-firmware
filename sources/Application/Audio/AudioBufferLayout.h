/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _AUDIO_BUFFER_LAYOUT_H_
#define _AUDIO_BUFFER_LAYOUT_H_

#include <cstdint>

enum class AudioBufferLayout : uint8_t {
  MonoPacked = 1,
  StereoInterleaved = 2,
};

inline constexpr int
GetAudioBufferLayoutChannelCount(AudioBufferLayout layout) {
  return static_cast<int>(layout);
}

inline constexpr int GetAudioBufferLayoutStride(AudioBufferLayout layout) {
  return GetAudioBufferLayoutChannelCount(layout);
}

#endif
