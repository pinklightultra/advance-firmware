/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Audio/AudioSample.h"
#include "Application/Player/BitcrushInsertEffect.h"
#include "Application/Player/ClipInsertEffect.h"
#include "Application/Player/DCOffsetInsertEffect.h"
#include "Application/Player/DetuneInsertEffect.h"
#include "Application/Player/DownsampleInsertEffect.h"
#include "Application/Player/InsertEffectDefinitions.h"
#include "Application/Player/InsertEffectSlot.h"
#include "Application/Player/OverdriveInsertEffect.h"
#include "Application/Player/PassthroughInsertEffect.h"
#include "Application/Player/RectifyInsertEffect.h"
#include "Application/Player/SlewInsertEffect.h"
#include <algorithm>
#include <array>

namespace {

AudioSample expectedBitcrushQuantizedSample(AudioSample sample, int amount) {
  constexpr int32_t kMinPcm = -32768;
  constexpr int32_t kMaxPcm = 32767;
  constexpr int kMaxBitReduction = 15;
  const int clampedAmount = std::clamp(amount, 0, 0xFF);
  const int inverse = 0xFF - clampedAmount;
  const int curvedAmount = 0xFF - ((inverse * inverse + (0xFF / 2)) / 0xFF);
  const int shift = (curvedAmount * kMaxBitReduction + (0xFF / 2)) / 0xFF;
  const int32_t quantizationStep = 1 << shift;

  const AudioSample scaled = sample * kAudioPcmScale;
  const int32_t pcm = std::clamp(
      static_cast<int32_t>(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f),
      kMinPcm, kMaxPcm);
  if (pcm == 0) {
    return 0.0f;
  }
  if (quantizationStep == 1) {
    return static_cast<AudioSample>(pcm) / kAudioPcmScale;
  }

  const int32_t halfStep = quantizationStep >> 1;
  const bool negative = pcm < 0;
  const int32_t magnitude = std::min<int32_t>(negative ? -pcm : pcm, kMaxPcm);
  const int32_t quantizedMagnitude =
      ((magnitude / quantizationStep) * quantizationStep) + halfStep;
  const int32_t quantized = std::clamp(
      negative ? -quantizedMagnitude : quantizedMagnitude, kMinPcm, kMaxPcm);
  return static_cast<AudioSample>(quantized) / kAudioPcmScale;
}

AudioSample expectedOverdriveSample(AudioSample sample, int amount) {
  constexpr int kAmountMax = 0xFF;
  constexpr int kControlStepMax = 127;
  constexpr int kControlStepCount = 128;
  constexpr float kMinDrive = 0.15f;
  constexpr float kMaxDrive = 1.0f;
  constexpr float kDriveRange = kMaxDrive - kMinDrive;
  constexpr int kStageCount = 3;

  const int clampedAmount = std::clamp(amount, 0, kAmountMax);
  if (clampedAmount == 0) {
    return sample;
  }

  const int control =
      ((clampedAmount * kControlStepMax) + (kAmountMax / 2)) / kAmountMax;
  const float normalizedControl =
      static_cast<float>(control) / static_cast<float>(kControlStepCount);
  const float drive =
      kMinDrive + (kDriveRange * normalizedControl * normalizedControl);
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

AudioSample expectedClipSample(AudioSample sample, int amount) {
  constexpr int32_t kMaxPcm = 32767;
  constexpr int32_t kMinThreshold = 2048;
  const int clampedAmount = std::clamp(amount, 0, 0xFF);
  const int32_t threshold =
      kMaxPcm -
      ((clampedAmount * (kMaxPcm - kMinThreshold)) + (0xFF / 2)) / 0xFF;
  const int32_t pcm = std::clamp(static_cast<int32_t>(sample * kAudioPcmScale),
                                 -kMaxPcm, kMaxPcm);
  return static_cast<AudioSample>(std::clamp(pcm, -threshold, threshold)) /
         kAudioPcmScale;
}

AudioSample expectedRectifySample(AudioSample sample, int amount) {
  constexpr int32_t kMinPcm = -32768;
  constexpr int32_t kMaxPcm = 32767;
  const int clampedAmount = std::clamp(amount, 0, 0xFF);
  const int32_t pcm = std::clamp(static_cast<int32_t>(sample * kAudioPcmScale),
                                 kMinPcm, kMaxPcm);
  const int32_t rectified = std::min<int32_t>(pcm < 0 ? -pcm : pcm, kMaxPcm);
  const int32_t blended =
      pcm + ((clampedAmount * (rectified - pcm)) + (0xFF / 2)) / 0xFF;
  return static_cast<AudioSample>(blended) / kAudioPcmScale;
}

AudioSample expectedDCOffsetSample(AudioSample sample, int amount) {
  constexpr int32_t kMinPcm = -32768;
  constexpr int32_t kMaxPcm = 32767;
  constexpr int32_t kMaxOffset = 8192;
  const int clampedAmount = std::clamp(amount, 0, 0xFF);
  const int32_t offset = ((clampedAmount - 0x80) * kMaxOffset) / 0x80;
  const int32_t pcm = std::clamp(static_cast<int32_t>(sample * kAudioPcmScale),
                                 kMinPcm, kMaxPcm);
  return static_cast<AudioSample>(std::clamp(pcm + offset, kMinPcm, kMaxPcm)) /
         kAudioPcmScale;
}

AudioSample expectedSlewStep(int amount) {
  constexpr int32_t kMaxPcm = 32767;
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
  const int32_t maxDelta =
      std::clamp<int32_t>(kPcmSpan / rampFrames, 1, kMaxPcm);
  return static_cast<AudioSample>(maxDelta) / kAudioPcmScale;
}

class GainInsertEffect : public InsertEffectProcessor {
public:
  explicit GainInsertEffect(AudioSample gain) : gain_(gain) {}

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    (void)amount;
    const int count = samplecount * GetAudioBufferLayoutChannelCount(layout);
    for (int i = 0; i < count; ++i) {
      buffer[i] *= gain_;
    }
  }

  virtual const char *GetShortName() const override { return "GIN"; }

private:
  AudioSample gain_;
};

class ResetTrackingInsertEffect : public InsertEffectProcessor {
public:
  void Reset() override { resetCount_++; }

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    (void)buffer;
    (void)samplecount;
    (void)amount;
    (void)layout;
  }

