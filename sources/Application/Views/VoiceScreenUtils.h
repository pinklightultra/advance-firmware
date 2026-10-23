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

inline bool SupportsAudioVoiceScreen(I_Instrument *instrument) {
  return instrument != nullptr &&
         InstrumentTypeUsesModernAudioVoice(instrument->GetType());
}

inline bool SupportsInstrumentPreview(I_Instrument *instrument) {
  if (instrument == nullptr) {
    return false;
  }
  const InstrumentType type = instrument->GetType();
  return type != IT_NONE && type != IT_MIDI && type != IT_LAST;
}

inline bool ShouldShowAudioVoiceScreenOnMap(ViewType viewType,
                                            I_Instrument *instrument) {
  if (!SupportsAudioVoiceScreen(instrument)) {
    return false;
  }

  return viewType == VT_INSTRUMENT || viewType == VT_VOICE;
}

#endif
