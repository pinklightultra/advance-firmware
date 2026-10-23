/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _INSERT_EFFECT_DEFINITIONS_H_
#define _INSERT_EFFECT_DEFINITIONS_H_

#include "Foundation/Types/Types.h"

struct InsertEffectInfo {
  const char *name_;
  const char *displayName_;
};

enum VoiceInsertPositionType {
  kVoiceInsertPositionPre = 0,
  kVoiceInsertPositionPost = 1,
};

enum VoiceInsertEffectType {
  kVoiceInsertEffectPassthrough = 0,
  kVoiceInsertEffectOverdrive = 1,
  kVoiceInsertEffectBitcrush = 2,
  kVoiceInsertEffectDownsample = 3,
  kVoiceInsertEffectSlew = 4,
  kVoiceInsertEffectRectify = 5,
};

inline constexpr int kVoiceInsertEffectCount = kVoiceInsertEffectRectify + 1;
inline constexpr int kDefaultVoiceInsertEffectIndex =
    kVoiceInsertEffectPassthrough;
inline constexpr int kDefaultVoiceInsertAmount = 0;
inline constexpr int kVoiceInsertPositionCount = 2;
inline constexpr int kDefaultVoiceInsertPosition = kVoiceInsertPositionPre;

inline constexpr InsertEffectInfo kVoiceInsertEffects[kVoiceInsertEffectCount] =
    {{"none", "none"},       {"overdrive", "odrv"}, {"bitcrush", "crsh"},
     {"downsample", "down"}, {"slew", "slew"},      {"rectify", "rect"}};

inline constexpr const char
    *kVoiceInsertPositionNames[kVoiceInsertPositionCount] = {
        "pre",
        "post",
};

inline int ClampVoiceInsertEffectIndex(int index) {
  if (index < 0 || index >= kVoiceInsertEffectCount) {
    return kDefaultVoiceInsertEffectIndex;
  }

  return index;
}

inline int ClampVoiceInsertPositionIndex(int index) {
  if (index < 0 || index >= kVoiceInsertPositionCount) {
    return kDefaultVoiceInsertPosition;
  }

  return index;
}

inline const InsertEffectInfo &GetVoiceInsertEffectInfo(int index) {
  return kVoiceInsertEffects[ClampVoiceInsertEffectIndex(index)];
}

inline const char *const *GetVoiceInsertPositionNames() {
  return kVoiceInsertPositionNames;
}

inline FourCC GetVoiceInsertTypeFourCC(int slot) {
  switch (slot) {
  case 0:
    return FourCC::VoiceInsertType;
  case 1:
    return FourCC::VoiceInsertType2;
  default:
    return FourCC::VoiceInsertType;
  }
}

inline FourCC GetVoiceInsertAmountFourCC(int slot) {
  switch (slot) {
  case 0:
    return FourCC::VoiceInsertAmount;
  case 1:
    return FourCC::VoiceInsertAmount2;
  default:
    return FourCC::VoiceInsertAmount;
  }
}

inline FourCC GetVoiceInsertPositionFourCC(int slot) {
  switch (slot) {
  case 0:
    return FourCC::VoiceInsertPosition;
  case 1:
    return FourCC::VoiceInsertPosition2;
  default:
    return FourCC::VoiceInsertPosition;
  }
}

#endif
