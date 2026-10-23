/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _DETUNE_INSERT_EFFECT_H_
#define _DETUNE_INSERT_EFFECT_H_

#include "Application/Audio/AudioSample.h"
#include "InsertEffectProcessor.h"
#include <algorithm>
#include <array>
#include <cmath>

class DetuneInsertEffect : public InsertEffectProcessor {
public:
  virtual ~DetuneInsertEffect(){};

  void Reset() override {
    for (auto &frame : delayBuffer_) {
      frame[0] = 0.0f;
      frame[1] = 0.0f;
    }
    writeIndex_ = 0;
    phaseA_ = 0.0f;
    phaseB_ = 0.5f;
  }

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    const int clampedAmount = std::clamp(amount, 0, 0xFF);
    if (clampedAmount <= 0 || layout != AudioBufferLayout::StereoInterleaved) {
      return;
    }

    const float amountNorm = clampedAmount / 255.0f;
    const float wetMix = 0.15f + amountNorm * 0.55f;
    const float dryMix = 1.0f - wetMix * 0.35f;
    const float baseDelayFrames =
        kBaseDelayMinFrames + amountNorm * kBaseDelayAmountFrames;
    const float depthFrames = kDepthMinFrames + amountNorm * kDepthAmountFrames;
    const float phaseStep = 0.0000175f + amountNorm * 0.00005f;

    for (int i = 0; i < samplecount; ++i) {
      const int index = i * 2;
      writeFrame(buffer[index], buffer[index + 1]);

      const float delayA = baseDelayFrames + depthFrames * triangleLfo(phaseA_);
      const float delayB = baseDelayFrames + depthFrames * triangleLfo(phaseB_);
      AudioSample detuneALeft = 0.0f;
      AudioSample detuneARight = 0.0f;
      AudioSample detuneBLeft = 0.0f;
      AudioSample detuneBRight = 0.0f;
      readStereoTap(delayA, detuneALeft, detuneARight);
      readStereoTap(delayB, detuneBLeft, detuneBRight);

      const AudioSample inputLeft = buffer[index];
      const AudioSample inputRight = buffer[index + 1];
      const AudioSample wetLeft =
          static_cast<AudioSample>((detuneALeft + detuneBLeft) * 0.5f);
      const AudioSample wetRight =
          static_cast<AudioSample>((detuneBRight + detuneARight) * 0.5f);

      buffer[index] = std::clamp(dryMix * inputLeft + wetMix * wetLeft,
                                 kAudioSampleMin, kAudioSampleMax);
      buffer[index + 1] = std::clamp(dryMix * inputRight + wetMix * wetRight,
                                     kAudioSampleMin, kAudioSampleMax);

      advancePhase(phaseA_, phaseStep);
      advancePhase(phaseB_, phaseStep * 1.13f);
    }
  }

  virtual const char *GetShortName() const override { return "DTN"; }

private:
  static constexpr int kDelayBufferFrames = 192;
  static constexpr int kBaseDelayMinFrames = 28;
  static constexpr int kBaseDelayAmountFrames = 64;
  static constexpr int kDepthMinFrames = 2;
  static constexpr int kDepthAmountFrames = 52;
  static constexpr int kMaxDelayFrames = kBaseDelayMinFrames +
                                         kBaseDelayAmountFrames +
                                         kDepthMinFrames + kDepthAmountFrames;
  static_assert(kMaxDelayFrames < kDelayBufferFrames - 1,
                "Detune delay range needs the single-wrap tap reader");

  static float triangleLfo(float phase) {
    const float wrapped = phase - static_cast<int>(phase);
    return 1.0f - 4.0f * std::abs(wrapped - 0.5f);
  }

  static void advancePhase(float &phase, float step) {
    phase += step;
    if (phase >= 1.0f) {
      phase -= static_cast<int>(phase);
    }
  }

  void writeFrame(AudioSample left, AudioSample right) {
    delayBuffer_[writeIndex_][0] = left;
    delayBuffer_[writeIndex_][1] = right;
    ++writeIndex_;
    if (writeIndex_ >= kDelayBufferFrames) {
      writeIndex_ = 0;
    }
  }

  void readStereoTap(float delayFrames, AudioSample &left,
                     AudioSample &right) const {
    float readPosition = static_cast<float>(writeIndex_) - delayFrames;
    // The configured delay range is shorter than the delay line, so a single
    // wrap is enough and avoids the cost of generic modulo/clamping per tap.
    if (readPosition < 0.0f) {
      readPosition += static_cast<float>(kDelayBufferFrames);
    }

    const int indexA = static_cast<int>(readPosition);
    int indexB = indexA + 1;
    if (indexB >= kDelayBufferFrames) {
      indexB = 0;
    }
    const float fraction = readPosition - static_cast<float>(indexA);
    const auto &frameA = delayBuffer_[indexA];
    const auto &frameB = delayBuffer_[indexB];
    left = frameA[0] + (frameB[0] - frameA[0]) * fraction;
    right = frameA[1] + (frameB[1] - frameA[1]) * fraction;
  }

  std::array<std::array<AudioSample, 2>, kDelayBufferFrames> delayBuffer_{};
  int writeIndex_ = 0;
  float phaseA_ = 0.0f;
  float phaseB_ = 0.5f;
};

#endif
