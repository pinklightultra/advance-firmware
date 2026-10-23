/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */
#include "Adapters/adv/system/advUsbRoleManager.h"
#include "doctest/doctest.h"

TEST_CASE("USB host attach requests VBUS power through charger registers") {
  AdvUsbTypeCStatus status = {};
  status.cableState = ADV_USB_CABLE_HOST;

  CHECK(advUsbStatusRequestsHostPower(&status));
}

TEST_CASE("USB non-host states keep VBUS power output disabled") {
  AdvUsbTypeCStatus status = {};

  status.cableState = ADV_USB_CABLE_DETACHED;
  CHECK_FALSE(advUsbStatusRequestsHostPower(&status));

  status.cableState = ADV_USB_CABLE_DEVICE;
  CHECK_FALSE(advUsbStatusRequestsHostPower(&status));

  status.cableState = ADV_USB_CABLE_UNRESOLVED;
  CHECK_FALSE(advUsbStatusRequestsHostPower(&status));

  CHECK_FALSE(advUsbStatusRequestsHostPower(nullptr));
}