  virtual const char *GetShortName() const override { return "RST"; }

  int resetCount_ = 0;
};

} // namespace

TEST_CASE("InsertEffectSlot renders through its default effect") {
  PassthroughInsertEffect effect;
  InsertEffectSlot slot;
  slot.SetEffect(&effect);
  AudioSample buffer[4] = {1.0f, -2.0f, 3.0f, -4.0f};

  slot.Render(buffer, 2, kDefaultVoiceInsertAmount,
              AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(1.0f));
  CHECK(buffer[1] == doctest::Approx(-2.0f));
  CHECK(buffer[2] == doctest::Approx(3.0f));
  CHECK(buffer[3] == doctest::Approx(-4.0f));
}

TEST_CASE("InsertEffectSlot renders through the selected processor") {
  PassthroughInsertEffect passthrough;
  GainInsertEffect effect(0.5f);
  InsertEffectSlot slot;
  slot.SetEffect(&passthrough);
  AudioSample buffer[4] = {1.0f, -2.0f, 3.0f, -4.0f};

  slot.SetEffect(&effect);
  slot.Render(buffer, 2, kDefaultVoiceInsertAmount,
              AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(0.5f));
  CHECK(buffer[1] == doctest::Approx(-1.0f));
  CHECK(buffer[2] == doctest::Approx(1.5f));
  CHECK(buffer[3] == doctest::Approx(-2.0f));
}

