/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _RECTIFY_INSERT_EFFECT_H_
#define _RECTIFY_INSERT_EFFECT_H_

#include "Application/Audio/AudioSample.h"
#include "InsertEffectProcessor.h"
#include <algorithm>
#include <cstdint>
#include <limits>

class RectifyInsertEffect : public InsertEffectProcessor {
public:
  virtual ~RectifyInsertEffect(){};

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    constexpr int32_t kMinPcm = std::numeric_limits<int16_t>::min();
    constexpr int32_t kMaxPcm = std::numeric_limits<int16_t>::max();
    const int clampedAmount = std::clamp(amount, 0, 0xFF);
    const int count = samplecount * GetAudioBufferLayoutChannelCount(layout);

    for (int i = 0; i < count; ++i) {
      const int32_t pcm = std::clamp(
          static_cast<int32_t>(buffer[i] * kAudioPcmScale), kMinPcm, kMaxPcm);
      const int32_t rectified =
          std::min<int32_t>(pcm < 0 ? -pcm : pcm, kMaxPcm);
      const int32_t blended =
          pcm + ((clampedAmount * (rectified - pcm)) + (0xFF / 2)) / 0xFF;
      buffer[i] = static_cast<AudioSample>(blended) / kAudioPcmScale;
    }
  }

  virtual const char *GetShortName() const override { return "RCT"; }
};

#endif
