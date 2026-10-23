/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "Application/Player/PlayButtonBehavior.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>

TEST_CASE("Play button kill hold stays inactive before one second") {
  CHECK(!HasPlayButtonKillHoldElapsed(true, 100U, 1099U));
}

TEST_CASE("Play button kill hold triggers after one second") {
  CHECK(HasPlayButtonKillHoldElapsed(true, 100U, 1100U));
}

TEST_CASE("Play button kill hold ignores inactive presses") {
  CHECK(!HasPlayButtonKillHoldElapsed(false, 100U, 5000U));
}

TEST_CASE("Play button kill hold handles tick wraparound") {
  const uint32_t pressedAt = std::numeric_limits<uint32_t>::max() - 499U;

  CHECK(!HasPlayButtonKillHoldElapsed(true, pressedAt, 400U));
  CHECK(HasPlayButtonKillHoldElapsed(true, pressedAt, 500U));
}

TEST_CASE("Sample source preview bypasses only alt play deferral") {
  static constexpr uint16_t kPlay = 0x0100;
  static constexpr uint16_t kAlt = 0x0010;
  static constexpr uint16_t kNav = 0x0080;

  CHECK(ShouldBypassPlayButtonDeferralForPreview(true, kPlay | kAlt, kPlay,
                                                 kAlt));
  CHECK_FALSE(
      ShouldBypassPlayButtonDeferralForPreview(true, kPlay, kPlay, kAlt));
  CHECK_FALSE(ShouldBypassPlayButtonDeferralForPreview(
      true, kPlay | kAlt | kNav, kPlay, kAlt));
  CHECK_FALSE(ShouldBypassPlayButtonDeferralForPreview(false, kPlay | kAlt,
                                                       kPlay, kAlt));
}

TEST_CASE("Sample preview button mask requires exact alt play press") {
  static constexpr uint16_t kPlay = 0x0100;
  static constexpr uint16_t kAlt = 0x0010;
  static constexpr uint16_t kEdit = 0x0020;

  CHECK(IsSamplePreviewButtonMask(kPlay | kAlt, kPlay, kAlt));
  CHECK_FALSE(IsSamplePreviewButtonMask(kPlay, kPlay, kAlt));
  CHECK_FALSE(IsSamplePreviewButtonMask(kPlay | kAlt | kEdit, kPlay, kAlt));
}

TEST_CASE(
    "Sample preview combo remains held only while alt and play are down") {
  static constexpr uint16_t kPlay = 0x0100;
  static constexpr uint16_t kAlt = 0x0010;
  static constexpr uint16_t kNav = 0x0080;

  CHECK(IsSamplePreviewComboHeld(kPlay | kAlt, kPlay, kAlt));
  CHECK(IsSamplePreviewComboHeld(kPlay | kAlt | kNav, kPlay, kAlt));
  CHECK_FALSE(IsSamplePreviewComboHeld(kPlay, kPlay, kAlt));
  CHECK_FALSE(IsSamplePreviewComboHeld(kAlt, kPlay, kAlt));
}