TEST_CASE("InsertEffectSlot switches back explicitly to passthrough") {
  PassthroughInsertEffect passthrough;
  GainInsertEffect effect(0.5f);
  InsertEffectSlot slot;
  slot.SetEffect(&passthrough);
  AudioSample buffer[2] = {1.0f, -2.0f};

  slot.SetEffect(&effect);
  slot.SetEffect(&passthrough);
  slot.Render(buffer, 1, kDefaultVoiceInsertAmount,
              AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(1.0f));
  CHECK(buffer[1] == doctest::Approx(-2.0f));
}

TEST_CASE("InsertEffectSlot reset forwards to the selected processor") {
  PassthroughInsertEffect passthrough;
  ResetTrackingInsertEffect effect;
  InsertEffectSlot slot;
  slot.SetEffect(&passthrough);

  slot.SetEffect(&effect);
  slot.ResetState();

  CHECK(effect.resetCount_ == 1);
}

TEST_CASE("BitcrushInsertEffect quantizes audio through PCM integer steps") {
  BitcrushInsertEffect effect;
  AudioSample buffer[4] = {0.1234f, -0.1234f, 0.1234f, -0.1234f};

  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] ==
        doctest::Approx(expectedBitcrushQuantizedSample(0.1234f, 0xFF)));
  CHECK(buffer[1] ==
        doctest::Approx(expectedBitcrushQuantizedSample(-0.1234f, 0xFF)));
  CHECK(buffer[2] ==
        doctest::Approx(expectedBitcrushQuantizedSample(0.1234f, 0xFF)));
  CHECK(buffer[3] ==
        doctest::Approx(expectedBitcrushQuantizedSample(-0.1234f, 0xFF)));
}

TEST_CASE("BitcrushInsertEffect max amount has two nonzero output levels") {
  BitcrushInsertEffect effect;
  AudioSample buffer[8] = {1000.0f / kAudioPcmScale,
                           -1000.0f / kAudioPcmScale,
                           10000.0f / kAudioPcmScale,
                           -10000.0f / kAudioPcmScale,
                           1.0f,
                           -1.0f,
                           0.0f,
                           0.0f};

  effect.Render(buffer, 4, 0xFF, AudioBufferLayout::StereoInterleaved);

  const AudioSample positiveLevel = 16384.0f / kAudioPcmScale;
  const AudioSample negativeLevel = -16384.0f / kAudioPcmScale;
  CHECK(buffer[0] == doctest::Approx(positiveLevel));
  CHECK(buffer[1] == doctest::Approx(negativeLevel));
  CHECK(buffer[2] == doctest::Approx(positiveLevel));
  CHECK(buffer[3] == doctest::Approx(negativeLevel));
  CHECK(buffer[4] == doctest::Approx(positiveLevel));
  CHECK(buffer[5] == doctest::Approx(negativeLevel));
  CHECK(buffer[6] == doctest::Approx(0.0f));
  CHECK(buffer[7] == doctest::Approx(0.0f));
}

TEST_CASE("BitcrushInsertEffect amount zero preserves PCM resolution") {
  BitcrushInsertEffect effect;
  AudioSample buffer[2] = {0.1234f, -0.1234f};

  effect.Render(buffer, 1, 0, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] ==
        doctest::Approx(expectedBitcrushQuantizedSample(0.1234f, 0)));
  CHECK(buffer[1] ==
        doctest::Approx(expectedBitcrushQuantizedSample(-0.1234f, 0)));
}

TEST_CASE(
    "BitcrushInsertEffect midpoint amount reaches the upper crush range") {
  BitcrushInsertEffect effect;
  AudioSample buffer[2] = {0.125f, -0.125f};

  effect.Render(buffer, 1, 0x80, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(5120.0f / kAudioPcmScale));
  CHECK(buffer[1] == doctest::Approx(-5120.0f / kAudioPcmScale));
}

TEST_CASE("OverdriveInsertEffect amount zero preserves normalized output") {
  OverdriveInsertEffect effect;
  AudioSample buffer[4] = {0.25f, -0.5f, 0.75f, -0.125f};

  effect.Render(buffer, 2, 0, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(expectedOverdriveSample(0.25f, 0)));
  CHECK(buffer[1] == doctest::Approx(expectedOverdriveSample(-0.5f, 0)));
  CHECK(buffer[2] == doctest::Approx(expectedOverdriveSample(0.75f, 0)));
  CHECK(buffer[3] == doctest::Approx(expectedOverdriveSample(-0.125f, 0)));
}

