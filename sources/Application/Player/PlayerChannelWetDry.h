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
#include "config/MemorySections.h"
#include <stdint.h>

namespace PlayerChannelWetDry {

// This function traverses every rendered sample and must remain in ITCM.
PICOTRACKER_FAST_CODE("player_channel_wet_dry")
__attribute__((noinline)) inline void
ApplyWetDryRouting(AudioSample *__restrict__ buffer, int count,
                   AudioSample dryGain, AudioSample *__restrict__ send0,
                   AudioSample sendGain0, AudioSample *__restrict__ send1,
                   AudioSample sendGain1, AudioSample *__restrict__ send2,
                   AudioSample sendGain2, AudioSample *__restrict__ send3,
                   AudioSample sendGain3) {
  for (int i = 0; i < count; ++i) {
    const AudioSample source = buffer[i];
    buffer[i] = source * dryGain;
    send0[i] += source * sendGain0;
    send1[i] += source * sendGain1;
    send2[i] += source * sendGain2;
    send3[i] += source * sendGain3;
  }
}

} // namespace PlayerChannelWetDry

#endif
