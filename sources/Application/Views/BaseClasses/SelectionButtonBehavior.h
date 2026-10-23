/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef APPLICATION_VIEWS_BASECLASSES_SELECTION_BUTTON_BEHAVIOR_H
#define APPLICATION_VIEWS_BASECLASSES_SELECTION_BUTTON_BEHAVIOR_H

#include <cstdint>

enum class SelectionEditAction : uint8_t {
  None,
  Copy,
  Cut,
};

inline SelectionEditAction GetSelectionEditAction(uint16_t mask, bool pressed,
                                                  uint16_t editMask,
                                                  uint16_t enterMask,
                                                  bool &copyPending) {
  if (!pressed) {
    if (copyPending && (mask & editMask) == 0U) {
      copyPending = false;
      return SelectionEditAction::Copy;
    }
    return SelectionEditAction::None;
  }

  if ((mask & editMask) == 0U) {
    return SelectionEditAction::None;
  }

  if ((mask & enterMask) != 0U) {
    copyPending = false;
    return SelectionEditAction::Cut;
  }

  copyPending = mask == editMask;
  return SelectionEditAction::None;
}

#endif
