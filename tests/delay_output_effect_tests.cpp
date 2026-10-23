/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Effects/DelayOutputEffect.h"
#include "Application/Player/SyncMaster.h"

namespace {

struct DelayTestMemory {
  AudioSample input[2];
  AudioSample output[2];
};

DelayTestMemory &GetDelayTestMemory() {
  static DelayTestMemory memory;
  return memory;
}

uint32_t GetExpectedDelayFrameCount(int time) {
  const float tickFrameCount = SyncMaster::GetInstance()->GetTickSampleCount();
  return static_cast<uint32_t>((tickFrameCount * static_cast<float>(time)) +
                               0.5f);
}

void ConfigureDelay(DelayTestMemory &memory, DelayOutputEffect &effect,
                    int time, int tempo,
                    int feedback = DelayOutputEffect::DefaultFeedback) {
  (void)memory;
  effect.Reset();
  SyncMaster::GetInstance()->SetTempo(tempo);

  Variable *timeVariable = effect.FindVariable(FourCC::VarDelayTime);
  Variable *feedbackVariable = effect.FindVariable(FourCC::VarDelayFeedback);
  REQUIRE(timeVariable != nullptr);
  REQUIRE(feedbackVariable != nullptr);

  timeVariable->SetInt(time);
  feedbackVariable->SetInt(feedback);
  effect.PrepareRender();
}

void RenderFrame(DelayTestMemory &memory, DelayOutputEffect &effect,
                 AudioSample inputLeft, AudioSample inputRight) {
  memory.input[0] = inputLeft;
  memory.input[1] = inputRight;
  memory.output[0] = kAudioSampleZero;
  memory.output[1] = kAudioSampleZero;
  effect.Render(memory.input, memory.output, 1);
}

} // namespace

TEST_CASE("Delay effect owns its parameter model") {
  DelayOutputEffect effect;
  Variable *time = effect.FindVariable(FourCC::VarDelayTime);
  Variable *feedback = effect.FindVariable(FourCC::VarDelayFeedback);
  REQUIRE(time != nullptr);
  REQUIRE(feedback != nullptr);

  time->SetInt(37);
  feedback->SetInt(DelayOutputEffect::MaximumFeedback);

  CHECK(effect.GetTime() == 37);
  CHECK(effect.GetFeedback() == DelayOutputEffect::MaximumFeedback);
}

TEST_CASE("Delay returns a stereo impulse after the selected musical time") {
  DelayTestMemory &memory = GetDelayTestMemory();
  DelayOutputEffect effect;
  ConfigureDelay(memory, effect, DelayOutputEffect::MinimumTime, MAX_TEMPO);
  const uint32_t delayFrameCount =
      GetExpectedDelayFrameCount(DelayOutputEffect::MinimumTime);

  CHECK(effect.GetTailFrameCount() == static_cast<int32_t>(delayFrameCount));
  for (uint32_t frame = 0; frame <= delayFrameCount; ++frame) {
    RenderFrame(memory, effect, frame == 0 ? 0.75f : 0.0f,
                frame == 0 ? -0.5f : 0.0f);

    const AudioSample expectedLeft = frame == delayFrameCount ? 0.75f : 0.0f;
    const AudioSample expectedRight = frame == delayFrameCount ? -0.5f : 0.0f;
    CHECK(memory.output[0] == doctest::Approx(expectedLeft));
    CHECK(memory.output[1] == doctest::Approx(expectedRight));
  }
}

TEST_CASE("Six delay time units equal one sequencer step") {
  DelayTestMemory &memory = GetDelayTestMemory();
  DelayOutputEffect effect;
  ConfigureDelay(memory, effect, DelayOutputEffect::TicksPerStep, MAX_TEMPO);

  const uint32_t expectedStepFrames = static_cast<uint32_t>(
      SyncMaster::GetInstance()->GetStepSampleCount() + 0.5f);
  CHECK(effect.GetTailFrameCount() == static_cast<int32_t>(expectedStepFrames));
}

TEST_CASE("Delay feedback repeats at the selected time and gain") {
  DelayTestMemory &memory = GetDelayTestMemory();
  DelayOutputEffect effect;
  ConfigureDelay(memory, effect, 2, MAX_TEMPO,
                 DelayOutputEffect::FeedbackScale / 2);
  const uint32_t delayFrameCount = GetExpectedDelayFrameCount(2);

  CHECK(effect.GetTailFrameCount() > static_cast<int32_t>(delayFrameCount));
  for (uint32_t frame = 0; frame <= delayFrameCount * 2U; ++frame) {
    RenderFrame(memory, effect, frame == 0 ? 0.8f : 0.0f,
                frame == 0 ? -0.4f : 0.0f);

    AudioSample expectedLeft = 0.0f;
    AudioSample expectedRight = 0.0f;
    if (frame == delayFrameCount) {
      expectedLeft = 0.8f;
      expectedRight = -0.4f;
    } else if (frame == delayFrameCount * 2U) {
      expectedLeft = 0.4f;
      expectedRight = -0.2f;
    }
    CHECK(memory.output[0] == doctest::Approx(expectedLeft));
    CHECK(memory.output[1] == doctest::Approx(expectedRight));
  }
}

