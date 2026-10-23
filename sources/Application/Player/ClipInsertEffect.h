/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _CLIP_INSERT_EFFECT_H_
#define _CLIP_INSERT_EFFECT_H_

#include "Application/Audio/AudioSample.h"
#include "InsertEffectProcessor.h"
#include <algorithm>
#include <cstdint>
#include <limits>

class ClipInsertEffect : public InsertEffectProcessor {
public:
  virtual ~ClipInsertEffect(){};

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    constexpr int32_t kMaxPcm = std::numeric_limits<int16_t>::max();
    constexpr int32_t kMinThreshold = 2048;
    const int clampedAmount = std::clamp(amount, 0, 0xFF);
    const int32_t threshold =
        kMaxPcm -
        ((clampedAmount * (kMaxPcm - kMinThreshold)) + (0xFF / 2)) / 0xFF;

    const int count = samplecount * GetAudioBufferLayoutChannelCount(layout);
    for (int i = 0; i < count; ++i) {
      const int32_t pcm = std::clamp(
          static_cast<int32_t>(buffer[i] * kAudioPcmScale), -kMaxPcm, kMaxPcm);
      const int32_t clipped = std::clamp(pcm, -threshold, threshold);
      buffer[i] = static_cast<AudioSample>(clipped) / kAudioPcmScale;
    }
  }

  virtual const char *GetShortName() const override { return "CLP"; }
};

#endif
