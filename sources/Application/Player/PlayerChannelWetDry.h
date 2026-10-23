/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _PLAYER_CHANNEL_WET_DRY_H_
#define _PLAYER_CHANNEL_WET_DRY_H_

#include "Application/Audio/AudioSample.h"
#include <stdint.h>

namespace PlayerChannelWetDry {

constexpr uint8_t kSendMask0 = 1u << 0;
constexpr uint8_t kSendMask1 = 1u << 1;
constexpr uint8_t kSendMask2 = 1u << 2;

inline uint8_t BuildSendMask(const AudioSample *send0, const AudioSample *send1,
                             const AudioSample *send2) {
  uint8_t mask = 0;
  if (send0) {
    mask |= kSendMask0;
  }
  if (send1) {
    mask |= kSendMask1;
  }
  if (send2) {
    mask |= kSendMask2;
  }
  return mask;
}

template <bool kDryUnity, uint8_t kSendMask>
inline __attribute__((always_inline)) void
ProcessSample(AudioSample *__restrict__ buffer, int index, AudioSample dryGain,
              AudioSample *__restrict__ send0, AudioSample sendGain0,
              AudioSample *__restrict__ send1, AudioSample sendGain1,
              AudioSample *__restrict__ send2, AudioSample sendGain2) {
  const AudioSample source = buffer[index];
  if constexpr (!kDryUnity) {
    buffer[index] = source * dryGain;
  }
  if constexpr ((kSendMask & kSendMask0) != 0) {
    send0[index] += source * sendGain0;
  }
  if constexpr ((kSendMask & kSendMask1) != 0) {
    send1[index] += source * sendGain1;
  }
  if constexpr ((kSendMask & kSendMask2) != 0) {
    send2[index] += source * sendGain2;
  }
}

template <bool kDryUnity, uint8_t kSendMask>
inline void Apply(AudioSample *__restrict__ buffer, int count,
                  AudioSample dryGain, AudioSample *__restrict__ send0,
                  AudioSample sendGain0, AudioSample *__restrict__ send1,
                  AudioSample sendGain1, AudioSample *__restrict__ send2,
                  AudioSample sendGain2) {
  int i = 0;
  for (; i + 2 <= count; i += 2) {
    ProcessSample<kDryUnity, kSendMask>(buffer, i + 0, dryGain, send0,
                                        sendGain0, send1, sendGain1, send2,
                                        sendGain2);
    ProcessSample<kDryUnity, kSendMask>(buffer, i + 1, dryGain, send0,
                                        sendGain0, send1, sendGain1, send2,
                                        sendGain2);
  }

  for (; i < count; ++i) {
    ProcessSample<kDryUnity, kSendMask>(buffer, i, dryGain, send0, sendGain0,
                                        send1, sendGain1, send2, sendGain2);
  }
}

inline void
ApplyWetDryRouting(AudioSample *__restrict__ buffer, int count,
                   AudioSample dryGain, AudioSample *__restrict__ send0,
                   AudioSample sendGain0, AudioSample *__restrict__ send1,
                   AudioSample sendGain1, AudioSample *__restrict__ send2,
                   AudioSample sendGain2) {
  const uint8_t sendMask = BuildSendMask(send0, send1, send2);
  if (dryGain == kAudioSampleOne) {
    switch (sendMask) {
    case 0:
      return;
    case kSendMask0:
      Apply<true, kSendMask0>(buffer, count, dryGain, send0, sendGain0, send1,
                              sendGain1, send2, sendGain2);
      return;
    case kSendMask1:
      Apply<true, kSendMask1>(buffer, count, dryGain, send0, sendGain0, send1,
                              sendGain1, send2, sendGain2);
      return;
    case kSendMask2:
      Apply<true, kSendMask2>(buffer, count, dryGain, send0, sendGain0, send1,
                              sendGain1, send2, sendGain2);
      return;
    case kSendMask0 | kSendMask1:
      Apply<true, kSendMask0 | kSendMask1>(buffer, count, dryGain, send0,
                                           sendGain0, send1, sendGain1, send2,
                                           sendGain2);
      return;
    case kSendMask0 | kSendMask2:
      Apply<true, kSendMask0 | kSendMask2>(buffer, count, dryGain, send0,
                                           sendGain0, send1, sendGain1, send2,
                                           sendGain2);
      return;
    case kSendMask1 | kSendMask2:
      Apply<true, kSendMask1 | kSendMask2>(buffer, count, dryGain, send0,
                                           sendGain0, send1, sendGain1, send2,
                                           sendGain2);
      return;
    default:
      Apply<true, kSendMask0 | kSendMask1 | kSendMask2>(
          buffer, count, dryGain, send0, sendGain0, send1, sendGain1, send2,
          sendGain2);
      return;
    }
  }

  switch (sendMask) {
  case 0:
    Apply<false, 0>(buffer, count, dryGain, send0, sendGain0, send1, sendGain1,
                    send2, sendGain2);
    return;
  case kSendMask0:
    Apply<false, kSendMask0>(buffer, count, dryGain, send0, sendGain0, send1,
                             sendGain1, send2, sendGain2);
    return;
  case kSendMask1:
    Apply<false, kSendMask1>(buffer, count, dryGain, send0, sendGain0, send1,
                             sendGain1, send2, sendGain2);
    return;
  case kSendMask2:
    Apply<false, kSendMask2>(buffer, count, dryGain, send0, sendGain0, send1,
                             sendGain1, send2, sendGain2);
    return;
  case kSendMask0 | kSendMask1:
    Apply<false, kSendMask0 | kSendMask1>(buffer, count, dryGain, send0,
                                          sendGain0, send1, sendGain1, send2,
                                          sendGain2);
    return;
  case kSendMask0 | kSendMask2:
    Apply<false, kSendMask0 | kSendMask2>(buffer, count, dryGain, send0,
                                          sendGain0, send1, sendGain1, send2,
                                          sendGain2);
    return;
  case kSendMask1 | kSendMask2:
    Apply<false, kSendMask1 | kSendMask2>(buffer, count, dryGain, send0,
                                          sendGain0, send1, sendGain1, send2,
                                          sendGain2);
    return;
  default:
    Apply<false, kSendMask0 | kSendMask1 | kSendMask2>(
        buffer, count, dryGain, send0, sendGain0, send1, sendGain1, send2,
        sendGain2);
    return;
  }
}

} // namespace PlayerChannelWetDry

#endif
