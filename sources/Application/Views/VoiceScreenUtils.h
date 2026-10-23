/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _VOICE_SCREEN_UTILS_H_
#define _VOICE_SCREEN_UTILS_H_

#include "Application/Instruments/I_Instrument.h"
#include "BaseClasses/View.h"

inline bool SupportsAudioVoiceScreen(const I_Instrument *instrument) {
  return instrument != nullptr && instrument->SupportsAudioVoiceScreen();
}

inline bool ShouldShowAudioVoiceScreenOnMap(ViewType viewType,
                                            const I_Instrument *instrument) {
  if (!SupportsAudioVoiceScreen(instrument)) {
    return false;
  }

  return viewType == VT_INSTRUMENT || viewType == VT_VOICE;
}

#endif
