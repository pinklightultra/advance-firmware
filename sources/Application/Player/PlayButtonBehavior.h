/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef APPLICATION_PLAYER_PLAY_BUTTON_BEHAVIOR_H
#define APPLICATION_PLAYER_PLAY_BUTTON_BEHAVIOR_H

#include <cstdint>

constexpr uint32_t playButtonKillHoldMs = 1000U;

inline bool HasPlayButtonKillHoldElapsed(bool active, uint32_t pressedAt,
                                         uint32_t now) {
  return active &&
         static_cast<uint32_t>(now - pressedAt) >= playButtonKillHoldMs;
}

inline bool IsInstrumentPreviewButtonMask(uint16_t mask, uint16_t playMask,
                                          uint16_t altMask) {
  return mask == static_cast<uint16_t>(playMask | altMask);
}

inline bool IsInstrumentPreviewComboHeld(uint16_t mask, uint16_t playMask,
                                         uint16_t altMask) {
  const uint16_t previewMask = static_cast<uint16_t>(playMask | altMask);
  return (mask & previewMask) == previewMask;
}

inline bool ShouldBypassPlayButtonDeferralForPreview(bool previewAvailable,
                                                     uint16_t mask,
                                                     uint16_t playMask,
                                                     uint16_t altMask) {
  return previewAvailable &&
         IsInstrumentPreviewButtonMask(mask, playMask, altMask);
}

#endif