TEST_CASE("OverdriveInsertEffect high amount matches the three-stage curve") {
  OverdriveInsertEffect effect;
  AudioSample buffer[4] = {0.4f, -0.6f, 0.8f, -0.8f};

  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(expectedOverdriveSample(0.4f, 0xFF)));
  CHECK(buffer[1] == doctest::Approx(expectedOverdriveSample(-0.6f, 0xFF)));
  CHECK(buffer[2] == doctest::Approx(expectedOverdriveSample(0.8f, 0xFF)));
  CHECK(buffer[3] == doctest::Approx(expectedOverdriveSample(-0.8f, 0xFF)));
}

TEST_CASE(
    "OverdriveInsertEffect smooths drive changes across a control block") {
  OverdriveInsertEffect effect;
  AudioSample warmup[1] = {0.5f};
  effect.Render(warmup, 1, 0x20, AudioBufferLayout::MonoPacked);

  AudioSample buffer[2] = {0.5f, 0.5f};
  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::MonoPacked);

  CHECK(buffer[0] == doctest::Approx(expectedOverdriveSample(0.5f, 0x20)));
  CHECK(buffer[1] > buffer[0]);
  CHECK(buffer[1] < expectedOverdriveSample(0.5f, 0xFF));
}

TEST_CASE(
    "BitcrushInsertEffect high amount quantizes each frame independently") {
  BitcrushInsertEffect effect;
  constexpr int kAmount = 0xC0;
  AudioSample buffer[4] = {0.125f, -0.125f, 0.5f, -0.5f};

  effect.Render(buffer, 2, kAmount, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] ==
        doctest::Approx(expectedBitcrushQuantizedSample(0.125f, kAmount)));
  CHECK(buffer[1] ==
        doctest::Approx(expectedBitcrushQuantizedSample(-0.125f, kAmount)));
  CHECK(buffer[2] ==
        doctest::Approx(expectedBitcrushQuantizedSample(0.5f, kAmount)));
  CHECK(buffer[3] ==
        doctest::Approx(expectedBitcrushQuantizedSample(-0.5f, kAmount)));
  CHECK(buffer[2] != doctest::Approx(buffer[0]));
  CHECK(buffer[3] != doctest::Approx(buffer[1]));
}

TEST_CASE("DetuneInsertEffect leaves audio unchanged at zero amount") {
  DetuneInsertEffect effect;
  AudioSample buffer[4] = {0.25f, -0.25f, 0.5f, -0.5f};

  effect.Render(buffer, 2, 0, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(0.25f));
  CHECK(buffer[1] == doctest::Approx(-0.25f));
  CHECK(buffer[2] == doctest::Approx(0.5f));
  CHECK(buffer[3] == doctest::Approx(-0.5f));
}

TEST_CASE("DetuneInsertEffect high amount modulates alternating audio") {
  DetuneInsertEffect effect;
  std::array<AudioSample, 512> buffer{};
  std::array<AudioSample, 512> dryBuffer{};

  for (size_t i = 0; i < buffer.size() / 2; ++i) {
    const AudioSample value = (i & 1) ? -0.75f : 0.75f;
    buffer[i * 2] = value;
    buffer[i * 2 + 1] = -value;
    dryBuffer[i * 2] = value;
    dryBuffer[i * 2 + 1] = -value;
  }

  effect.Render(buffer.data(), static_cast<int>(buffer.size() / 2), 0xFF,
                AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[510] != doctest::Approx(dryBuffer[510]));
  CHECK(buffer[511] != doctest::Approx(dryBuffer[511]));
}

