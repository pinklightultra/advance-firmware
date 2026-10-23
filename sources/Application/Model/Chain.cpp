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

#include "Chain.h"
#include "System/System/System.h"
#include <stdlib.h>
#include <string.h>

Chain::Chain() { Reset(); };

Chain::~Chain(){};

void Chain::Reset() {
  for (int i = 0; i < CHAIN_COUNT * PHRASES_PER_CHAIN; i++) {
    data_[i] = 0xFF;
    transpose_[i] = 0x00;
  }
  for (int i = 0; i < CHAIN_COUNT; i++) {
    isUsed_[i] = false;
  }
}

unsigned short Chain::GetNext() {
  for (int i = 0; i < CHAIN_COUNT; i++) {
    if (!isUsed_[i]) {
      isUsed_[i] = true;
      return i;
    }
  }
  return NO_MORE_CHAIN;
};

uint16_t Chain::GetNextFrom(uint8_t current) {
  for (uint16_t i = current; i < CHAIN_COUNT; i++) {
    if (!isUsed_[i]) {
      isUsed_[i] = true;
      return i;
    }
  }
  return NO_MORE_CHAIN;
};

void Chain::SetUsed(unsigned char c) { isUsed_[c] = true; }

void Chain::ClearAllocation() {

  for (int i = 0; i < CHAIN_COUNT; i++) {
    isUsed_[i] = false;
  }
}
