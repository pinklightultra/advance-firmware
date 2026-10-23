/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _SAMPLE_POOL_STORAGE_H_
#define _SAMPLE_POOL_STORAGE_H_

#include <cstdint>

namespace AdvanceSampleStorage {

static constexpr uint32_t kSampleAlignment = 4;

constexpr uint32_t GetAlignmentPadding(uint32_t writeOffset) {
  return (kSampleAlignment - (writeOffset % kSampleAlignment)) %
         kSampleAlignment;
}

constexpr uint32_t GetAvailableCapacity(uint32_t writeOffset,
                                        uint32_t storeLimit) {
  if (writeOffset > storeLimit) {
    return 0;
  }

  const uint32_t remaining = storeLimit - writeOffset;
  const uint32_t padding = GetAlignmentPadding(writeOffset);
  return padding <= remaining ? remaining - padding : 0;
}

constexpr bool Fits(uint32_t writeOffset, uint32_t storeLimit,
                    uint32_t sampleSize) {
  return sampleSize <= GetAvailableCapacity(writeOffset, storeLimit);
}

constexpr bool FitsInEither(uint32_t writeOffset1, uint32_t storeLimit1,
                            uint32_t writeOffset2, uint32_t storeLimit2,
                            uint32_t sampleSize) {
  return Fits(writeOffset1, storeLimit1, sampleSize) ||
         Fits(writeOffset2, storeLimit2, sampleSize);
}

} // namespace AdvanceSampleStorage

#endif
