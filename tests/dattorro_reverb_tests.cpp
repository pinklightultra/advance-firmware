/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Effects/DattorroReverb.h"
#include "Application/Effects/DattorroReverbOutputEffect.h"
#include "output_effect_test_support.h"

#include <climits>
#include <cmath>

namespace {

constexpr int32_t kRenderFrameCount = 8192;

struct DattorroTestBuffers {
  alignas(32) uint8_t internalA[DattorroReverb::kInternalMemorySize];
  alignas(32) uint8_t externalA[DattorroReverb::kExternalMemorySize];
  alignas(32) uint8_t internalB[DattorroReverb::kInternalMemorySize];
  alignas(32) uint8_t externalB[DattorroReverb::kExternalMemorySize];
  AudioSample input[kRenderFrameCount * 2];
  AudioSample outputA[kRenderFrameCount * 2];
  AudioSample outputB[kRenderFrameCount * 2];
};

DattorroTestBuffers &GetDattorroTestBuffers() {
  static DattorroTestBuffers buffers;
  return buffers;
}

void ClearAudioBuffers(DattorroTestBuffers &buffers) {
  for (int32_t sample = 0; sample < kRenderFrameCount * 2; ++sample) {
    buffers.input[sample] = 0.0f;
    buffers.outputA[sample] = 0.0f;
    buffers.outputB[sample] = 0.0f;
  }
}

} // namespace

using namespace OutputEffectTestSupport;

TEST_CASE("Dattorro settings clamp to their stored ranges") {
  DattorroTestBuffers &buffers = GetDattorroTestBuffers();
  ClearAudioBuffers(buffers);
  buffers.input[0] = 0.5f;
  buffers.input[1] = 0.5f;

  DattorroReverbSettings outsideRange;
  outsideRange.preDelay_ = -1;
  outsideRange.inputTone_ = 1000;
  outsideRange.preDiffusion1_ = -1;
  outsideRange.preDiffusion2_ = 1000;
  outsideRange.decay_ = -1;
  outsideRange.decayDiffusion_ = 1000;
  outsideRange.tailTone_ = -1;
  outsideRange.modulationRate_ = 1000;
  outsideRange.modulationDepth_ = -1;

  DattorroReverbSettings endpoints;
  endpoints.preDelay_ = DattorroReverbSettings::MinimumParameterValue;
  endpoints.inputTone_ = DattorroReverbSettings::MaximumParameterValue;
  endpoints.preDiffusion1_ = DattorroReverbSettings::MinimumDiffusion;
  endpoints.preDiffusion2_ = DattorroReverbSettings::MaximumDiffusion;
  endpoints.decay_ = DattorroReverbSettings::MinimumParameterValue;
  endpoints.decayDiffusion_ = DattorroReverbSettings::MaximumDiffusion;
  endpoints.tailTone_ = DattorroReverbSettings::MinimumParameterValue;
  endpoints.modulationRate_ = DattorroReverbSettings::MaximumParameterValue;
  endpoints.modulationDepth_ = DattorroReverbSettings::MinimumParameterValue;

  DattorroReverb clamped;
  DattorroReverb reference;
  clamped.Configure(outsideRange);
  reference.Configure(endpoints);
  REQUIRE(clamped.Initialize(
      buffers.internalA, DattorroReverb::kInternalMemorySize, buffers.externalA,
      DattorroReverb::kExternalMemorySize));
  REQUIRE(reference.Initialize(
      buffers.internalB, DattorroReverb::kInternalMemorySize, buffers.externalB,
      DattorroReverb::kExternalMemorySize));

  clamped.Render(buffers.input, buffers.outputA, kRenderFrameCount);
  reference.Render(buffers.input, buffers.outputB, kRenderFrameCount);
  for (int32_t sample = 0; sample < kRenderFrameCount * 2; ++sample) {
    CHECK(buffers.outputA[sample] == buffers.outputB[sample]);
  }
  CHECK(clamped.GetTailFrameCount() == reference.GetTailFrameCount());
}

