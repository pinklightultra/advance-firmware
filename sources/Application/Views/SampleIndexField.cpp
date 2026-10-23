/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#include "SampleIndexField.h"

SampleIndexField::SampleIndexField(const GUIPoint &position, Variable &variable,
                                   int32_t maximum, ChangeRequest changeRequest)
    : UIIntVarField(position, variable, "Sample: %.17s", 0, maximum, 1, 0x10),
      changeRequest_(changeRequest) {}

void SampleIndexField::ProcessArrow(unsigned short mask) {
  int32_t value = src_.GetInt();

  switch (mask) {
  case EPBM_UP:
    value += yOffset_;
    break;
  case EPBM_DOWN:
    value -= yOffset_;
    break;
  case EPBM_LEFT:
    value -= xOffset_;
    break;
  case EPBM_RIGHT:
    value += xOffset_;
    break;
  }

  if (value < min_) {
    value = min_;
  }
  if (value > max_) {
    value = max_;
  }
  requestChange(value);
}

void SampleIndexField::ProcessClear() {
  if (src_.IsModified()) {
    requestChange(0);
  }
}

void SampleIndexField::requestChange(int32_t value) {
  if (value == src_.GetInt() || !changeRequest_(value)) {
    return;
  }

  src_.SetInt(value);
  SetChanged();
  NotifyObservers(reinterpret_cast<I_ObservableData *>(
      static_cast<uintptr_t>(src_.GetID())));
}
