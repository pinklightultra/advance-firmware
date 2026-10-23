/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#pragma once

#include "UIFramework/Framework/GUIColor.h"
#include <cstdint>

// Convert a 0-255 GUI color component to a 0-127 Launchpad RGB component
inline uint8_t toLaunchpadRgbComponent(unsigned short value) {
  const uint32_t scaled = (static_cast<uint32_t>(value) * 127u + 127u) / 255u;
  return static_cast<uint8_t>(scaled > 127u ? 127u : scaled);
}

// Pack a GUIColor into a 24-bit RGB value with each component scaled to 0-127
inline uint32_t packGuiColor(const GUIColor &color) {
  return (static_cast<uint32_t>(toLaunchpadRgbComponent(color._r)) << 16) |
         (static_cast<uint32_t>(toLaunchpadRgbComponent(color._g)) << 8) |
         static_cast<uint32_t>(toLaunchpadRgbComponent(color._b));
}