TEST_CASE("DetuneInsertEffect keeps silent stereo channels isolated") {
  DetuneInsertEffect effect;
  std::array<AudioSample, 384> buffer{};

  for (size_t i = 0; i < buffer.size() / 2; ++i) {
    buffer[i * 2] = (i & 1) ? -0.5f : 0.5f;
    buffer[i * 2 + 1] = 0.0f;
  }

  effect.Render(buffer.data(), static_cast<int>(buffer.size() / 2), 0xFF,
                AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[382] != doctest::Approx(-0.5f));
  for (size_t i = 0; i < buffer.size() / 2; ++i) {
    CHECK(buffer[i * 2 + 1] == doctest::Approx(0.0f));
  }
}

TEST_CASE("DetuneInsertEffect reset clears delay state") {
  DetuneInsertEffect effect;
  std::array<AudioSample, 384> buffer{};
  AudioSample silence[4] = {};

  for (size_t i = 0; i < buffer.size() / 2; ++i) {
    const AudioSample value = (i & 1) ? -0.75f : 0.75f;
    buffer[i * 2] = value;
    buffer[i * 2 + 1] = -value;
  }

  effect.Render(buffer.data(), static_cast<int>(buffer.size() / 2), 0xFF,
                AudioBufferLayout::StereoInterleaved);
  effect.Reset();
  effect.Render(silence, 2, 0xFF, AudioBufferLayout::StereoInterleaved);

  CHECK(silence[0] == doctest::Approx(0.0f));
  CHECK(silence[1] == doctest::Approx(0.0f));
  CHECK(silence[2] == doctest::Approx(0.0f));
  CHECK(silence[3] == doctest::Approx(0.0f));
}

TEST_CASE("DownsampleInsertEffect high amount holds samples between frames") {
  DownsampleInsertEffect effect;
  AudioSample buffer[4] = {0.125f, -0.125f, 0.5f, -0.5f};

  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(0.125f));
  CHECK(buffer[1] == doctest::Approx(-0.125f));
  CHECK(buffer[2] == doctest::Approx(buffer[0]));
  CHECK(buffer[3] == doctest::Approx(buffer[1]));
}

TEST_CASE("DownsampleInsertEffect maps amount to octave-scaled hold frames") {
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(-1) == 1);
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(0x00) == 1);
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(0x20) == 2);
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(0x40) == 4);
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(0x80) == 16);
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(0xA0) == 32);
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(0xC0) == 64);
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(0xE0) == 128);
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(0xFF) == 256);
  CHECK(DownsampleInsertEffect::HoldFramesForAmount(0x100) == 256);

  int previous = DownsampleInsertEffect::HoldFramesForAmount(0);
  for (int amount = 1; amount <= 0xFF; ++amount) {
    const int current = DownsampleInsertEffect::HoldFramesForAmount(amount);
    CHECK(current >= previous);
    previous = current;
  }
}

TEST_CASE("DownsampleInsertEffect amount zero clears held state") {
  DownsampleInsertEffect effect;
  AudioSample heldBuffer[4] = {0.125f, -0.125f, 0.5f, -0.5f};
  AudioSample dryBuffer[4] = {0.25f, -0.25f, 0.75f, -0.75f};

  effect.Render(heldBuffer, 2, 0xFF, AudioBufferLayout::StereoInterleaved);
  effect.Render(dryBuffer, 2, 0, AudioBufferLayout::StereoInterleaved);

  CHECK(dryBuffer[0] == doctest::Approx(0.25f));
  CHECK(dryBuffer[1] == doctest::Approx(-0.25f));
  CHECK(dryBuffer[2] == doctest::Approx(0.75f));
  CHECK(dryBuffer[3] == doctest::Approx(-0.75f));
}

TEST_CASE("ClipInsertEffect amount zero leaves sub-threshold audio alone") {
  ClipInsertEffect effect;
  AudioSample buffer[4] = {0.25f, -0.25f, 0.5f, -0.5f};

  effect.Render(buffer, 2, 0, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(0.25f));
  CHECK(buffer[1] == doctest::Approx(-0.25f));
  CHECK(buffer[2] == doctest::Approx(0.5f));
  CHECK(buffer[3] == doctest::Approx(-0.5f));
}

