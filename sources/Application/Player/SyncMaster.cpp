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

#include "SyncMaster.h"
#include "Services/Audio/Audio.h"

#define AUDIO_SLICES_PER_STEP 6 // needs to be a multiple of 6 !

SyncMaster::SyncMaster() = default;

void SyncMaster::Start() {
  currentSlice_ = 0;
  beatCount_ = 0;
};

void SyncMaster::Stop(){};

void SyncMaster::SetTempo(int tempo) {
  tempo_ = tempo;
  int driverRate = Audio::GetInstance()->GetSampleRate();
  playSampleCount_ =
      60.0f * driverRate * 2.0f / tempo_ / 8.0f / float(AUDIO_SLICES_PER_STEP);
  tickSampleCount_ =
      60.0f * driverRate * 2.0f / tempo_ / 8.0f / float(AUDIO_SLICES_PER_STEP);
};

int SyncMaster::GetTempo() { return tempo_; };

void SyncMaster::NextSlice() {
  currentSlice_ = (currentSlice_ + 1) % AUDIO_SLICES_PER_STEP;
  if (currentSlice_ == 0) {
    beatCount_++;
  };
};

bool SyncMaster::MajorSlice() { return currentSlice_ == 0; };

bool SyncMaster::TableSlice() { return MidiSlice(); };

bool SyncMaster::MidiSlice() {
  int midiTick = currentSlice_ % (AUDIO_SLICES_PER_STEP / 6);
  return midiTick == 0;
};

// Returns the number of samples per play slice

float SyncMaster::GetPlaySampleCount() { return playSampleCount_; };

// Returns the number of sample per tick
float SyncMaster::GetTickSampleCount() { return tickSampleCount_; };

// xx samples per tick
// xx/driverRate seconds
// xx/driverRate*1000 msecs

float SyncMaster::GetTickTime() {
  return 60.0f * 2.0f / tempo_ / 8.0f / AUDIO_SLICES_PER_STEP * 1000.0f;
};

unsigned int SyncMaster::GetBeatCount() { return beatCount_; };
