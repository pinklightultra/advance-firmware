/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OUTPUT_EFFECT_DEFINITIONS_H_
#define _OUTPUT_EFFECT_DEFINITIONS_H_

#include "OutputEffectConstants.h"

struct OutputEffectInfo {
  const char *shortName_;
  const char *displayName_;
};

enum OutputEffectType {
  kOutputEffectNone = 0,
  kOutputEffectReverb = 1,
};

inline constexpr int kOutputEffectTypeCount = kOutputEffectReverb + 1;
inline constexpr int kDefaultOutputEffectTypeIndex = kOutputEffectNone;

inline constexpr OutputEffectInfo kOutputEffectTypes[kOutputEffectTypeCount] = {
    {"NONE", "NONE"},
    {"REVB", "REVERB"},
};

inline constexpr const char *kOutputEffectTypeNames[kOutputEffectTypeCount] = {
    kOutputEffectTypes[kOutputEffectNone].shortName_,
    kOutputEffectTypes[kOutputEffectReverb].shortName_,
};

inline constexpr int kDefaultOutputEffectTypes[OUTPUT_EFFECT_COUNT] = {
    kOutputEffectNone,
    kOutputEffectNone,
    kOutputEffectNone,
};

inline int ClampOutputEffectTypeIndex(int index) {
  if (index < 0 || index >= kOutputEffectTypeCount) {
    return kDefaultOutputEffectTypeIndex;
  }

  return index;
}

inline const OutputEffectInfo &GetOutputEffectInfo(int index) {
  return kOutputEffectTypes[ClampOutputEffectTypeIndex(index)];
}

inline const char *const *GetOutputEffectNames() {
  return kOutputEffectTypeNames;
}

#endif