TEST_CASE("ClipInsertEffect high amount clamps the waveform") {
  ClipInsertEffect effect;
  AudioSample buffer[4] = {0.8f, -0.8f, 0.25f, -0.25f};

  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(expectedClipSample(0.8f, 0xFF)));
  CHECK(buffer[1] == doctest::Approx(expectedClipSample(-0.8f, 0xFF)));
  CHECK(buffer[2] == doctest::Approx(expectedClipSample(0.25f, 0xFF)));
  CHECK(buffer[3] == doctest::Approx(expectedClipSample(-0.25f, 0xFF)));
}

TEST_CASE("SlewInsertEffect high amount limits fast steps") {
  SlewInsertEffect effect;
  AudioSample buffer[4] = {0.0f, 0.0f, 1.0f, -1.0f};

  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(0.0f));
  CHECK(buffer[1] == doctest::Approx(0.0f));
  CHECK(buffer[2] == doctest::Approx(expectedSlewStep(0xFF)));
  CHECK(buffer[3] == doctest::Approx(-expectedSlewStep(0xFF)));
}

TEST_CASE("SlewInsertEffect upper amounts use the extended ramp range") {
  SlewInsertEffect effect;
  AudioSample buffer[2] = {0.0f, 1.0f};

  effect.Render(buffer, 2, 0xC0, AudioBufferLayout::MonoPacked);

  CHECK(buffer[0] == doctest::Approx(0.0f));
  CHECK(buffer[1] == doctest::Approx(expectedSlewStep(0xC0)));
}

TEST_CASE("RectifyInsertEffect amount zero preserves the source") {
  RectifyInsertEffect effect;
  AudioSample buffer[4] = {0.25f, -0.5f, 0.75f, -0.125f};

  effect.Render(buffer, 2, 0, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(0.25f));
  CHECK(buffer[1] == doctest::Approx(-0.5f));
  CHECK(buffer[2] == doctest::Approx(0.75f));
  CHECK(buffer[3] == doctest::Approx(-0.125f));
}

TEST_CASE("RectifyInsertEffect high amount flips the negative half-cycle") {
  RectifyInsertEffect effect;
  AudioSample buffer[4] = {0.25f, -0.5f, -0.75f, 0.125f};

  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(expectedRectifySample(0.25f, 0xFF)));
  CHECK(buffer[1] == doctest::Approx(expectedRectifySample(-0.5f, 0xFF)));
  CHECK(buffer[2] == doctest::Approx(expectedRectifySample(-0.75f, 0xFF)));
  CHECK(buffer[3] == doctest::Approx(expectedRectifySample(0.125f, 0xFF)));
}

TEST_CASE("DCOffsetInsertEffect center amount preserves the source") {
  DCOffsetInsertEffect effect;
  AudioSample buffer[4] = {0.25f, -0.5f, 0.75f, -0.125f};

  effect.Render(buffer, 2, 0x80, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(0.25f));
  CHECK(buffer[1] == doctest::Approx(-0.5f));
  CHECK(buffer[2] == doctest::Approx(0.75f));
  CHECK(buffer[3] == doctest::Approx(-0.125f));
}

TEST_CASE("DCOffsetInsertEffect high amount biases the waveform upward") {
  DCOffsetInsertEffect effect;
  AudioSample buffer[4] = {0.0f, -0.25f, 0.25f, -0.75f};

  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::StereoInterleaved);

  CHECK(buffer[0] == doctest::Approx(expectedDCOffsetSample(0.0f, 0xFF)));
  CHECK(buffer[1] == doctest::Approx(expectedDCOffsetSample(-0.25f, 0xFF)));
  CHECK(buffer[2] == doctest::Approx(expectedDCOffsetSample(0.25f, 0xFF)));
  CHECK(buffer[3] == doctest::Approx(expectedDCOffsetSample(-0.75f, 0xFF)));
}

