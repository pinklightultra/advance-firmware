/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _BITCRUSH_INSERT_EFFECT_H_
#define _BITCRUSH_INSERT_EFFECT_H_

#include "Application/Audio/AudioSample.h"
#include "InsertEffectProcessor.h"
#include <algorithm>
#include <cstdint>
#include <limits>

class BitcrushInsertEffect : public InsertEffectProcessor {
public:
  virtual ~BitcrushInsertEffect(){};

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    constexpr int kMaxBitReduction = 15;
    const int clampedAmount = std::clamp(amount, 0, 0xFF);
    const int shift =
        (curveAmount(clampedAmount) * kMaxBitReduction + (0xFF / 2)) / 0xFF;
    const int32_t quantizationStep = 1 << shift;
    const int count = samplecount * GetAudioBufferLayoutChannelCount(layout);

    for (int i = 0; i < count; ++i) {
      buffer[i] = quantize(buffer[i], quantizationStep);
    }
  }

  virtual const char *GetShortName() const override { return "CRS"; }

private:
  static int curveAmount(int amount) {
    const int inverse = 0xFF - amount;
    return 0xFF - ((inverse * inverse + (0xFF / 2)) / 0xFF);
  }

  static int32_t roundToPcm(AudioSample sample) {
    constexpr int32_t kMinPcm = std::numeric_limits<int16_t>::min();
    constexpr int32_t kMaxPcm = std::numeric_limits<int16_t>::max();
    const AudioSample scaled = sample * kAudioPcmScale;
    const int32_t rounded =
        static_cast<int32_t>(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
    return std::clamp(rounded, kMinPcm, kMaxPcm);
  }

  static int32_t quantizePcm(int32_t pcm, int32_t quantizationStep) {
    constexpr int32_t kMinPcm = std::numeric_limits<int16_t>::min();
    constexpr int32_t kMaxPcm = std::numeric_limits<int16_t>::max();
    if (pcm == 0) {
      return 0;
    }
    if (quantizationStep == 1) {
      return pcm;
    }

    const int32_t halfStep = quantizationStep >> 1;
    // Mid-riser quantization avoids a zero dead-zone at high crush amounts.
    const bool negative = pcm < 0;
    const int32_t magnitude = std::min<int32_t>(negative ? -pcm : pcm, kMaxPcm);
    const int32_t quantizedMagnitude =
        ((magnitude / quantizationStep) * quantizationStep) + halfStep;
    const int32_t quantized =
        negative ? -quantizedMagnitude : quantizedMagnitude;
    return std::clamp(quantized, kMinPcm, kMaxPcm);
  }

  static AudioSample quantize(AudioSample sample, int32_t quantizationStep) {
    return static_cast<AudioSample>(
               quantizePcm(roundToPcm(sample), quantizationStep)) /
           kAudioPcmScale;
  }
};

#endif