TEST_CASE("Delay maximum feedback remains below unity") {
  DelayTestMemory &memory = GetDelayTestMemory();
  DelayOutputEffect effect;
  ConfigureDelay(memory, effect, DelayOutputEffect::MinimumTime, MAX_TEMPO,
                 DelayOutputEffect::MaximumFeedback);
  const uint32_t delayFrameCount =
      GetExpectedDelayFrameCount(DelayOutputEffect::MinimumTime);
  CHECK(effect.GetTailFrameCount() > static_cast<int32_t>(delayFrameCount));
  CHECK(effect.GetTailFrameCount() < INT32_MAX);
  const AudioSample feedbackGain =
      static_cast<AudioSample>(DelayOutputEffect::MaximumFeedback) /
      static_cast<AudioSample>(DelayOutputEffect::FeedbackScale);
  CHECK(feedbackGain == doctest::Approx(255.0f / 256.0f));
  CHECK(feedbackGain < kAudioSampleOne);
  AudioSample expectedEcho = 0.5f;

  for (uint32_t frame = 0; frame <= delayFrameCount * 10U; ++frame) {
    RenderFrame(memory, effect, frame == 0 ? expectedEcho : 0.0f, 0.0f);

    if (frame > 0 && frame % delayFrameCount == 0) {
      CHECK(memory.output[0] == doctest::Approx(expectedEcho));
      expectedEcho *= feedbackGain;
    } else {
      CHECK(memory.output[0] == doctest::Approx(0.0f));
    }
    CHECK(memory.output[1] == doctest::Approx(0.0f));
  }
  CHECK(expectedEcho < 0.5f);
}

TEST_CASE("Delay maximum time is a full bar at the minimum tempo") {
  DelayTestMemory &memory = GetDelayTestMemory();
  DelayOutputEffect effect;
  ConfigureDelay(memory, effect, DelayOutputEffect::MaximumTime, MIN_TEMPO);

  const uint32_t expectedFullBarFrames = static_cast<uint32_t>(
      (SyncMaster::GetInstance()->GetStepSampleCount() * 16.0f) + 0.5f);
  CHECK(DelayOutputEffect::MaximumTime == DelayOutputEffect::TicksPerBar);
  CHECK(expectedFullBarFrames == DelayOutputEffect::kBufferFrameCount);
  CHECK(DelayOutputEffect::kExternalMemorySize ==
        static_cast<size_t>(expectedFullBarFrames) * sizeof(AudioSample) * 2U);
  CHECK(effect.GetTailFrameCount() ==
        static_cast<int32_t>(expectedFullBarFrames));
}

TEST_CASE("Delay tail is not capped while feedback remains audible") {
  DelayTestMemory &memory = GetDelayTestMemory();
  DelayOutputEffect effect;
  ConfigureDelay(memory, effect, DelayOutputEffect::MaximumTime, MIN_TEMPO,
                 DelayOutputEffect::MaximumFeedback);

  CHECK(effect.GetTailFrameCount() >
        10 * static_cast<int32_t>(kAudioSampleRateHz));
}

TEST_CASE("Delay follows tempo changes without effect reconfiguration") {
  DelayTestMemory &memory = GetDelayTestMemory();
  DelayOutputEffect effect;
  ConfigureDelay(memory, effect, DelayOutputEffect::MinimumTime, MAX_TEMPO);

  SyncMaster::GetInstance()->SetTempo(MAX_TEMPO / 2);
  const uint32_t delayFrameCount =
      GetExpectedDelayFrameCount(DelayOutputEffect::MinimumTime);
  for (uint32_t frame = 0; frame <= delayFrameCount; ++frame) {
    RenderFrame(memory, effect, frame == 0 ? 0.6f : 0.0f,
                frame == 0 ? -0.3f : 0.0f);

    CHECK(memory.output[0] ==
          doctest::Approx(frame == delayFrameCount ? 0.6f : 0.0f));
    CHECK(memory.output[1] ==
          doctest::Approx(frame == delayFrameCount ? -0.3f : 0.0f));
  }
}

TEST_CASE("Reset removes audio waiting in the delay memory") {
  DelayTestMemory &memory = GetDelayTestMemory();
  DelayOutputEffect effect;
  ConfigureDelay(memory, effect, DelayOutputEffect::MinimumTime, MAX_TEMPO);
  const uint32_t delayFrameCount =
      GetExpectedDelayFrameCount(DelayOutputEffect::MinimumTime);

  RenderFrame(memory, effect, 0.6f, -0.3f);
  effect.Reset();

  for (uint32_t frame = 0; frame <= delayFrameCount; ++frame) {
    RenderFrame(memory, effect, 0.0f, 0.0f);
    CHECK(memory.output[0] == doctest::Approx(0.0f));
    CHECK(memory.output[1] == doctest::Approx(0.0f));
  }
}
