/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#include "AudioOut.h"
#include "Application/Player/SyncMaster.h"
#include "config/MemorySections.h"

AudioOut::AudioOut() : AudioMixer("AudioOut"), sampleOffset_(0){};

AudioOut::~AudioOut(){};

PICOTRACKER_FAST_CODE("audio_out_get_play_sample_count")
int AudioOut::getPlaySampleCount() {
  sampleOffset_ += SyncMaster::GetInstance()->GetPlaySampleCount();
  int count = int(sampleOffset_);
  sampleOffset_ -= count;
  return count;
};