TEST_CASE("Reverb diffuse macro sets the canonical diffusion ratios") {
  DattorroReverbOutputEffect effect;
  Variable *diffuse = effect.FindVariable(FourCC::VarReverbDiffuse);
  Variable *preDiffusion1 = effect.FindVariable(FourCC::VarReverbPreDiffusion1);
  Variable *preDiffusion2 = effect.FindVariable(FourCC::VarReverbPreDiffusion2);
  Variable *decayDiffusion =
      effect.FindVariable(FourCC::VarReverbDecayDiffusion);
  REQUIRE(diffuse != nullptr);
  REQUIRE(preDiffusion1 != nullptr);
  REQUIRE(preDiffusion2 != nullptr);
  REQUIRE(decayDiffusion != nullptr);
  CHECK(diffuse->GetInt() == DattorroReverbOutputEffect::DefaultDiffuse);

  diffuse->SetInt(0);
  CHECK(preDiffusion1->GetInt() == 0);
  CHECK(preDiffusion2->GetInt() == 0);
  CHECK(decayDiffusion->GetInt() == 0);

  diffuse->SetInt(0x5A);
  CHECK(preDiffusion1->GetInt() == 375);
  CHECK(preDiffusion2->GetInt() == 313);
  CHECK(decayDiffusion->GetInt() == 350);

  diffuse->SetInt(DattorroReverbOutputEffect::DefaultDiffuse);
  CHECK(preDiffusion1->GetInt() ==
        DattorroReverbOutputEffect::DefaultPreDiffusion1);
  CHECK(preDiffusion2->GetInt() ==
        DattorroReverbOutputEffect::DefaultPreDiffusion2);
  CHECK(decayDiffusion->GetInt() ==
        DattorroReverbOutputEffect::DefaultDecayDiffusion);

  diffuse->SetInt(DattorroReverbOutputEffect::MaximumParameterValue);
  CHECK(preDiffusion1->GetInt() ==
        DattorroReverbOutputEffect::MaximumDiffusion);
  CHECK(preDiffusion2->GetInt() ==
        DattorroReverbOutputEffect::MaximumDiffusion);
  CHECK(decayDiffusion->GetInt() ==
        DattorroReverbOutputEffect::MaximumDiffusion);
}

TEST_CASE("Reverb diffusion components remain independently editable") {
  DattorroReverbOutputEffect effect;
  Variable *diffuse = effect.FindVariable(FourCC::VarReverbDiffuse);
  Variable *preDiffusion1 = effect.FindVariable(FourCC::VarReverbPreDiffusion1);
  Variable *preDiffusion2 = effect.FindVariable(FourCC::VarReverbPreDiffusion2);
  Variable *decayDiffusion =
      effect.FindVariable(FourCC::VarReverbDecayDiffusion);
  REQUIRE(diffuse != nullptr);
  REQUIRE(preDiffusion1 != nullptr);
  REQUIRE(preDiffusion2 != nullptr);
  REQUIRE(decayDiffusion != nullptr);

  diffuse->SetInt(0x5A);
  preDiffusion1->SetInt(900);
  preDiffusion2->SetInt(800);
  decayDiffusion->SetInt(450);

  CHECK(diffuse->GetInt() == 0x5A);
  CHECK(preDiffusion1->GetInt() == 900);
  CHECK(preDiffusion2->GetInt() == 800);
  CHECK(decayDiffusion->GetInt() == 450);
}

TEST_CASE("Reverb tail duration follows decay and supports hold") {
  DattorroReverbOutputEffect effect;
  Variable *preDelay = effect.FindVariable(FourCC::VarReverbPreDelay);
  Variable *decay = effect.FindVariable(FourCC::VarReverbDecay);
  REQUIRE(preDelay != nullptr);
  REQUIRE(decay != nullptr);

  preDelay->SetInt(0xFF);
  decay->SetInt(0x00);
  effect.PrepareRender();
  CHECK(effect.GetTailFrameCount() == 92610);

  decay->SetInt(DattorroReverbOutputEffect::MaximumFiniteDecayParameterValue);
  effect.PrepareRender();
  CHECK(effect.GetTailFrameCount() != INT_MAX);

  decay->SetInt(DattorroReverbOutputEffect::MaximumParameterValue);
  effect.PrepareRender();
  CHECK(effect.GetTailFrameCount() == INT_MAX);
}

