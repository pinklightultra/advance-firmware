/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OUTPUT_EFFECT_TEST_SUPPORT_H_
#define _OUTPUT_EFFECT_TEST_SUPPORT_H_

#include "doctest/doctest.h"

#include "Application/Audio/AudioSample.h"
#include "Application/Effects/OutputEffect.h"

namespace OutputEffectTestSupport {

constexpr int kTestSampleCount = 256;

template <typename Effect> struct EffectTestMemory {
  AudioSample input[kTestSampleCount * 2];
  AudioSample output[kTestSampleCount * 2];
};

template <typename Effect> EffectTestMemory<Effect> &GetEffectTestMemory() {
  static EffectTestMemory<Effect> memory;
  return memory;
}

template <typename Effect> Effect &GetTestEffect() {
  static Effect effect;
  return effect;
}

template <typename Effect> void InitializeEffect(Effect &effect) {
  effect.Reset();
  effect.PrepareRender();
}

template <typename Effect> void CheckSilentInputProducesSilence() {
  EffectTestMemory<Effect> &memory = GetEffectTestMemory<Effect>();
  Effect &effect = GetTestEffect<Effect>();
  InitializeEffect(effect);
  for (int sample = 0; sample < 8; ++sample) {
    memory.input[sample] = 0.0f;
    memory.output[sample] = 1.0f;
  }

  effect.Render(memory.input, memory.output, 4);

  for (int sample = 0; sample < 8; ++sample) {
    CHECK(memory.output[sample] == doctest::Approx(0.0f));
  }
}

template <typename Effect> void CheckDeclaresTailLength() {
  Effect &effect = GetTestEffect<Effect>();
  InitializeEffect(effect);
  const int32_t tailFrameCount = effect.GetTailFrameCount();
  CHECK(tailFrameCount > 0);

  OutputEffect &baseEffect = effect;
  baseEffect.Reset();
  CHECK(effect.GetTailFrameCount() == tailFrameCount);
}

template <typename Effect> void CheckOnlyRequestedFramesAreWritten() {
  EffectTestMemory<Effect> &memory = GetEffectTestMemory<Effect>();
  Effect &effect = GetTestEffect<Effect>();
  InitializeEffect(effect);
  constexpr AudioSample kInitialOutput[8] = {9.0f, 8.0f, 7.0f, 6.0f,
                                             5.0f, 4.0f, 3.0f, 2.0f};
  for (int sample = 0; sample < 8; ++sample) {
    memory.input[sample] = 0.0f;
    memory.output[sample] = kInitialOutput[sample];
  }

  effect.Render(memory.input, memory.output, 2);

  for (int sample = 0; sample < 4; ++sample) {
    CHECK(memory.output[sample] == doctest::Approx(0.0f));
  }
  for (int sample = 4; sample < 8; ++sample) {
    CHECK(memory.output[sample] == doctest::Approx(kInitialOutput[sample]));
  }
}

template <typename Effect> void CheckAudibleInputProducesWetOutput() {
  EffectTestMemory<Effect> &memory = GetEffectTestMemory<Effect>();
  Effect &effect = GetTestEffect<Effect>();
  InitializeEffect(effect);
  for (int sample = 0; sample < kTestSampleCount * 2; ++sample) {
    memory.input[sample] = 0.0f;
    memory.output[sample] = 0.0f;
  }

  memory.input[0] = 12000.0f / kAudioPcmScale;
  memory.input[1] = 12000.0f / kAudioPcmScale;

  bool gotAudibleWetOutput = false;
  for (int block = 0; block < 16 && !gotAudibleWetOutput; ++block) {
    effect.Render(memory.input, memory.output, kTestSampleCount);

    for (int sample = 0; sample < kTestSampleCount * 2; ++sample) {
      if (memory.output[sample] != 0.0f) {
        gotAudibleWetOutput = true;
        break;
      }
    }

    for (int sample = 0; sample < kTestSampleCount * 2; ++sample) {
      memory.input[sample] = 0.0f;
    }
  }

  CHECK(gotAudibleWetOutput);
}

} // namespace OutputEffectTestSupport

#endif
