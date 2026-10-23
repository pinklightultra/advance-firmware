/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _DC_OFFSET_INSERT_EFFECT_H_
#define _DC_OFFSET_INSERT_EFFECT_H_

#include "Application/Audio/AudioSample.h"
#include "InsertEffectProcessor.h"
#include <algorithm>
#include <cstdint>
#include <limits>

class DCOffsetInsertEffect : public InsertEffectProcessor {
public:
  virtual ~DCOffsetInsertEffect(){};

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    constexpr int32_t kMinPcm = std::numeric_limits<int16_t>::min();
    constexpr int32_t kMaxPcm = std::numeric_limits<int16_t>::max();
    constexpr int32_t kMaxOffset = 8192;
    const int clampedAmount = std::clamp(amount, 0, 0xFF);
    const int32_t offset = ((clampedAmount - 0x80) * kMaxOffset) / 0x80;
    const int count = samplecount * GetAudioBufferLayoutChannelCount(layout);

    for (int i = 0; i < count; ++i) {
      const int32_t pcm = std::clamp(
          static_cast<int32_t>(buffer[i] * kAudioPcmScale), kMinPcm, kMaxPcm);
      const int32_t biased = std::clamp(pcm + offset, kMinPcm, kMaxPcm);
      buffer[i] = static_cast<AudioSample>(biased) / kAudioPcmScale;
    }
  }

  virtual const char *GetShortName() const override { return "DCO"; }
};

#endif