TEST_CASE("Dattorro requires both correctly sized memory arenas") {
  DattorroTestBuffers &buffers = GetDattorroTestBuffers();
  DattorroReverb reverb;

  CHECK_FALSE(reverb.Initialize(
      buffers.internalA, DattorroReverb::kInternalMemorySize - 1,
      buffers.externalA, DattorroReverb::kExternalMemorySize));
  CHECK_FALSE(reverb.Initialize(
      buffers.internalA, DattorroReverb::kInternalMemorySize, buffers.externalA,
      DattorroReverb::kExternalMemorySize - 1));
  CHECK(reverb.Initialize(
      buffers.internalA, DattorroReverb::kInternalMemorySize, buffers.externalA,
      DattorroReverb::kExternalMemorySize));
}

TEST_CASE("Dattorro rendering is independent of render block size") {
  DattorroTestBuffers &buffers = GetDattorroTestBuffers();
  ClearAudioBuffers(buffers);
  buffers.input[0] = 0.5f;
  buffers.input[1] = 0.5f;

  DattorroReverb fullBlock;
  DattorroReverb splitBlocks;
  REQUIRE(fullBlock.Initialize(
      buffers.internalA, DattorroReverb::kInternalMemorySize, buffers.externalA,
      DattorroReverb::kExternalMemorySize));
  REQUIRE(splitBlocks.Initialize(
      buffers.internalB, DattorroReverb::kInternalMemorySize, buffers.externalB,
      DattorroReverb::kExternalMemorySize));

  fullBlock.Render(buffers.input, buffers.outputA, kRenderFrameCount);
  constexpr int32_t kFirstBlockFrames = 731;
  splitBlocks.Render(buffers.input, buffers.outputB, kFirstBlockFrames);
  splitBlocks.Render(buffers.input + kFirstBlockFrames * 2,
                     buffers.outputB + kFirstBlockFrames * 2,
                     kRenderFrameCount - kFirstBlockFrames);

  for (int32_t sample = 0; sample < kRenderFrameCount * 2; ++sample) {
    CHECK(buffers.outputA[sample] == buffers.outputB[sample]);
  }
}

TEST_CASE("Dattorro output contains no direct input signal") {
  DattorroTestBuffers &buffers = GetDattorroTestBuffers();
  ClearAudioBuffers(buffers);
  for (int32_t frame = 0; frame < 64; ++frame) {
    buffers.input[frame * 2] = static_cast<float>(frame) * 0.01f;
    buffers.input[frame * 2 + 1] = -static_cast<float>(frame) * 0.005f;
  }

  DattorroReverb reverb;
  REQUIRE(reverb.Initialize(
      buffers.internalA, DattorroReverb::kInternalMemorySize, buffers.externalA,
      DattorroReverb::kExternalMemorySize));
  reverb.Render(buffers.input, buffers.outputA, 64);

  for (int32_t sample = 0; sample < 128; ++sample) {
    CHECK(buffers.outputA[sample] == 0.0f);
  }
}

TEST_CASE("Dattorro configuration applies before the next render block") {
  DattorroTestBuffers &buffers = GetDattorroTestBuffers();
  ClearAudioBuffers(buffers);
  buffers.input[0] = 0.75f;
  buffers.input[1] = 0.75f;

  DattorroReverb edited;
  DattorroReverb reference;
  REQUIRE(edited.Initialize(
      buffers.internalA, DattorroReverb::kInternalMemorySize, buffers.externalA,
      DattorroReverb::kExternalMemorySize));
  REQUIRE(reference.Initialize(
      buffers.internalB, DattorroReverb::kInternalMemorySize, buffers.externalB,
      DattorroReverb::kExternalMemorySize));

  DattorroReverbSettings settings;
  settings.preDelay_ = DattorroReverbSettings::MaximumParameterValue;
  edited.Configure(settings);
  edited.Render(buffers.input, buffers.outputA, 2048);
  reference.Render(buffers.input, buffers.outputB, 2048);

  float referenceEnergy = 0.0f;
  for (int32_t sample = 0; sample < 4096; ++sample) {
    CHECK(buffers.outputA[sample] == 0.0f);
    referenceEnergy += buffers.outputB[sample] * buffers.outputB[sample];
  }
  CHECK(referenceEnergy > 0.0f);
}

