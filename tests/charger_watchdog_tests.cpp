/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#include "Adapters/adv/system/chargerWatchdog.h"
#include "doctest/doctest.h"

TEST_CASE("BQ25601 watchdog decode matches REG05 watchdog field") {
  CHECK(chargerWatchdogModeFromReg05(0x00) == CHARGER_WATCHDOG_DISABLED);
  CHECK(chargerWatchdogModeFromReg05(0x10) == CHARGER_WATCHDOG_40_SECONDS);
  CHECK(chargerWatchdogModeFromReg05(0x20) == CHARGER_WATCHDOG_80_SECONDS);
  CHECK(chargerWatchdogModeFromReg05(0x30) == CHARGER_WATCHDOG_160_SECONDS);
  CHECK(chargerWatchdogModeFromReg05(0x9F) == CHARGER_WATCHDOG_40_SECONDS);
}
