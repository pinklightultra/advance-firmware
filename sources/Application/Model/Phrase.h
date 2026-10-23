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

#ifndef _PHRASE_H_
#define _PHRASE_H_

#include "Foundation/Types/Types.h"

#define PHRASE_COUNT 0xFF
#define NO_MORE_PHRASE 0x100
#define STEPS_PER_PHRASE 16

class Phrase {
public:
  Phrase();
  ~Phrase();
  void Reset();
  unsigned short GetNext();
  uint16_t GetNextFrom(uint8_t current);
  bool IsUsed(uchar i) { return isUsed_[i]; };
  void SetUsed(uchar c);
  void ClearAllocation();

  uchar note_[PHRASE_COUNT * STEPS_PER_PHRASE];
  uchar instr_[PHRASE_COUNT * STEPS_PER_PHRASE];
  TrackerCommand cmd1_[PHRASE_COUNT * STEPS_PER_PHRASE];
  ushort param1_[PHRASE_COUNT * STEPS_PER_PHRASE];
  TrackerCommand cmd2_[PHRASE_COUNT * STEPS_PER_PHRASE];
  ushort param2_[PHRASE_COUNT * STEPS_PER_PHRASE];

private:
  bool isUsed_[PHRASE_COUNT];
};

#endif
