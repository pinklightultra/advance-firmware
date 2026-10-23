/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Audio/AudioSample.h"
#include "Application/Player/PlayerChannelWetDry.h"

namespace {

constexpr int kSampleCount = 13;

void FillBuffer(AudioSample *buffer, int count, AudioSample base) {
  for (int i = 0; i < count; ++i) {
    buffer[i] = base + static_cast<AudioSample>(i) * 0.25f;
  }
}

void ApplyReference(AudioSample *buffer, int count, AudioSample dryGain,
                    AudioSample *send0, AudioSample sendGain0,
                    AudioSample *send1, AudioSample sendGain1,
                    AudioSample *send2, AudioSample sendGain2,
                    AudioSample *send3, AudioSample sendGain3) {
  for (int i = 0; i < count; ++i) {
    const AudioSample source = buffer[i];
    buffer[i] = source * dryGain;
    send0[i] += source * sendGain0;
    send1[i] += source * sendGain1;
    send2[i] += source * sendGain2;
    send3[i] += source * sendGain3;
  }
}

void CheckBuffersEqual(const AudioSample *actual, const AudioSample *expected,
                       int count) {
  for (int i = 0; i < count; ++i) {
    CHECK(actual[i] == doctest::Approx(expected[i]));
  }
}

} // namespace

TEST_CASE("PlayerChannelWetDry applies silent sends at unity dry gain") {
  AudioSample actual[kSampleCount];
  AudioSample expected[kSampleCount];
  AudioSample actualSend0[kSampleCount];
  AudioSample actualSend1[kSampleCount];
  AudioSample actualSend2[kSampleCount];
  AudioSample actualSend3[kSampleCount];
  AudioSample expectedSend0[kSampleCount];
  AudioSample expectedSend1[kSampleCount];
  AudioSample expectedSend2[kSampleCount];
  AudioSample expectedSend3[kSampleCount];

  FillBuffer(actual, kSampleCount, 0.5f);
  FillBuffer(expected, kSampleCount, 0.5f);
  FillBuffer(actualSend0, kSampleCount, 2.0f);
  FillBuffer(actualSend1, kSampleCount, 3.0f);
  FillBuffer(actualSend2, kSampleCount, 4.0f);
  FillBuffer(actualSend3, kSampleCount, 5.0f);
  FillBuffer(expectedSend0, kSampleCount, 2.0f);
  FillBuffer(expectedSend1, kSampleCount, 3.0f);
  FillBuffer(expectedSend2, kSampleCount, 4.0f);
  FillBuffer(expectedSend3, kSampleCount, 5.0f);

  PlayerChannelWetDry::ApplyWetDryRouting(actual, kSampleCount, kAudioSampleOne,
                                          actualSend0, 0.0f, actualSend1, 0.0f,
                                          actualSend2, 0.0f, actualSend3, 0.0f);

  CheckBuffersEqual(actual, expected, kSampleCount);
  CheckBuffersEqual(actualSend0, expectedSend0, kSampleCount);
  CheckBuffersEqual(actualSend1, expectedSend1, kSampleCount);
  CheckBuffersEqual(actualSend2, expectedSend2, kSampleCount);
  CheckBuffersEqual(actualSend3, expectedSend3, kSampleCount);
}

TEST_CASE("PlayerChannelWetDry fixed routing matches scalar processing") {
  constexpr AudioSample kDryGains[] = {kAudioSampleOne, 0.625f};
  constexpr AudioSample kSendGains[][4] = {
      {0.0f, 0.0f, 0.0f, 0.0f},
      {0.125f, 0.25f, 0.5f, 0.75f},
  };

  for (AudioSample dryGain : kDryGains) {
    for (const auto &sendGains : kSendGains) {
      AudioSample actual[kSampleCount];
      AudioSample expected[kSampleCount];
      AudioSample actualSend0[kSampleCount];
      AudioSample actualSend1[kSampleCount];
      AudioSample actualSend2[kSampleCount];
      AudioSample actualSend3[kSampleCount];
      AudioSample expectedSend0[kSampleCount];
      AudioSample expectedSend1[kSampleCount];
      AudioSample expectedSend2[kSampleCount];
      AudioSample expectedSend3[kSampleCount];

      FillBuffer(actual, kSampleCount, -1.0f);
      FillBuffer(expected, kSampleCount, -1.0f);
      FillBuffer(actualSend0, kSampleCount, 2.0f);
      FillBuffer(actualSend1, kSampleCount, 3.0f);
      FillBuffer(actualSend2, kSampleCount, 4.0f);
      FillBuffer(actualSend3, kSampleCount, 5.0f);
      FillBuffer(expectedSend0, kSampleCount, 2.0f);
      FillBuffer(expectedSend1, kSampleCount, 3.0f);
      FillBuffer(expectedSend2, kSampleCount, 4.0f);
      FillBuffer(expectedSend3, kSampleCount, 5.0f);

      PlayerChannelWetDry::ApplyWetDryRouting(
          actual, kSampleCount, dryGain, actualSend0, sendGains[0], actualSend1,
          sendGains[1], actualSend2, sendGains[2], actualSend3, sendGains[3]);
      ApplyReference(expected, kSampleCount, dryGain, expectedSend0,
                     sendGains[0], expectedSend1, sendGains[1], expectedSend2,
                     sendGains[2], expectedSend3, sendGains[3]);

      CheckBuffersEqual(actual, expected, kSampleCount);
      CheckBuffersEqual(actualSend0, expectedSend0, kSampleCount);
      CheckBuffersEqual(actualSend1, expectedSend1, kSampleCount);
      CheckBuffersEqual(actualSend2, expectedSend2, kSampleCount);
      CheckBuffersEqual(actualSend3, expectedSend3, kSampleCount);
    }
  }
}