TEST_CASE("OverdriveInsertEffect processes packed mono buffers") {
  OverdriveInsertEffect effect;
  AudioSample buffer[2] = {0.4f, -0.6f};

  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::MonoPacked);

  CHECK(buffer[0] == doctest::Approx(expectedOverdriveSample(0.4f, 0xFF)));
  CHECK(buffer[1] == doctest::Approx(expectedOverdriveSample(-0.6f, 0xFF)));
}

TEST_CASE("DownsampleInsertEffect holds packed mono samples between frames") {
  DownsampleInsertEffect effect;
  AudioSample buffer[2] = {0.125f, 0.5f};

  effect.Render(buffer, 2, 0xFF, AudioBufferLayout::MonoPacked);

  CHECK(buffer[0] == doctest::Approx(0.125f));
  CHECK(buffer[1] == doctest::Approx(buffer[0]));
}

TEST_CASE("Voice insert registry clamps unavailable effects and maps slots") {
  CHECK(kDefaultVoiceInsertAmount == 0);
  CHECK(kVoiceInsertPositionCount == 2);
  CHECK(kDefaultVoiceInsertPosition == kVoiceInsertPositionPre);
  CHECK(ClampVoiceInsertEffectIndex(-1) == kDefaultVoiceInsertEffectIndex);
  CHECK(ClampVoiceInsertEffectIndex(kVoiceInsertEffectCount) ==
        kDefaultVoiceInsertEffectIndex);
  CHECK(ClampVoiceInsertEffectIndex(kVoiceInsertEffectOverdrive) ==
        kVoiceInsertEffectOverdrive);
  for (int i = 0; i < kVoiceInsertEffectCount; ++i) {
    CHECK(doctest::String(kVoiceInsertEffects[i].name_) !=
          doctest::String("detune"));
  }
  CHECK(ClampVoiceInsertPositionIndex(-1) == kDefaultVoiceInsertPosition);
  CHECK(ClampVoiceInsertPositionIndex(99) == kDefaultVoiceInsertPosition);
  CHECK(doctest::String(GetVoiceInsertTypeFourCC(0).c_str()) ==
        doctest::String(FourCC(FourCC::VoiceInsertType).c_str()));
  CHECK(doctest::String(GetVoiceInsertTypeFourCC(1).c_str()) ==
        doctest::String(FourCC(FourCC::VoiceInsertType2).c_str()));
  CHECK(doctest::String(GetVoiceInsertAmountFourCC(0).c_str()) ==
        doctest::String(FourCC(FourCC::VoiceInsertAmount).c_str()));
  CHECK(doctest::String(GetVoiceInsertAmountFourCC(1).c_str()) ==
        doctest::String(FourCC(FourCC::VoiceInsertAmount2).c_str()));
  CHECK(doctest::String(GetVoiceInsertPositionFourCC(0).c_str()) ==
        doctest::String(FourCC(FourCC::VoiceInsertPosition).c_str()));
  CHECK(doctest::String(GetVoiceInsertPositionFourCC(1).c_str()) ==
        doctest::String(FourCC(FourCC::VoiceInsertPosition2).c_str()));
  CHECK(doctest::String(GetVoiceInsertEffectInfo(kDefaultVoiceInsertEffectIndex)
                            .displayName_) == doctest::String("none"));
  CHECK(doctest::String(GetVoiceInsertEffectInfo(kVoiceInsertEffectOverdrive)
                            .displayName_) == doctest::String("odrv"));
  CHECK(doctest::String(
            GetVoiceInsertEffectInfo(kVoiceInsertEffectOverdrive).name_) ==
        doctest::String("overdrive"));
  CHECK(doctest::String(GetVoiceInsertEffectInfo(kVoiceInsertEffectBitcrush)
                            .displayName_) == doctest::String("crsh"));
  CHECK(
      doctest::String(GetVoiceInsertPositionNames()[kVoiceInsertPositionPre]) ==
      doctest::String("pre"));
  CHECK(doctest::String(
            GetVoiceInsertPositionNames()[kVoiceInsertPositionPost]) ==
        doctest::String("post"));
}
