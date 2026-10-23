/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _DEVICE_IDENTITY_H_
#define _DEVICE_IDENTITY_H_

#include <stdint.h>

namespace DeviceIdentity {

// Hash all 96 bits of the hardware UID into a stable 32-bit seed. Processing
// bytes explicitly keeps the result independent of the processor's endianness.
constexpr uint32_t SeedFromUidWords(uint32_t word0, uint32_t word1,
                                    uint32_t word2) {
  constexpr uint32_t kFnvOffsetBasis = 2166136261U;
  constexpr uint32_t kFnvPrime = 16777619U;
  const uint32_t words[] = {word0, word1, word2};
  uint32_t hash = kFnvOffsetBasis;
  for (uint32_t word : words) {
    for (uint8_t byte = 0; byte < sizeof(word); ++byte) {
      hash ^= (word >> (byte * 8)) & 0xFFU;
      hash *= kFnvPrime;
    }
  }
  return hash;
}

} // namespace DeviceIdentity

#endif
