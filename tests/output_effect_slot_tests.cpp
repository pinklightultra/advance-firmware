/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Audio/AudioSample.h"
#include "Application/Mixer/OutputEffectSlot.h"
#include "Application/Mixer/PassthroughOutputEffect.h"
#include "Application/Mixer/SilentOutputEffect.h"

namespace {

AudioSample *GetTestWetBuffer() {
  static AudioSample wetBuffer[MAX_SAMPLE_COUNT * 2];
  return wetBuffer;
}

PassthroughOutputEffect &GetDefaultEffect() {
  static PassthroughOutputEffect effect;
  return effect;
}

class DoubleOutputEffect : public OutputEffectProcessor {
public:
  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) override {
    const int count = samplecount * 2;
    for (int i = 0; i < count; ++i) {
      output[i] = input[i] * 2.0f;
    }
  }

  virtual const char *GetShortName() const override { return "DBL"; }
};

class ResetTrackingEffect : public OutputEffectProcessor {
public:
  void Reset() override { resetCount_++; }

  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) override {
    const int count = samplecount * 2;
    for (int i = 0; i < count; ++i) {
      output[i] = input[i];
    }
  }

  virtual const char *GetShortName() const override { return "RST"; }

  int resetCount_ = 0;
};

class TailTrackingEffect : public OutputEffectProcessor {
public:
  bool IsTailActive() const override { return tailActive_; }

  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) override {
    const int count = samplecount * 2;
    for (int i = 0; i < count; ++i) {
      output[i] = input[i];
    }
  }

  virtual const char *GetShortName() const override { return "TAIL"; }

  bool tailActive_ = false;
};

} // namespace

TEST_CASE("OutputEffectSlot returns false when untouched") {
  OutputEffectSlot slot(GetTestWetBuffer(), &GetDefaultEffect());
  AudioSample buffer[8] = {};

  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(kAudioSampleOne);
  CHECK(slot.Render(buffer, 4));
  CHECK(buffer[0] == 0);
  CHECK(buffer[1] == 0);
}

TEST_CASE("OutputEffectSlot renders accumulated wet buffer at unity gain") {
  OutputEffectSlot slot(GetTestWetBuffer(), &GetDefaultEffect());
  AudioSample buffer[4] = {};

  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(kAudioSampleOne);
  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 1.0f;
  wet[1] = 2.0f;
  wet[2] = 3.0f;
  wet[3] = 4.0f;

  REQUIRE(slot.Render(buffer, 2));
  CHECK(buffer[0] == doctest::Approx(1.0f));
  CHECK(buffer[1] == doctest::Approx(2.0f));
  CHECK(buffer[2] == doctest::Approx(3.0f));
  CHECK(buffer[3] == doctest::Approx(4.0f));
}

TEST_CASE("OutputEffectSlot clears accumulation buffer on request") {
  OutputEffectSlot slot(GetTestWetBuffer(), &GetDefaultEffect());

  slot.ClearAccumulationBuffer();
  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 5.0f;
  wet[1] = 6.0f;

  slot.ClearAccumulationBuffer();
  AudioSample *next = slot.GetAccumulationBuffer();

  CHECK(next[0] == 0);
  CHECK(next[1] == 0);
}

TEST_CASE("OutputEffectSlot applies return gain during render") {
  OutputEffectSlot slot(GetTestWetBuffer(), &GetDefaultEffect());
  AudioSample buffer[2] = {};

  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(0.5f);
  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 8.0f;
  wet[1] = 6.0f;

  REQUIRE(slot.Render(buffer, 1));
  CHECK(buffer[0] == doctest::Approx(4.0f));
  CHECK(buffer[1] == doctest::Approx(3.0f));
}

TEST_CASE("OutputEffectSlot renders through assigned effect processor") {
  OutputEffectSlot slot(GetTestWetBuffer(), &GetDefaultEffect());
  DoubleOutputEffect effect;
  AudioSample buffer[2] = {};

  slot.SetEffect(&effect);
  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(0.5f);
  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 3.0f;
  wet[1] = 4.0f;

  REQUIRE(slot.Render(buffer, 1));
  CHECK(buffer[0] == doctest::Approx(3.0f));
  CHECK(buffer[1] == doctest::Approx(4.0f));
}

TEST_CASE("OutputEffectSlot falls back to default effect when cleared") {
  OutputEffectSlot slot(GetTestWetBuffer(), &GetDefaultEffect());
  DoubleOutputEffect effect;
  AudioSample buffer[2] = {};

  slot.SetEffect(&effect);
  slot.SetEffect(nullptr);
  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(kAudioSampleOne);
  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 7.0f;
  wet[1] = 9.0f;

  REQUIRE(slot.Render(buffer, 1));
  CHECK(buffer[0] == doctest::Approx(7.0f));
  CHECK(buffer[1] == doctest::Approx(9.0f));
}

TEST_CASE("OutputEffectSlot reset clears tails and processor state") {
  OutputEffectSlot slot(GetTestWetBuffer(), &GetDefaultEffect());
  ResetTrackingEffect effect;

  slot.SetEffect(&effect);
  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 7.0f;
  wet[1] = 9.0f;

  slot.ResetState();

  CHECK(effect.resetCount_ == 1);
  CHECK(wet[0] == 0);
  CHECK(wet[1] == 0);
}

TEST_CASE("OutputEffectSlot reports its processor tail state") {
  OutputEffectSlot slot(GetTestWetBuffer(), &GetDefaultEffect());
  TailTrackingEffect effect;

  slot.SetEffect(&effect);
  CHECK_FALSE(slot.IsTailActive());

  effect.tailActive_ = true;
  CHECK(slot.IsTailActive());
}

TEST_CASE("PassthroughOutputEffect copies wet input unchanged") {
  PassthroughOutputEffect effect;
  AudioSample input[4] = {1.0f, 2.0f, 3.0f, 4.0f};
  AudioSample output[4] = {};

  effect.Render(input, output, 2);

  CHECK(output[0] == doctest::Approx(1.0f));
  CHECK(output[1] == doctest::Approx(2.0f));
  CHECK(output[2] == doctest::Approx(3.0f));
  CHECK(output[3] == doctest::Approx(4.0f));
}

TEST_CASE("SilentOutputEffect always outputs zeros") {
  SilentOutputEffect effect;
  AudioSample input[4] = {1.0f, 2.0f, 3.0f, 4.0f};
  AudioSample output[4] = {5.0f, 6.0f, 7.0f, 8.0f};

  effect.Render(input, output, 2);

  CHECK(output[0] == doctest::Approx(0.0f));
  CHECK(output[1] == doctest::Approx(0.0f));
  CHECK(output[2] == doctest::Approx(0.0f));
  CHECK(output[3] == doctest::Approx(0.0f));
}

TEST_CASE("OutputEffectSlot with SilentOutputEffect default stays silent") {
  SilentOutputEffect effect;
  OutputEffectSlot slot(GetTestWetBuffer(), &effect);
  AudioSample buffer[4] = {9.0f, 8.0f, 7.0f, 6.0f};

  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(kAudioSampleOne);
  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 1.0f;
  wet[1] = 2.0f;
  wet[2] = 3.0f;
  wet[3] = 4.0f;

  REQUIRE(slot.Render(buffer, 2));
  CHECK(buffer[0] == doctest::Approx(0.0f));
  CHECK(buffer[1] == doctest::Approx(0.0f));
  CHECK(buffer[2] == doctest::Approx(0.0f));
  CHECK(buffer[3] == doctest::Approx(0.0f));
}
