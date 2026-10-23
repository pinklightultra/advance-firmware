/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Audio/AudioSample.h"
#include "Application/Mixer/ReverbOutputEffect.h"

TEST_CASE("ReverbOutputEffect outputs silence for silent input") {
  ReverbOutputEffect effect;
  AudioSample input[8] = {};
  AudioSample output[8] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

  effect.Render(input, output, 4);

  CHECK_FALSE(effect.IsTailActive());
  CHECK(output[0] == doctest::Approx(0.0f));
  CHECK(output[1] == doctest::Approx(0.0f));
  CHECK(output[2] == doctest::Approx(0.0f));
  CHECK(output[3] == doctest::Approx(0.0f));
  CHECK(output[4] == doctest::Approx(0.0f));
  CHECK(output[5] == doctest::Approx(0.0f));
  CHECK(output[6] == doctest::Approx(0.0f));
  CHECK(output[7] == doctest::Approx(0.0f));
}

TEST_CASE("ReverbOutputEffect reports and resets an active tail") {
  ReverbOutputEffect effect;
  AudioSample input[2] = {1.0f, 1.0f};
  AudioSample output[2] = {};

  effect.Render(input, output, 1);
  CHECK(effect.IsTailActive());

  OutputEffectProcessor &processor = effect;
  processor.Reset();
  CHECK_FALSE(effect.IsTailActive());
}

TEST_CASE("ReverbOutputEffect only writes the requested stereo frames") {
  ReverbOutputEffect effect;
  AudioSample input[4] = {};
  AudioSample output[8] = {9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f};

  effect.Render(input, output, 2);

  CHECK(output[0] == doctest::Approx(0.0f));
  CHECK(output[1] == doctest::Approx(0.0f));
  CHECK(output[2] == doctest::Approx(0.0f));
  CHECK(output[3] == doctest::Approx(0.0f));
  CHECK(output[4] == doctest::Approx(5.0f));
  CHECK(output[5] == doctest::Approx(4.0f));
  CHECK(output[6] == doctest::Approx(3.0f));
  CHECK(output[7] == doctest::Approx(2.0f));
}

TEST_CASE(
    "ReverbOutputEffect returns engine-scaled wet output for audible input") {
  ReverbOutputEffect effect;
  constexpr int sampleCount = 256;
  AudioSample input[sampleCount * 2] = {};
  AudioSample output[sampleCount * 2] = {};

  input[0] = 12000.0f / kAudioPcmScale;
  input[1] = 12000.0f / kAudioPcmScale;

  bool gotAudibleWetOutput = false;
  for (int block = 0; block < 16 && !gotAudibleWetOutput; ++block) {
    effect.Render(input, output, sampleCount);

    for (AudioSample sample : output) {
      if (sample != 0.0f) {
        gotAudibleWetOutput = true;
        break;
      }
    }

    for (AudioSample &sample : input) {
      sample = 0.0f;
    }
  }

  CHECK(gotAudibleWetOutput);
}
