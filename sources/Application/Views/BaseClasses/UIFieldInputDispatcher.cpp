/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "UIFieldInputDispatcher.h"

bool UIFieldInputDispatcher::ProcessButtonMask(UIField &field,
                                               unsigned short mask) {
  if (mask & EPBM_ENTER) {
    if (mask & EPBM_DOWN) {
      field.ProcessArrow(EPBM_DOWN);
    }
    if (mask & EPBM_UP) {
      field.ProcessArrow(EPBM_UP);
    }
    if (mask & EPBM_LEFT) {
      field.ProcessArrow(EPBM_LEFT);
    }
    if (mask & EPBM_RIGHT) {
      field.ProcessArrow(EPBM_RIGHT);
    }
    if (mask & EPBM_EDIT) {
      field.ProcessClear();
    }
    if (mask == EPBM_ENTER) {
      field.OnClick();
    }
    return true;
  }

  if (mask & EPBM_EDIT) {
    if (mask == EPBM_EDIT) {
      field.OnEditClick();
    }
    if (mask & EPBM_DOWN) {
      field.ProcessEditArrow(EPBM_DOWN);
    }
    if (mask & EPBM_UP) {
      field.ProcessEditArrow(EPBM_UP);
    }
    if (mask & EPBM_LEFT) {
      field.ProcessEditArrow(EPBM_LEFT);
    }
    if (mask & EPBM_RIGHT) {
      field.ProcessEditArrow(EPBM_RIGHT);
    }
    return true;
  }

  return false;
}
