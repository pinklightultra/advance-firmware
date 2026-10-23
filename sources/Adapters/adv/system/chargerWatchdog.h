/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#ifndef _ADV_CHARGER_WATCHDOG_H_
#define _ADV_CHARGER_WATCHDOG_H_

#include <stdint.h>

enum ChargerWatchdogMode {
  CHARGER_WATCHDOG_DISABLED = 0,
  CHARGER_WATCHDOG_40_SECONDS,
  CHARGER_WATCHDOG_80_SECONDS,
  CHARGER_WATCHDOG_160_SECONDS,
};

static inline ChargerWatchdogMode
chargerWatchdogModeFromReg05(uint8_t regValue) {
  return (ChargerWatchdogMode)((regValue >> 4) & 0x3U);
}

static inline const char *chargerWatchdogModeName(ChargerWatchdogMode mode) {
  switch (mode) {
  case CHARGER_WATCHDOG_DISABLED:
    return "disabled";
  case CHARGER_WATCHDOG_40_SECONDS:
    return "enabled (40s)";
  case CHARGER_WATCHDOG_80_SECONDS:
    return "enabled (80s)";
  case CHARGER_WATCHDOG_160_SECONDS:
    return "enabled (160s)";
  }

  return "unknown";
}

#endif
