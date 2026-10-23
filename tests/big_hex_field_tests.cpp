/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Views/BaseClasses/UIBigHexVarField.h"

using UIBigHexVarFieldDetail::AdjustIndependentHexDigit;

TEST_CASE("independent hex digit adjustments clamp without carry or wrap") {
  CHECK(AdjustIndependentHexDigit(0x123E, 0, 1) == 0x123F);
  CHECK(AdjustIndependentHexDigit(0x123F, 0, 1) == 0x123F);
  CHECK(AdjustIndependentHexDigit(0x1230, 0, -1) == 0x1230);

  CHECK(AdjustIndependentHexDigit(0x12EF, 1, 1) == 0x12FF);
  CHECK(AdjustIndependentHexDigit(0x12FF, 1, 1) == 0x12FF);
  CHECK(AdjustIndependentHexDigit(0x120F, 1, -1) == 0x120F);

  CHECK(AdjustIndependentHexDigit(0xA0FF, 3, 1) == 0xB0FF);
  CHECK(AdjustIndependentHexDigit(0xA0FF, 2, -1) == 0xA0FF);
}
