/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _PITCH_OFFSET_H_
#define _PITCH_OFFSET_H_

#include <cstdint>

namespace PitchOffset {

constexpr int32_t CentsPerSemitone = 100;
constexpr int32_t CentsPerOctave = 12 * CentsPerSemitone;
constexpr int32_t LimitSemitones = 48;
constexpr int32_t LimitCents = LimitSemitones * CentsPerSemitone;
constexpr int32_t MinimumCents = -LimitCents;
constexpr int32_t MaximumCents = LimitCents;

constexpr int32_t Clamp(int32_t cents) {
  return cents < MinimumCents ? MinimumCents
                              : (cents > MaximumCents ? MaximumCents : cents);
}

} // namespace PitchOffset

#endif
