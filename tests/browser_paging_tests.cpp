/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

#include "Application/Views/BaseClasses/BrowserPaging.h"
#include "doctest/doctest.h"

TEST_CASE(
    "BrowserPaging: empty list does not crash and keeps indices at zero") {
  size_t currentIndex = 0;
  size_t topIndex = 0;

  BrowserPaging::Warp(currentIndex, topIndex, 0, 20, true, false);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);

  BrowserPaging::Warp(currentIndex, topIndex, 0, 20, false, false);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);

  BrowserPaging::Warp(currentIndex, topIndex, 0, 20, true, true);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);

  BrowserPaging::Warp(currentIndex, topIndex, 0, 20, false, true);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);
}

TEST_CASE("BrowserPaging: single item list boundaries") {
  size_t currentIndex = 0;
  size_t topIndex = 0;

  // Single step down
  BrowserPaging::Warp(currentIndex, topIndex, 1, 20, false, false);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);

  // Single step up
  BrowserPaging::Warp(currentIndex, topIndex, 1, 20, true, false);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);

  // Page down
  BrowserPaging::Warp(currentIndex, topIndex, 1, 20, false, true);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);

  // Page up
  BrowserPaging::Warp(currentIndex, topIndex, 1, 20, true, true);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);
}

TEST_CASE("BrowserPaging: list smaller than page size jumps between ends") {
  const size_t totalItems = 5;
  const size_t pageSize = 20;
  size_t currentIndex = 0;
  size_t topIndex = 0;

  // Page down jumps to the last item
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, false,
                      true);
  CHECK(currentIndex == 4);
  CHECK(topIndex == 0);

  // Subsequent page down stays at the last item
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, false,
                      true);
  CHECK(currentIndex == 4);
  CHECK(topIndex == 0);

  // Page up jumps to the first item
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, true, true);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);

  // Subsequent page up stays at the first item
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, true, true);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);
}

TEST_CASE(
    "BrowserPaging: multi-page navigation preserves relative cursor position") {
  const size_t totalItems = 50;
  const size_t pageSize = 20;
  size_t currentIndex = 3;
  size_t topIndex = 0;

  // Page down: from item 3 (row 3) to item 23 (row 3, page starting at 20)
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, false,
                      true);
  CHECK(currentIndex == 23);
  CHECK(topIndex == 20);

  // Page down: to item 43 (row 13, max topIndex is 50 - 20 = 30)
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, false,
                      true);
  CHECK(currentIndex == 43);
  CHECK(topIndex == 30);

  // Page down again: clamped to the last item (49)
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, false,
                      true);
  CHECK(currentIndex == 49);
  CHECK(topIndex == 30);

  // Page up: 49 - 20 = 29, topIndex 30 - 20 = 10
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, true, true);
  CHECK(currentIndex == 29);
  CHECK(topIndex == 10);

  // Page up: 29 - 20 = 9, topIndex clamps to 0
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, true, true);
  CHECK(currentIndex == 9);
  CHECK(topIndex == 0);

  // Page up again: clamps to first item (0), topIndex 0
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, true, true);
  CHECK(currentIndex == 0);
  CHECK(topIndex == 0);
}

TEST_CASE("BrowserPaging: fractional page boundary clamping") {
  const size_t totalItems = 25;
  const size_t pageSize = 20;
  size_t currentIndex = 0;
  size_t topIndex = 0;

  // Page down: item 20, max topIndex is 25 - 20 = 5
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, false,
                      true);
  CHECK(currentIndex == 20);
  CHECK(topIndex == 5);

  // Page down again: clamps to last item 24, topIndex remains 5
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, false,
                      true);
  CHECK(currentIndex == 24);
  CHECK(topIndex == 5);

  // Page up: 24 - 20 = 4, topIndex clamps to 0
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, true, true);
  CHECK(currentIndex == 4);
  CHECK(topIndex == 0);
}

TEST_CASE("BrowserPaging: single-step navigation matches existing behavior") {
  const size_t totalItems = 25;
  const size_t pageSize = 20;
  size_t currentIndex = 0;
  size_t topIndex = 0;

  for (size_t i = 1; i < 20; i++) {
    BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, false,
                        false);
    CHECK(currentIndex == i);
    CHECK(topIndex == 0);
  }

  // Scrolling past page boundary shifts topIndex
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, false,
                      false);
  CHECK(currentIndex == 20);
  CHECK(topIndex == 1);

  // Single step back up
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, true,
                      false);
  CHECK(currentIndex == 19);
  CHECK(topIndex == 1);

  // Single step up to topIndex 0
  BrowserPaging::Warp(currentIndex, topIndex, totalItems, pageSize, true,
                      false);
  CHECK(currentIndex == 18);
  CHECK(topIndex == 1);
}
