/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Audio/AudioSample.h"
#include "Application/Effects/OutputEffectSlot.h"

namespace {

AudioSample *GetTestWetBuffer(int index = 0) {
  static AudioSample wetBuffers[2][MAX_SAMPLE_COUNT * 2];
  return wetBuffers[index];
}

NoneOutputEffect &GetNoneEffect() {
  static NoneOutputEffect effect;
  return effect;
}

class TestOutputEffect : public OutputEffect {
public:
  TestOutputEffect() : OutputEffect(&variables_) {}

private:
  etl::ivector<Variable *> *Variables() override { return &variables_; }

  etl::vector<Variable *, 1> variables_;
};

class PassthroughTestEffect : public TestOutputEffect {
public:
  void Render(const AudioSample *input, AudioSample *output,
              int samplecount) override {
    const int count = samplecount * 2;
    for (int i = 0; i < count; ++i) {
      output[i] = input[i];
    }
  }
};

class DoubleOutputEffect : public TestOutputEffect {
public:
  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) override {
    const int count = samplecount * 2;
    for (int i = 0; i < count; ++i) {
      output[i] = input[i] * 2.0f;
    }
  }
};

class ResetTrackingEffect : public TestOutputEffect {
public:
  void Reset() override { resetCount_++; }
  int32_t GetTailFrameCount() const override { return 4; }

  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) override {
    const int count = samplecount * 2;
    for (int i = 0; i < count; ++i) {
      output[i] = input[i];
    }
  }

  int resetCount_ = 0;
};

class TailTrackingEffect : public TestOutputEffect {
public:
  int32_t GetTailFrameCount() const override { return 2; }

  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) override {
    ++renderCount_;
    const int count = samplecount * 2;
    for (int i = 0; i < count; ++i) {
      output[i] = input[i];
    }
  }

  int renderCount_ = 0;
};

} // namespace

TEST_CASE("OutputEffectSlot returns false when untouched") {
  OutputEffectSlot slot(GetTestWetBuffer(), GetNoneEffect());
  AudioSample buffer[8] = {};

  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(kAudioSampleOne);
  CHECK(slot.Render(buffer, 4));
  CHECK(buffer[0] == 0);
  CHECK(buffer[1] == 0);
}

