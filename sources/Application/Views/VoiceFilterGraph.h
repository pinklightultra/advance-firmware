/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _VOICE_FILTER_GRAPH_H_
#define _VOICE_FILTER_GRAPH_H_

#include <cstdint>

struct VoiceFilterGraphGeometry {
  int32_t leftShoulderX = 0;
  int32_t cutoffX = 0;
  int32_t rightShoulderX = 0;
  int32_t leftY = 0;
  int32_t peakY = 0;
  int32_t rightY = 0;
};

VoiceFilterGraphGeometry
CalculateVoiceFilterGraphGeometry(int32_t width, int32_t height, int32_t cutoff,
                                  int32_t resonance, int32_t filterType);

#endif
