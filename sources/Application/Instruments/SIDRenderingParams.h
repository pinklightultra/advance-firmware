/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _SID_RENDER_PARAMS_H_
#define _SID_RENDER_PARAMS_H_

#include "AdvanceSID/AdvanceSID.h"
#include <cstdint>

struct SIDRenderParams {
  SIDRenderParams() : sid_(44100), playing_(false), baseFrequency_{} {}

  void Reset() {
    sid_.Reset();
    playing_ = false;
    for (uint16_t &frequency : baseFrequency_) {
      frequency = 0;
    }
  }

  AdvanceSID sid_;
  bool playing_;
  uint16_t baseFrequency_[3];
};

#endif
