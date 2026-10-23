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

#include "Phrase.h"
#include "Song.h"
#include "System/System/System.h"
#include <stdlib.h>
#include <string.h>

Phrase::Phrase() { Reset(); };

Phrase::~Phrase(){};

void Phrase::Reset() {
  for (int i = 0; i < PHRASE_COUNT * STEPS_PER_PHRASE; i++) {
    note_[i] = NO_NOTE;
    instr_[i] = 0xFF;
    cmd1_[i] = TrackerCommand::InstrumentCommandNone;
    param1_[i] = 0x00;
    cmd2_[i] = TrackerCommand::InstrumentCommandNone;
    param2_[i] = 0x00;
  }
  for (int i = 0; i < PHRASE_COUNT; i++) {
    isUsed_[i] = false;
  }
}

unsigned short Phrase::GetNext() {
  for (int i = 0; i < PHRASE_COUNT; i++) {
    if (!isUsed_[i]) {
      isUsed_[i] = true;
      return i;
    }
  }
  return NO_MORE_PHRASE;
};

uint16_t Phrase::GetNextFrom(uint8_t current) {
  for (uint16_t i = current; i < PHRASE_COUNT; i++) {
    if (!isUsed_[i]) {
      isUsed_[i] = true;
      return i;
    }
  }
  return NO_MORE_PHRASE;
};

void Phrase::SetUsed(unsigned char c) { isUsed_[c] = true; }

void Phrase::ClearAllocation() {

  for (int i = 0; i < PHRASE_COUNT; i++) {
    isUsed_[i] = false;
  }
};
