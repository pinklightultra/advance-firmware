/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "ReverbOutputEffect.h"

#include "Services/Audio/Audio.h"
#include "Services/Audio/AudioDriver.h"
#include "System/Console/n_assert.h"

#include <cstring>

namespace {

constexpr uint32_t kSampleRateHz = 48000;
constexpr int32_t kDefaultProfilerSampleRateHz = 44100;
constexpr int32_t kMaxTailFrames = 10 * static_cast<int32_t>(kSampleRateHz);
constexpr float kDefaultInputGain = 0.2f;
constexpr float kDefaultTime = 0.35f;
constexpr float kDefaultDiffusion = 0.625f;
constexpr float kDefaultLowPass = 0.7f;

float ClampUnitRange(float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

} // namespace

ReverbOutputEffect::ReverbOutputEffect()
    : inputGain_(kDefaultInputGain), time_(kDefaultTime),
      diffusion_(kDefaultDiffusion), lowPass_(kDefaultLowPass) {
  Reset();
}

void ReverbOutputEffect::SetParameters(float inputGain, float time,
                                       float diffusion, float lowPass) {
  inputGain_ = ClampUnitRange(inputGain);
  time_ = ClampUnitRange(time);
  diffusion_ = ClampUnitRange(diffusion);
  lowPass_ = ClampUnitRange(lowPass);
  implementation_.SetParameters(inputGain_, time_, diffusion_, lowPass_);
}

void ReverbOutputEffect::Reset() {
  implementation_.Reset();
  implementation_.SetParameters(inputGain_, time_, diffusion_, lowPass_);
  tailFramesRemaining_ = 0;
}

bool ReverbOutputEffect::IsTailActive() const {
  return tailFramesRemaining_ > 0;
}

bool ReverbOutputEffect::HasAudibleInput(const AudioSample *input,
                                         int32_t sampleCount) const {
  const int32_t valueCount = sampleCount * 2;
  for (int32_t sampleIndex = 0; sampleIndex < valueCount; ++sampleIndex) {
    if (input[sampleIndex] != kAudioSampleZero) {
      return true;
    }
  }

  return false;
}

void ReverbOutputEffect::Render(const AudioSample *input, AudioSample *output,
                                int samplecount) {
  NAssert(input != nullptr);
  NAssert(output != nullptr);
  NAssert(samplecount >= 0);
  NAssert(samplecount <= MAX_SAMPLE_COUNT);

  const int32_t sampleCount = static_cast<int32_t>(samplecount);
  const bool hasAudibleInput = HasAudibleInput(input, sampleCount);

  if (!hasAudibleInput && tailFramesRemaining_ <= 0) {
    memset(output, 0,
           static_cast<size_t>(sampleCount) * 2 * sizeof(AudioSample));
    return;
  }

  if (hasAudibleInput) {
    tailFramesRemaining_ = kMaxTailFrames;
  } else if (tailFramesRemaining_ > 0) {
    tailFramesRemaining_ -= sampleCount;
  }

  implementation_.Render(input, output, sampleCount);
}
