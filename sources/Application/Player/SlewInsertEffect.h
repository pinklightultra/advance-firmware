/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _SLEW_INSERT_EFFECT_H_
#define _SLEW_INSERT_EFFECT_H_

#include "Application/Audio/AudioSample.h"
#include "InsertEffectProcessor.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

class SlewInsertEffect : public InsertEffectProcessor {
public:
  virtual ~SlewInsertEffect(){};

  void Reset() override {
    initialized_ = false;
    prev_ = {};
  }

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    constexpr int32_t kMaxPcm = std::numeric_limits<int16_t>::max();
    const int32_t maxDelta = maxDeltaForAmount(amount);

    const int channelCount = GetAudioBufferLayoutChannelCount(layout);

    for (int i = 0; i < samplecount; ++i) {
      const int index = i * channelCount;
      for (int ch = 0; ch < channelCount; ++ch) {
        const int sampleIndex = index + ch;
        const int32_t input = std::clamp(
            static_cast<int32_t>(buffer[sampleIndex] * kAudioPcmScale),
            -kMaxPcm, kMaxPcm);

        if (!initialized_) {
          prev_[ch] = input;
          buffer[sampleIndex] =
              static_cast<AudioSample>(input) / kAudioPcmScale;
          continue;
        }

        const int32_t delta =
            std::clamp(input - prev_[ch], -maxDelta, maxDelta);
        prev_[ch] += delta;
        buffer[sampleIndex] =
            static_cast<AudioSample>(prev_[ch]) / kAudioPcmScale;
      }
      initialized_ = true;
    }
  }

  virtual const char *GetShortName() const override { return "SLW"; }

private:
  static int32_t maxDeltaForAmount(int amount) {
    constexpr int32_t kMaxPcm = std::numeric_limits<int16_t>::max();
    constexpr int32_t kPcmSpan = 1 << 16;
    constexpr int32_t kMinRampFrames = 2;
    constexpr int32_t kMaxRampFrames = 512;
    constexpr int32_t kAmountMax = 0xFF;
    constexpr int32_t kAmountMaxCubed = kAmountMax * kAmountMax * kAmountMax;
    const int32_t clampedAmount = std::clamp<int32_t>(amount, 0, kAmountMax);
    const int32_t amountCubed = clampedAmount * clampedAmount * clampedAmount;
    const int32_t rampFrames =
        kMinRampFrames +
        static_cast<int32_t>(((static_cast<int64_t>(amountCubed) *
                               (kMaxRampFrames - kMinRampFrames)) +
                              (kAmountMaxCubed / 2)) /
                             kAmountMaxCubed);
    return std::clamp<int32_t>(kPcmSpan / rampFrames, 1, kMaxPcm);
  }

  bool initialized_ = false;
  std::array<int32_t, 2> prev_{};
};

#endif
