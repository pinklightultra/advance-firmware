/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "Application/Views/BaseClasses/SelectionButtonBehavior.h"
#include <doctest/doctest.h>

namespace {
constexpr uint16_t editMask = 0x0020U;
constexpr uint16_t enterMask = 0x0040U;
constexpr uint16_t altMask = 0x0010U;
} // namespace

TEST_CASE("Selection delete works when edit is pressed first") {
  bool copyPending = false;

  CHECK(GetSelectionEditAction(editMask, true, editMask, enterMask,
                               copyPending) == SelectionEditAction::None);
  CHECK(copyPending);
  CHECK(GetSelectionEditAction(editMask | enterMask, true, editMask, enterMask,
                               copyPending) == SelectionEditAction::Cut);
  CHECK_FALSE(copyPending);
}

TEST_CASE("Selection delete works when enter is pressed first") {
  bool copyPending = false;

  CHECK(GetSelectionEditAction(enterMask, true, editMask, enterMask,
                               copyPending) == SelectionEditAction::None);
  CHECK(GetSelectionEditAction(editMask | enterMask, true, editMask, enterMask,
                               copyPending) == SelectionEditAction::Cut);
  CHECK_FALSE(copyPending);
}

TEST_CASE("Selection copy runs when edit is released") {
  bool copyPending = false;

  CHECK(GetSelectionEditAction(editMask, true, editMask, enterMask,
                               copyPending) == SelectionEditAction::None);
  CHECK(GetSelectionEditAction(0U, false, editMask, enterMask, copyPending) ==
        SelectionEditAction::Copy);
  CHECK_FALSE(copyPending);
}

TEST_CASE("Selection modifier chord cancels pending copy") {
  bool copyPending = false;

  CHECK(GetSelectionEditAction(editMask, true, editMask, enterMask,
                               copyPending) == SelectionEditAction::None);
  CHECK(GetSelectionEditAction(editMask | altMask, true, editMask, enterMask,
                               copyPending) == SelectionEditAction::None);
  CHECK(GetSelectionEditAction(0U, false, editMask, enterMask, copyPending) ==
        SelectionEditAction::None);
  CHECK_FALSE(copyPending);
}
