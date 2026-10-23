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
#include "config/MemorySections.h"

// Voice buffers contain finite audio samples. Function-local fast math lets
// GCC use the Cortex-M7 branchless min-number instruction without changing the
// floating-point rules for the rest of the firmware.
#if defined(ADV) && defined(__GNUC__) && !defined(__clang__)
#define PICOTRACKER_OVERDRIVE_FAST_MATH __attribute__((optimize("fast-math")))
#else
#define PICOTRACKER_OVERDRIVE_FAST_MATH
#endif

class OverdriveInsertEffect : public InsertEffectProcessor {
public:
  virtual ~OverdriveInsertEffect(){};

  void Reset() override {
    control_ = 0.0f;
    driveControl_ = 0.0f;
    initialized_ = false;
  }

  PICOTRACKER_FAST_CODE("overdrive_insert_effect_render")
  PICOTRACKER_OVERDRIVE_FAST_MATH
  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    const int clampedAmount =
        amount < 0 ? 0 : (amount > kAmountMax ? kAmountMax : amount);
    if (clampedAmount == 0) {
      Reset();
      return;
    }

    if (samplecount <= 0) {
      return;
    }

    const float targetControl = controlForAmount(clampedAmount);
    if (!initialized_) {
      control_ = targetControl;
      driveControl_ = driveForControl(control_);
      initialized_ = true;
    }

    int frameOffset = 0;
    while (frameOffset < samplecount) {
      const int remainingFrames = samplecount - frameOffset;
      const int blockFrameCount = remainingFrames < kControlBlockSamples
                                      ? remainingFrames
                                      : kControlBlockSamples;
      control_ += (targetControl - control_) * kControlSmoothing;
      const float target = driveForControl(control_);
      const float step =
          (target - driveControl_) / static_cast<float>(kControlBlockSamples);
      float drive = driveControl_;

      if (layout == AudioBufferLayout::StereoInterleaved) {
        for (int frame = 0; frame < blockFrameCount; ++frame) {
          const int index = (frameOffset + frame) * 2;
          shapeStereoFrame(buffer + index, drive);
          drive += step;
        }
      } else {
        for (int frame = 0; frame < blockFrameCount; ++frame) {
          const int index = frameOffset + frame;
          buffer[index] = shapeSample(buffer[index], drive);
          drive += step;
        }
      }

      driveControl_ = blockFrameCount == kControlBlockSamples ? target : drive;
      frameOffset += blockFrameCount;
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

  PICOTRACKER_OVERDRIVE_FAST_MATH
  static float controlForAmount(int amount) {
    const int control =
        ((amount * kControlStepMax) + (kAmountMax / 2)) / kAmountMax;
    return static_cast<float>(control) / static_cast<float>(kControlStepCount);
  }

  PICOTRACKER_OVERDRIVE_FAST_MATH
  static float driveForControl(float control) {
    return kMinDrive + (kDriveRange * control * control);
  }

  PICOTRACKER_OVERDRIVE_FAST_MATH
  static AudioSample shapeSample(AudioSample sample, float drive) {
    const AudioSample driven = sample * drive;
    const bool negative = driven < 0.0f;
    AudioSample shaped = negative ? -driven : driven;
    shaped = shaped > kAudioSampleMax ? kAudioSampleMax : shaped;

    for (int i = 0; i < kStageCount; ++i) {
      shaped *= 2.0f - shaped;
    }

    shaped = shaped > kAudioSampleMax ? kAudioSampleMax : shaped;
    return negative ? -shaped : shaped;
  }

  PICOTRACKER_OVERDRIVE_FAST_MATH
  static void shapeStereoFrame(AudioSample *frame, float drive) {
    const AudioSample drivenLeft = frame[0] * drive;
    const AudioSample drivenRight = frame[1] * drive;
    const bool negativeLeft = drivenLeft < 0.0f;
    const bool negativeRight = drivenRight < 0.0f;
    AudioSample shapedLeft = negativeLeft ? -drivenLeft : drivenLeft;
    AudioSample shapedRight = negativeRight ? -drivenRight : drivenRight;
    shapedLeft = shapedLeft > kAudioSampleMax ? kAudioSampleMax : shapedLeft;
    shapedRight = shapedRight > kAudioSampleMax ? kAudioSampleMax : shapedRight;

    // Keep the two independent channels adjacent so the Cortex-M7 can fill
    // floating-point pipeline stalls with work from the other channel.
    for (int i = 0; i < kStageCount; ++i) {
      shapedLeft *= 2.0f - shapedLeft;
      shapedRight *= 2.0f - shapedRight;
    }

    shapedLeft = shapedLeft > kAudioSampleMax ? kAudioSampleMax : shapedLeft;
    shapedRight = shapedRight > kAudioSampleMax ? kAudioSampleMax : shapedRight;
    frame[0] = negativeLeft ? -shapedLeft : shapedLeft;
    frame[1] = negativeRight ? -shapedRight : shapedRight;
  }

  float control_ = 0.0f;
  float driveControl_ = 0.0f;
  bool initialized_ = false;
};

#undef PICOTRACKER_OVERDRIVE_FAST_MATH

#endif
