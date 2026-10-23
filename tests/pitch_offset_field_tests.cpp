/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Instruments/PitchOffset.h"
#include "Application/Views/BaseClasses/UIPitchOffsetVarField.h"

using UIPitchOffsetVarFieldDetail::FormatPitchOffsetCents;

TEST_CASE("pitch offsets use compact semitone and cent text") {
  CHECK(FormatPitchOffsetCents(0) == "00.00");
  CHECK(FormatPitchOffsetCents(1) == "00.01");
  CHECK(FormatPitchOffsetCents(99) == "00.99");
  CHECK(FormatPitchOffsetCents(100) == "01.00");
  CHECK(FormatPitchOffsetCents(1234) == "12.34");
  CHECK(FormatPitchOffsetCents(-1) == "-00.01");
  CHECK(FormatPitchOffsetCents(-125) == "-01.25");
  CHECK(FormatPitchOffsetCents(PitchOffset::MaximumCents) == "48.00");
  CHECK(FormatPitchOffsetCents(PitchOffset::MinimumCents) == "-48.00");
}

TEST_CASE("pitch offset formatting clamps to the supported range") {
  CHECK(FormatPitchOffsetCents(PitchOffset::MaximumCents + 1) == "48.00");
  CHECK(FormatPitchOffsetCents(PitchOffset::MinimumCents - 1) == "-48.00");
}
