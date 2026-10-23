/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OUTPUT_EFFECT_CONSTANTS_H_
#define _OUTPUT_EFFECT_CONSTANTS_H_

#include <cstddef>
#include <cstdint>

static constexpr int kOutputEffectSlot1Index = 0;
static constexpr int kOutputEffectSlot2Index = 1;
static constexpr int kOutputEffectDelayIndex = 2;
static constexpr int kOutputEffectReverbIndex = 3;
static constexpr int kOutputEffectSlot3Index = kOutputEffectDelayIndex;
static constexpr int kOutputEffectSlot4Index = kOutputEffectReverbIndex;
static constexpr int OUTPUT_EFFECT_COUNT = kOutputEffectSlot4Index + 1;
constexpr uint8_t OutputEffectSendMaximum = 0xFF;
constexpr uint8_t OutputEffectReturnMaximum = 0xFF;

constexpr int ClampOutputEffectSlotIndex(int slot) {
  if (slot < kOutputEffectSlot1Index) {
    return kOutputEffectSlot1Index;
  }
  if (slot >= OUTPUT_EFFECT_COUNT) {
    return OUTPUT_EFFECT_COUNT - 1;
  }
  return slot;
}

#endif