TEST_CASE("OutputEffectSlot renders accumulated wet buffer at unity gain") {
  PassthroughTestEffect effect;
  OutputEffectSlot slot(GetTestWetBuffer(), effect);
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
  OutputEffectSlot slot(GetTestWetBuffer(), GetNoneEffect());

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
  PassthroughTestEffect effect;
  OutputEffectSlot slot(GetTestWetBuffer(), effect);
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

TEST_CASE("OutputEffectSlot renders through its fixed effect processor") {
  DoubleOutputEffect effect;
  OutputEffectSlot slot(GetTestWetBuffer(), effect);
  AudioSample buffer[2] = {};

  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(0.5f);
  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 3.0f;
  wet[1] = 4.0f;

  REQUIRE(slot.Render(buffer, 1));
  CHECK(buffer[0] == doctest::Approx(3.0f));
  CHECK(buffer[1] == doctest::Approx(4.0f));
}

TEST_CASE("OutputEffectSlot renders silence through a None effect") {
  OutputEffectSlot slot(GetTestWetBuffer(), GetNoneEffect());
  AudioSample buffer[2] = {};

  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(kAudioSampleOne);
  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 7.0f;
  wet[1] = 9.0f;

  REQUIRE(slot.Render(buffer, 1));
  CHECK(buffer[0] == doctest::Approx(0.0f));
  CHECK(buffer[1] == doctest::Approx(0.0f));
}

TEST_CASE("OutputEffectSlot reset clears tails and processor state") {
  ResetTrackingEffect effect;
  OutputEffectSlot slot(GetTestWetBuffer(), effect);

  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 7.0f;
  wet[1] = 9.0f;
  AudioSample output[2] = {};

  slot.Render(output, 1);
  REQUIRE(slot.IsTailActive());

  slot.ResetState();

  CHECK(effect.resetCount_ == 1);
  CHECK_FALSE(slot.IsTailActive());
  CHECK(wet[0] == 0);
  CHECK(wet[1] == 0);
}

TEST_CASE("OutputEffectSlot skips idle processors and renders declared tails") {
  TailTrackingEffect effect;
  OutputEffectSlot slot(GetTestWetBuffer(), effect);
  AudioSample output[2] = {};

  slot.ClearAccumulationBuffer();
  slot.Render(output, 1);
  CHECK(effect.renderCount_ == 0);
  CHECK_FALSE(slot.IsTailActive());

  AudioSample *wet = slot.GetAccumulationBuffer();
  wet[0] = 1.0f;
  slot.Render(output, 1);
  CHECK(effect.renderCount_ == 1);
  CHECK(slot.IsTailActive());

  slot.ClearAccumulationBuffer();
  slot.Render(output, 1);
  CHECK(effect.renderCount_ == 2);
  CHECK(slot.IsTailActive());

  slot.ClearAccumulationBuffer();
  slot.Render(output, 1);
  CHECK(effect.renderCount_ == 3);
  CHECK_FALSE(slot.IsTailActive());

  output[0] = 9.0f;
  output[1] = 8.0f;
  slot.ClearAccumulationBuffer();
  slot.Render(output, 1);
  CHECK(effect.renderCount_ == 3);
  CHECK(output[0] == doctest::Approx(0.0f));
  CHECK(output[1] == doctest::Approx(0.0f));
}

TEST_CASE("OutputEffectSlot routes its wet output downstream") {
  DoubleOutputEffect effect;
  OutputEffectSlot slot(GetTestWetBuffer(), effect);
  AudioSample downstream[2] = {1.0f, 2.0f};
  AudioSample output[2] = {};

  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(0.25f);
  slot.AddSendRouting(downstream, 0.25f);
  AudioSample *input = slot.GetAccumulationBuffer();
  input[0] = 4.0f;
  input[1] = 8.0f;

  REQUIRE(slot.Render(output, 1));

  // Downstream routing uses the fully wet processor output.
  CHECK(downstream[0] == doctest::Approx(3.0f));
  CHECK(downstream[1] == doctest::Approx(6.0f));
  // The master return gain is applied only after the downstream send.
  CHECK(output[0] == doctest::Approx(2.0f));
  CHECK(output[1] == doctest::Approx(4.0f));
}

TEST_CASE("OutputEffectSlot zero-gain send contributes silence downstream") {
  DoubleOutputEffect effect;
  OutputEffectSlot slot(GetTestWetBuffer(), effect);
  AudioSample downstream[2] = {1.0f, 2.0f};
  AudioSample output[2] = {};

  slot.ClearAccumulationBuffer();
  slot.BeginRenderCycle(kAudioSampleOne);
  slot.AddSendRouting(downstream, kAudioSampleZero);
  AudioSample *input = slot.GetAccumulationBuffer();
  input[0] = 4.0f;
  input[1] = 8.0f;

  REQUIRE(slot.Render(output, 1));
  CHECK(downstream[0] == doctest::Approx(1.0f));
  CHECK(downstream[1] == doctest::Approx(2.0f));
}

TEST_CASE("OutputEffectSlot output can feed a later processing slot") {
  DoubleOutputEffect firstEffect;
  DoubleOutputEffect secondEffect;
  OutputEffectSlot first(GetTestWetBuffer(0), firstEffect);
  OutputEffectSlot second(GetTestWetBuffer(1), secondEffect);
  AudioSample firstOutput[2] = {};
  AudioSample secondOutput[2] = {};

  first.ClearAccumulationBuffer();
  second.ClearAccumulationBuffer();
  first.BeginRenderCycle(kAudioSampleZero);
  second.BeginRenderCycle(kAudioSampleOne);
  first.AddSendRouting(second.GetAccumulationBuffer(), kAudioSampleOne);
  AudioSample *input = first.GetAccumulationBuffer();
  input[0] = 0.25f;
  input[1] = -0.5f;

  REQUIRE(first.Render(firstOutput, 1));
  REQUIRE(second.Render(secondOutput, 1));

  CHECK(firstOutput[0] == doctest::Approx(0.0f));
  CHECK(firstOutput[1] == doctest::Approx(0.0f));
  CHECK(secondOutput[0] == doctest::Approx(1.0f));
  CHECK(secondOutput[1] == doctest::Approx(-2.0f));
}