TEST_CASE("Dattorro wet impulse response is finite and stereo decorrelated") {
  DattorroTestBuffers &buffers = GetDattorroTestBuffers();
  ClearAudioBuffers(buffers);
  buffers.input[0] = 0.5f;
  buffers.input[1] = 0.5f;

  DattorroReverbSettings settings;
  settings.modulationDepth_ = DattorroReverbSettings::MinimumParameterValue;
  DattorroReverb reverb;
  reverb.Configure(settings);
  REQUIRE(reverb.Initialize(
      buffers.internalA, DattorroReverb::kInternalMemorySize, buffers.externalA,
      DattorroReverb::kExternalMemorySize));
  reverb.Render(buffers.input, buffers.outputA, kRenderFrameCount);

  float wetEnergy = 0.0f;
  float stereoDifferenceEnergy = 0.0f;
  for (int32_t frame = 0; frame < kRenderFrameCount; ++frame) {
    const float left = buffers.outputA[frame * 2];
    const float right = buffers.outputA[frame * 2 + 1];
    CHECK(std::isfinite(left));
    CHECK(std::isfinite(right));
    wetEnergy += left * left + right * right;
    const float difference = left - right;
    stereoDifferenceEnergy += difference * difference;
  }

  CHECK(wetEnergy > 0.0f);
  CHECK(stereoDifferenceEnergy > 0.0f);

  // These checkpoints were generated by DattorroReverbNode at 44.1 kHz with
  // the same wet-only, zero-excursion impulse. They guard the translated delay
  // lengths, allpass signs, feedback routing, and stereo output taps.
  CHECK(buffers.outputA[1222 * 2] == doctest::Approx(0.0f));
  CHECK(buffers.outputA[1222 * 2 + 1] ==
        doctest::Approx(-0.04613796622f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[4000 * 2] ==
        doctest::Approx(0.0000008994130667f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[4000 * 2 + 1] ==
        doctest::Approx(0.0003729825257f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[5000 * 2] ==
        doctest::Approx(-0.01099392213f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[5000 * 2 + 1] ==
        doctest::Approx(0.009221153334f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[7000 * 2] ==
        doctest::Approx(-0.005123099312f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[7000 * 2 + 1] ==
        doctest::Approx(0.001113400445f).epsilon(0.000001).scale(1.0));
}

TEST_CASE("Dattorro excursion follows the AudioWorklet reference response") {
  DattorroTestBuffers &buffers = GetDattorroTestBuffers();
  ClearAudioBuffers(buffers);
  buffers.input[0] = 0.5f;
  buffers.input[1] = 0.5f;

  DattorroReverbSettings settings;
  DattorroReverb reverb;
  reverb.Configure(settings);
  REQUIRE(reverb.Initialize(
      buffers.internalA, DattorroReverb::kInternalMemorySize, buffers.externalA,
      DattorroReverb::kExternalMemorySize));
  reverb.Render(buffers.input, buffers.outputA, kRenderFrameCount);

  // DattorroReverbNode checkpoints using its default 0.5 Hz rate and 0.7 ms
  // excursion depth. The tolerance allows the recursive oscillator's tiny
  // difference from direct trigonometry while still detecting a different
  // waveform, rate, depth, or interpolation path.
  CHECK(buffers.outputA[4000 * 2] ==
        doctest::Approx(0.0000008994130667f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[4000 * 2 + 1] ==
        doctest::Approx(-0.0001088287245f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[5000 * 2] ==
        doctest::Approx(-0.006198561285f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[5000 * 2 + 1] ==
        doctest::Approx(-0.001681028516f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[7000 * 2] ==
        doctest::Approx(-0.002461593365f).epsilon(0.000001).scale(1.0));
  CHECK(buffers.outputA[7000 * 2 + 1] ==
        doctest::Approx(0.006905827671f).epsilon(0.000001).scale(1.0));
}

TEST_CASE("Dattorro reverb outputs silence for silent input") {
  CheckSilentInputProducesSilence<DattorroReverbOutputEffect>();
}

TEST_CASE("Dattorro reverb declares its tail length") {
  CheckDeclaresTailLength<DattorroReverbOutputEffect>();
}

TEST_CASE("Dattorro reverb only writes the requested stereo frames") {
  CheckOnlyRequestedFramesAreWritten<DattorroReverbOutputEffect>();
}

TEST_CASE("Dattorro reverb returns output for audible input") {
  CheckAudibleInputProducesWetOutput<DattorroReverbOutputEffect>();
}
