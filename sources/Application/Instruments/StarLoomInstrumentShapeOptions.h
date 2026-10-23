/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _STARLOOM_INSTRUMENT_SHAPE_OPTIONS_H_
#define _STARLOOM_INSTRUMENT_SHAPE_OPTIONS_H_

#include "Externals/braids/settings.h"

#include <stdint.h>

namespace StarLoomInstrumentShapes {

inline constexpr uint8_t kAllowedShapeValues[] = {
#define STARLOOM_INSTRUMENT_SHAPE(shape, name)                                 \
  static_cast<uint8_t>(braids::shape),
#include "StarLoomInstrumentShapeAllowList.inc"
#undef STARLOOM_INSTRUMENT_SHAPE
};

inline constexpr const char *kAllowedShapeNames[] = {
#define STARLOOM_INSTRUMENT_SHAPE(shape, name) name,
#include "StarLoomInstrumentShapeAllowList.inc"
#undef STARLOOM_INSTRUMENT_SHAPE
};

inline constexpr int kAllowedShapeCount =
    sizeof(kAllowedShapeValues) / sizeof(kAllowedShapeValues[0]);
static_assert(kAllowedShapeCount > 0);
static_assert(kAllowedShapeCount ==
              sizeof(kAllowedShapeNames) / sizeof(kAllowedShapeNames[0]));

constexpr braids::MacroOscillatorShape ShapeForSelection(int selection) {
  if (selection < 0 || selection >= kAllowedShapeCount) {
    return static_cast<braids::MacroOscillatorShape>(kAllowedShapeValues[0]);
  }
  return static_cast<braids::MacroOscillatorShape>(
      kAllowedShapeValues[selection]);
}

} // namespace StarLoomInstrumentShapes

#endif
