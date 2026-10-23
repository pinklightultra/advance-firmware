/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#include "Adapters/adv/system/charger.h"
#include "doctest/doctest.h"

TEST_CASE("host power enable uses charger register value") {
  CHECK(chargerHostPowerReg01Value(true) == 0x2B);
  CHECK(chargerHostPowerReg01Value(false) == 0x1B);
}
