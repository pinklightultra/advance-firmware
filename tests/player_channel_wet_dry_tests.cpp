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
                    AudioSample *send2, AudioSample sendGain2) {
  for (int i = 0; i < count; ++i) {
    const AudioSample source = buffer[i];
    buffer[i] = source * dryGain;
    if (send0) {
      send0[i] += source * sendGain0;
    }
    if (send1) {
      send1[i] += source * sendGain1;
    }
    if (send2) {
      send2[i] += source * sendGain2;
    }
  }
}

void CheckBuffersEqual(const AudioSample *actual, const AudioSample *expected,
                       int count) {
  for (int i = 0; i < count; ++i) {
    CHECK(actual[i] == doctest::Approx(expected[i]));
  }
}

} // namespace

TEST_CASE("PlayerChannelWetDry fast path leaves unity dry buffer untouched") {
  AudioSample actual[kSampleCount];
  AudioSample expected[kSampleCount];

  FillBuffer(actual, kSampleCount, 0.5f);
  FillBuffer(expected, kSampleCount, 0.5f);

  PlayerChannelWetDry::ApplyWetDryRouting(actual, kSampleCount, kAudioSampleOne,
                                          nullptr, 0.0f, nullptr, 0.0f, nullptr,
                                          0.0f);

  CheckBuffersEqual(actual, expected, kSampleCount);
}

TEST_CASE("PlayerChannelWetDry matches scalar routing for every send mask") {
  constexpr AudioSample kDryGains[] = {kAudioSampleOne, 0.625f};

  for (AudioSample dryGain : kDryGains) {
    for (uint8_t sendMask = 0; sendMask < 8; ++sendMask) {
      AudioSample actual[kSampleCount];
      AudioSample expected[kSampleCount];
      AudioSample actualSend0[kSampleCount];
      AudioSample actualSend1[kSampleCount];
      AudioSample actualSend2[kSampleCount];
      AudioSample expectedSend0[kSampleCount];
      AudioSample expectedSend1[kSampleCount];
      AudioSample expectedSend2[kSampleCount];

      FillBuffer(actual, kSampleCount, -1.0f);
      FillBuffer(expected, kSampleCount, -1.0f);
      FillBuffer(actualSend0, kSampleCount, 2.0f);
      FillBuffer(actualSend1, kSampleCount, 3.0f);
      FillBuffer(actualSend2, kSampleCount, 4.0f);
      FillBuffer(expectedSend0, kSampleCount, 2.0f);
      FillBuffer(expectedSend1, kSampleCount, 3.0f);
      FillBuffer(expectedSend2, kSampleCount, 4.0f);

      AudioSample *actual0 =
          (sendMask & PlayerChannelWetDry::kSendMask0) ? actualSend0 : nullptr;
      AudioSample *actual1 =
          (sendMask & PlayerChannelWetDry::kSendMask1) ? actualSend1 : nullptr;
      AudioSample *actual2 =
          (sendMask & PlayerChannelWetDry::kSendMask2) ? actualSend2 : nullptr;
      AudioSample *expected0 = (sendMask & PlayerChannelWetDry::kSendMask0)
                                   ? expectedSend0
                                   : nullptr;
      AudioSample *expected1 = (sendMask & PlayerChannelWetDry::kSendMask1)
                                   ? expectedSend1
                                   : nullptr;
      AudioSample *expected2 = (sendMask & PlayerChannelWetDry::kSendMask2)
                                   ? expectedSend2
                                   : nullptr;

      PlayerChannelWetDry::ApplyWetDryRouting(actual, kSampleCount, dryGain,
                                              actual0, 0.125f, actual1, 0.25f,
                                              actual2, 0.5f);
      ApplyReference(expected, kSampleCount, dryGain, expected0, 0.125f,
                     expected1, 0.25f, expected2, 0.5f);

      CheckBuffersEqual(actual, expected, kSampleCount);
      CheckBuffersEqual(actualSend0, expectedSend0, kSampleCount);
      CheckBuffersEqual(actualSend1, expectedSend1, kSampleCount);
      CheckBuffersEqual(actualSend2, expectedSend2, kSampleCount);
    }
  }
}
