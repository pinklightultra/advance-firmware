/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "VoiceFilterGraph.h"

#include <algorithm>

namespace {
constexpr int32_t kMaxFilterValue = 0xFF;

int32_t lerpInt(int32_t start, int32_t end, int32_t amount,
                int32_t amountMax = kMaxFilterValue) {
  return start + ((end - start) * amount) / amountMax;
}
} // namespace

VoiceFilterGraphGeometry
CalculateVoiceFilterGraphGeometry(int32_t width, int32_t height, int32_t cutoff,
                                  int32_t resonance, int32_t filterType) {
  const int32_t clampedWidth = std::max<int32_t>(width, 0);
  const int32_t maxY = std::max<int32_t>(height - 1, 0);
  const int32_t inset = std::min<int32_t>(12, clampedWidth / 6);
  const int32_t usableWidth = std::max<int32_t>(clampedWidth - (inset * 2), 0);
  const int32_t clampedCutoff = std::clamp<int32_t>(cutoff, 0, kMaxFilterValue);
  const int32_t clampedResonance =
      std::clamp<int32_t>(resonance, 0, kMaxFilterValue);
  const int32_t clampedType =
      std::clamp<int32_t>(filterType, 0, kMaxFilterValue);
  const int32_t lowY = maxY;
  const int32_t highY =
      std::min<int32_t>(maxY, std::max<int32_t>(3, height / 2));
  const int32_t leftY =
      (clampedType <= 0x80)
          ? lerpInt(highY, lowY, (clampedType * kMaxFilterValue) / 0x80)
          : lowY;
  const int32_t rightY =
      (clampedType <= 0x80)
          ? lowY
          : lerpInt(lowY, highY,
                    ((clampedType - 0x80) * kMaxFilterValue) / 0x7F);
  const int32_t cutoffX =
      inset + (usableWidth * clampedCutoff) / kMaxFilterValue;
  const int32_t shoulderSpread = std::max<int32_t>(8, usableWidth / 10);
  const int32_t peakBaseY = highY;
  const int32_t peakBoost =
      ((peakBaseY - 1) * clampedResonance) / kMaxFilterValue;

  VoiceFilterGraphGeometry geometry;
  geometry.leftShoulderX = std::max<int32_t>(0, cutoffX - shoulderSpread);
  geometry.cutoffX = cutoffX;
  geometry.rightShoulderX =
      std::min<int32_t>(clampedWidth, cutoffX + shoulderSpread);
  geometry.leftY = leftY;
  geometry.peakY = std::max<int32_t>(1, peakBaseY - peakBoost);
  geometry.rightY = rightY;
  return geometry;
}
