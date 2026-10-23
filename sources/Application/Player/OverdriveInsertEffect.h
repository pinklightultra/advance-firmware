/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OVERDRIVE_INSERT_EFFECT_H_
#define _OVERDRIVE_INSERT_EFFECT_H_

#include "Application/Audio/AudioSample.h"
#include "InsertEffectProcessor.h"
#include <algorithm>

class OverdriveInsertEffect : public InsertEffectProcessor {
public:
  virtual ~OverdriveInsertEffect(){};

  void Reset() override {
    control_ = 0.0f;
    driveControl_ = 0.0f;
    initialized_ = false;
  }

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    const int clampedAmount = std::clamp(amount, 0, kAmountMax);
    if (clampedAmount == 0) {
      Reset();
      return;
    }

    const int count = samplecount * GetAudioBufferLayoutChannelCount(layout);
    if (count <= 0) {
      return;
    }

    const float targetControl = controlForAmount(clampedAmount);
    if (!initialized_) {
      control_ = targetControl;
      driveControl_ = driveForControl(control_);
      initialized_ = true;
    }

    int offset = 0;
    while (offset < count) {
      const int blockCount = std::min(kControlBlockSamples, count - offset);
      control_ += (targetControl - control_) * kControlSmoothing;
      const float target = driveForControl(control_);
      const float step =
          (target - driveControl_) / static_cast<float>(kControlBlockSamples);
      float drive = driveControl_;

      for (int i = 0; i < blockCount; ++i) {
        buffer[offset + i] = shapeSample(buffer[offset + i], drive);
        drive += step;
      }

      driveControl_ = blockCount == kControlBlockSamples ? target : drive;
      offset += blockCount;
    }
  }

  virtual const char *GetShortName() const override { return "DRV"; }

private:
  static constexpr int kAmountMax = 0xFF;
  static constexpr int kControlStepMax = 127;
  static constexpr int kControlStepCount = 128;
  static constexpr int kControlBlockSamples = 32;
  static constexpr float kControlSmoothing = 0.03f;
  static constexpr float kMinDrive = 0.15f;
  static constexpr float kMaxDrive = 1.0f;
  static constexpr float kDriveRange = kMaxDrive - kMinDrive;
  static constexpr int kStageCount = 3;

  static float controlForAmount(int amount) {
    const int control =
        ((amount * kControlStepMax) + (kAmountMax / 2)) / kAmountMax;
    return static_cast<float>(control) / static_cast<float>(kControlStepCount);
  }

  static float driveForControl(float control) {
    return kMinDrive + (kDriveRange * control * control);
  }

  static AudioSample shapeSample(AudioSample sample, float drive) {
    const AudioSample driven = sample * drive;
    const bool negative = driven < 0.0f;
    AudioSample shaped = negative ? -driven : driven;
    shaped = std::min(shaped, kAudioSampleMax);

    for (int i = 0; i < kStageCount; ++i) {
      shaped = (2.0f * shaped) - (shaped * shaped);
    }

    shaped = std::min(shaped, kAudioSampleMax);
    return negative ? -shaped : shaped;
  }

  float control_ = 0.0f;
  float driveControl_ = 0.0f;
  bool initialized_ = false;
};

#endif
