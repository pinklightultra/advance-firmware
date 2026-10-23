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

#ifndef _SYNC_MASTER_H_
#define _SYNC_MASTER_H_

#include "Foundation/T_Singleton.h"

// Provide basic functionalities to compute various
// setting regarding tempo, buffer sizes, ticks

class SyncMaster : public T_Singleton<SyncMaster> {
public:
  // In theory, this could be updated in the future in order to make rendering
  // intervals smaller, if done, it must remain a multiple of 6 so table and
  // MIDI slices stay aligned.
  // In practice, we should probably never chnage this
  static constexpr int SlicesPerStep = 6;

  SyncMaster();
  void Start();
  void Stop();
  void SetTempo(int tempo);
  int GetTempo();
  void NextSlice();
  bool MajorSlice();
  bool TableSlice();
  bool MidiSlice();
  float GetPlaySampleCount();
  float GetTickSampleCount();
  float GetStepSampleCount();
  unsigned int GetBeatCount();
  float GetTickTime();

private:
  int tempo_;
  int currentSlice_;
  unsigned int beatCount_;
  float playSampleCount_;
  float tickSampleCount_;
};
#endif
