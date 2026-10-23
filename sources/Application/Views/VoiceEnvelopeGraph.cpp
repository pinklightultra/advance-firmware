/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "VoiceEnvelopeGraph.h"

#include <algorithm>
#include <cstdlib>

namespace {
constexpr int32_t kMaxEnvelopeValue = 0xFF;
constexpr int32_t kMaxFiniteReleaseValue = 0xFE;
} // namespace

VoiceEnvelopeGraphGeometry
CalculateVoiceEnvelopeGraphGeometry(int32_t width, int32_t height,
                                    const VoiceEnvelopeShapeValues &values,
                                    int32_t signedDepth) {
  const int32_t clampedWidth = std::max<int32_t>(width, 0);
  const int32_t maxY = std::max<int32_t>(height - 1, 0);
  const int32_t quarterWidth = clampedWidth / 4;
  const int32_t sustainX = (clampedWidth * 3) / 4;
  const int32_t releaseWidth = clampedWidth - sustainX;
  const int32_t clampedDepth =
      std::clamp<int32_t>(signedDepth, -kMaxEnvelopeValue, kMaxEnvelopeValue);
  const int32_t absDepth = std::abs(clampedDepth);
  const int32_t amplitude = (maxY * absDepth) / kMaxEnvelopeValue;
  const bool inverted = clampedDepth < 0;
  const int32_t axisY = inverted ? maxY - amplitude : maxY;
  const int32_t direction = inverted ? 1 : -1;
  const int32_t sustainAmplitude =
      (amplitude * std::clamp<int32_t>(values.sustain, 0, kMaxEnvelopeValue)) /
      kMaxEnvelopeValue;
  const int32_t attackX =
      (quarterWidth *
       std::clamp<int32_t>(values.attack, 0, kMaxEnvelopeValue)) /
      kMaxEnvelopeValue;
  const int32_t decayX =
      attackX +
      (quarterWidth * std::clamp<int32_t>(values.decay, 0, kMaxEnvelopeValue)) /
          kMaxEnvelopeValue;

  VoiceEnvelopeGraphGeometry geometry;
  geometry.axisY = axisY;
  geometry.peakY = std::clamp<int32_t>(axisY + direction * amplitude, 0, maxY);
  geometry.sustainY =
      std::clamp<int32_t>(axisY + direction * sustainAmplitude, 0, maxY);
  geometry.attackX = attackX;
  geometry.decayX = decayX;
  geometry.sustainX = sustainX;

  if (values.release >= kMaxEnvelopeValue) {
    geometry.releaseX = clampedWidth;
    geometry.releaseY = geometry.sustainY;
    geometry.releaseHold = true;
    return geometry;
  }

  const int32_t clampedRelease =
      std::clamp<int32_t>(values.release, 0, kMaxFiniteReleaseValue);
  geometry.releaseX =
      sustainX + (releaseWidth * clampedRelease) / kMaxFiniteReleaseValue;
  geometry.releaseY = axisY;
  geometry.releaseHold = false;
  return geometry;
}
