/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _VOICE_ENVELOPE_GRAPH_H_
#define _VOICE_ENVELOPE_GRAPH_H_

#include <cstdint>

struct VoiceEnvelopeShapeValues {
  int32_t attack = 0;
  int32_t decay = 0;
  int32_t sustain = 0;
  int32_t release = 0;
};

struct VoiceEnvelopeGraphGeometry {
  int32_t axisY = 0;
  int32_t peakY = 0;
  int32_t sustainY = 0;
  int32_t attackX = 0;
  int32_t decayX = 0;
  int32_t sustainX = 0;
  int32_t releaseX = 0;
  int32_t releaseY = 0;
  bool releaseHold = false;
};

VoiceEnvelopeGraphGeometry
CalculateVoiceEnvelopeGraphGeometry(int32_t width, int32_t height,
                                    const VoiceEnvelopeShapeValues &values,
                                    int32_t signedDepth);

#endif
