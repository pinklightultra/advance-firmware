/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "Application/Player/SongLoopRange.h"
#include <doctest/doctest.h>

TEST_CASE("Inactive song loop preserves normal playback boundaries") {
  const SongLoopRange range;

  CHECK_FALSE(ShouldWrapSongLoop(range, 12, false));
  CHECK_FALSE(ShouldWrapSongLoop(range, 12, true));
}

TEST_CASE("Song loop continues through rows inside its selection") {
  const SongLoopRange range = {
      .active = true,
      .startRow = 8,
      .endRow = 12,
  };

  CHECK_FALSE(ShouldWrapSongLoop(range, 12, false));
}

TEST_CASE("Song loop wraps after its final selected row") {
  const SongLoopRange range = {
      .active = true,
      .startRow = 8,
      .endRow = 12,
  };

  CHECK(ShouldWrapSongLoop(range, 13, false));
}

TEST_CASE("Song loop wraps at a natural section boundary") {
  const SongLoopRange range = {
      .active = true,
      .startRow = 8,
      .endRow = 12,
  };

  CHECK(ShouldWrapSongLoop(range, 10, true));
}

TEST_CASE("Invalid song loop range preserves normal playback") {
  const SongLoopRange range = {
      .active = true,
      .startRow = 12,
      .endRow = 8,
  };

  CHECK_FALSE(ShouldWrapSongLoop(range, 13, true));
}

TEST_CASE("Inactive song loop plays every channel") {
  const SongLoopRange range;

  CHECK(ShouldPlaySongLoopChannel(range, 0));
  CHECK(ShouldPlaySongLoopChannel(range, 7));
}

TEST_CASE("Song loop plays only selected channels") {
  const SongLoopRange range = {
      .active = true,
      .startRow = 8,
      .endRow = 12,
      .startChannel = 2,
      .endChannel = 4,
  };

  CHECK_FALSE(ShouldPlaySongLoopChannel(range, 1));
  CHECK(ShouldPlaySongLoopChannel(range, 2));
  CHECK(ShouldPlaySongLoopChannel(range, 3));
  CHECK(ShouldPlaySongLoopChannel(range, 4));
  CHECK_FALSE(ShouldPlaySongLoopChannel(range, 5));
}
