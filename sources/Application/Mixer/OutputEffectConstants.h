/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OUTPUT_EFFECT_CONSTANTS_H_
#define _OUTPUT_EFFECT_CONSTANTS_H_

static constexpr int kReverbEffectIndex = 0;
static constexpr int kDelayEffectIndex = 1;
static constexpr int kReservedEffectIndex = 2;
static constexpr int OUTPUT_EFFECT_COUNT = kReservedEffectIndex + 1;

#endif
