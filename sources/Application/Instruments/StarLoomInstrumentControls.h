/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _STARLOOM_INSTRUMENT_CONTROLS_H_
#define _STARLOOM_INSTRUMENT_CONTROLS_H_

#include <array>

#include "Foundation/Types/Types.h"

namespace StarLoomInstrumentControls {

constexpr std::array<FourCC, 13> kVisibleParameters = {
    FourCC::StarLoomInstrumentMix,
    FourCC::StarLoomInstrumentInteractionMode,
    FourCC::StarLoomInstrumentSignature,
    FourCC::StarLoomInstrumentShape,
    FourCC::StarLoomInstrumentOscillator1PitchOffset,
    FourCC::StarLoomInstrumentTimbre,
    FourCC::StarLoomInstrumentColor,
    FourCC::StarLoomInstrumentOscillator2Shape,
    FourCC::StarLoomInstrumentOscillator2PitchOffset,
    FourCC::StarLoomInstrumentOscillator2Timbre,
    FourCC::StarLoomInstrumentOscillator2Color,
    FourCC::StarLoomInstrumentTable,
    FourCC::StarLoomInstrumentTableAutomation,
};

constexpr bool IsVisibleParameter(FourCC fourcc) {
  for (FourCC parameter : kVisibleParameters) {
    if (parameter == fourcc) {
      return true;
    }
  }
  return false;
}

} // namespace StarLoomInstrumentControls

#endif
