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
#include "config/MemorySections.h"

SyncMaster::SyncMaster() = default;

void SyncMaster::Start() {
  currentSlice_ = 0;
  beatCount_ = 0;
};

void SyncMaster::Stop(){};

PICOTRACKER_FAST_CODE("sync_master_set_tempo")
void SyncMaster::SetTempo(int tempo) {
  tempo_ = tempo;
  int driverRate = Audio::GetInstance()->GetSampleRate();
  playSampleCount_ =
      60.0f * driverRate * 2.0f / tempo_ / 8.0f / float(SlicesPerStep);
  tickSampleCount_ =
      60.0f * driverRate * 2.0f / tempo_ / 8.0f / float(SlicesPerStep);
};

int SyncMaster::GetTempo() { return tempo_; };

PICOTRACKER_FAST_CODE("sync_master_next_slice")
void SyncMaster::NextSlice() {
  currentSlice_ = (currentSlice_ + 1) % SlicesPerStep;
  if (currentSlice_ == 0) {
    beatCount_++;
  };
};

bool SyncMaster::MajorSlice() { return currentSlice_ == 0; };

PICOTRACKER_FAST_CODE("sync_master_table_slice")
bool SyncMaster::TableSlice() { return MidiSlice(); };

PICOTRACKER_FAST_CODE("sync_master_midi_slice")
bool SyncMaster::MidiSlice() {
  int midiTick = currentSlice_ % (SlicesPerStep / 6);
  return midiTick == 0;
};

// Returns the number of samples per play slice

PICOTRACKER_FAST_CODE("sync_master_get_play_sample_count")
float SyncMaster::GetPlaySampleCount() { return playSampleCount_; };

// Returns the number of sample per tick
float SyncMaster::GetTickSampleCount() { return tickSampleCount_; };

// Returns the number of samples per sequencer step
float SyncMaster::GetStepSampleCount() {
  return tickSampleCount_ * SlicesPerStep;
}

// xx samples per tick
// xx/driverRate seconds
// xx/driverRate*1000 msecs

float SyncMaster::GetTickTime() {
  return 60.0f * 2.0f / tempo_ / 8.0f / SlicesPerStep * 1000.0f;
};

unsigned int SyncMaster::GetBeatCount() { return beatCount_; };
