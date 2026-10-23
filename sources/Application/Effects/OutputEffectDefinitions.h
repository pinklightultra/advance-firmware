/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OUTPUT_EFFECT_DEFINITIONS_H_
#define _OUTPUT_EFFECT_DEFINITIONS_H_

#include "Foundation/Types/Types.h"
#include "OutputEffectConstants.h"

inline constexpr const char *GetOutputEffectShortNameForSlot(int slot) {
  switch (slot) {
  case kOutputEffectSlot1Index:
    return "FX1";
  case kOutputEffectSlot2Index:
    return "FX2";
  case kOutputEffectDelayIndex:
    return "DLY";
  case kOutputEffectReverbIndex:
    return "RVB";
  default:
    return "";
  }
}

inline constexpr const char *GetOutputEffectDisplayNameForSlot(int slot) {
  switch (slot) {
  case kOutputEffectSlot1Index:
    return "FX1";
  case kOutputEffectSlot2Index:
    return "FX2";
  case kOutputEffectDelayIndex:
    return "DELAY";
  case kOutputEffectReverbIndex:
    return "REVERB";
  default:
    return "";
  }
}

inline constexpr const char *GetOutputEffectFileStemForSlot(int slot) {
  switch (slot) {
  case kOutputEffectSlot1Index:
    return "fx1";
  case kOutputEffectSlot2Index:
    return "fx2";
  case kOutputEffectDelayIndex:
    return "delay";
  case kOutputEffectReverbIndex:
    return "reverb";
  default:
    return "effect";
  }
}

inline constexpr FourCC GetOutputEffectReturnVariableId(int slot) {
  switch (slot) {
  case kOutputEffectSlot1Index:
    return FourCC::VarFX1Volume;
  case kOutputEffectSlot2Index:
    return FourCC::VarFX2Volume;
  case kOutputEffectDelayIndex:
    return FourCC::VarFX3Volume;
  case kOutputEffectReverbIndex:
    return FourCC::VarFX4Volume;
  default:
    return FourCC::Default;
  }
}

inline constexpr FourCC GetOutputEffectRouteVariableId(int source,
                                                       int destination) {
  if (source == kOutputEffectSlot1Index &&
      destination == kOutputEffectSlot2Index) {
    return FourCC::VarFX1ToFX2Send;
  }
  if (source == kOutputEffectSlot1Index &&
      destination == kOutputEffectDelayIndex) {
    return FourCC::VarFX1ToFX3Send;
  }
  if (source == kOutputEffectSlot1Index &&
      destination == kOutputEffectReverbIndex) {
    return FourCC::VarFX1ToFX4Send;
  }
  if (source == kOutputEffectSlot2Index &&
      destination == kOutputEffectDelayIndex) {
    return FourCC::VarFX2ToFX3Send;
  }
  if (source == kOutputEffectSlot2Index &&
      destination == kOutputEffectReverbIndex) {
    return FourCC::VarFX2ToFX4Send;
  }
  if (source == kOutputEffectDelayIndex &&
      destination == kOutputEffectReverbIndex) {
    return FourCC::VarFX3ToFX4Send;
  }
  return FourCC::Default;
}

#endif
