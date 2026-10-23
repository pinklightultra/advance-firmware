/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Mixer/OutputEffectDefinitions.h"
#include "Application/Model/Project.h"

TEST_CASE("Project defaults master output effect all slots to NONE") {
  Project project("fx-defaults");

  CHECK(project.GetFXSlotType(0) == kOutputEffectNone);
  CHECK(project.GetFXSlotType(1) == kOutputEffectNone);
  CHECK(project.GetFXSlotType(2) == kOutputEffectNone);
}

TEST_CASE("Changing an output effect slot type marks mix routing dirty") {
  Project project("fx-routing");
  Variable *slotType = project.FindVariable(FourCC::VarFX2Type);

  REQUIRE(slotType != nullptr);
  project.ClearMixRoutingDirty();

  slotType->SetInt(kOutputEffectReverb, true);

  CHECK(project.IsMixRoutingDirty());
  CHECK(project.GetFXSlotType(1) == kOutputEffectReverb);
}
