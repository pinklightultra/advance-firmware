/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#pragma once

#include <stdint.h>

struct ControlSurfaceState {
  enum class Cell : uint8_t {
    Empty,
    Populated,
    NoteOff,
    FirstPattern,
    Playing,
    Queued,
    Muted
  };
  Cell cells[8][8];
  bool isRunning;
  bool isLiveMode;
};
