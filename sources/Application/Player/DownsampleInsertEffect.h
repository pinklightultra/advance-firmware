/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _DOWNSAMPLE_INSERT_EFFECT_H_
#define _DOWNSAMPLE_INSERT_EFFECT_H_

#include "Application/Audio/AudioSample.h"
#include "InsertEffectProcessor.h"
#include <algorithm>

class DownsampleInsertEffect : public InsertEffectProcessor {
public:
  virtual ~DownsampleInsertEffect(){};

  static int HoldFramesForAmount(int amount) {
    constexpr int kOctaveStepBits = 5;
    constexpr int kOctaveStep = 1 << kOctaveStepBits;
    constexpr int kMaxOctave = 8;

    const int clampedAmount = std::clamp(amount, 0, 0xFF);
    if (clampedAmount == 0xFF) {
      return 1 << kMaxOctave;
    }

    // Spread the single knob across eight sample-and-hold octaves.
    const int octave = clampedAmount >> kOctaveStepBits;
    const int fraction = clampedAmount & (kOctaveStep - 1);
    const int base = 1 << octave;
    return base + ((base * fraction) + (kOctaveStep / 2)) / kOctaveStep;
  }

  void Reset() override {
    heldLeft_ = 0.0f;
    heldRight_ = 0.0f;
    holdCounter_ = 0;
  }

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    const int clampedAmount = std::clamp(amount, 0, 0xFF);
    if (clampedAmount == 0) {
      Reset();
      return;
    }

    const int holdFrames = HoldFramesForAmount(clampedAmount);
    holdCounter_ = std::min(holdCounter_, holdFrames - 1);
    const int channelCount = GetAudioBufferLayoutChannelCount(layout);

    for (int i = 0; i < samplecount; ++i) {
      const int index = i * channelCount;
      if (holdCounter_ <= 0) {
        heldLeft_ = buffer[index];
        heldRight_ = channelCount > 1 ? buffer[index + 1] : heldLeft_;
        holdCounter_ = holdFrames - 1;
      } else {
        holdCounter_--;
      }

      buffer[index] = heldLeft_;
      if (channelCount > 1) {
        buffer[index + 1] = heldRight_;
      }
    }
  }

  virtual const char *GetShortName() const override { return "DSP"; }

private:
  AudioSample heldLeft_ = 0.0f;
  AudioSample heldRight_ = 0.0f;
  int holdCounter_ = 0;
};

#endif
