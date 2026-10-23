/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Views/VoiceFilterGraph.h"

TEST_CASE("voice filter graph shows lowpass at type 00") {
  const VoiceFilterGraphGeometry geometry =
      CalculateVoiceFilterGraphGeometry(120, 61, 0xFF, 0x00, 0x00);

  CHECK(geometry.leftY < geometry.rightY);
  CHECK(geometry.cutoffX > 100);
}

TEST_CASE("voice filter graph shows bandpass at type 80") {
  const VoiceFilterGraphGeometry geometry =
      CalculateVoiceFilterGraphGeometry(120, 61, 0x80, 0x00, 0x80);

  CHECK(geometry.leftY == geometry.rightY);
  CHECK(geometry.leftY > geometry.peakY);
}

TEST_CASE("voice filter graph shows highpass at type FF") {
  const VoiceFilterGraphGeometry geometry =
      CalculateVoiceFilterGraphGeometry(120, 61, 0x40, 0x00, 0xFF);

  CHECK(geometry.leftY > geometry.rightY);
}

TEST_CASE("voice filter graph raises the resonance peak at cutoff") {
  const VoiceFilterGraphGeometry flat =
      CalculateVoiceFilterGraphGeometry(120, 61, 0x60, 0x00, 0x00);
  const VoiceFilterGraphGeometry resonant =
      CalculateVoiceFilterGraphGeometry(120, 61, 0x60, 0xFF, 0x00);

  CHECK(flat.cutoffX == resonant.cutoffX);
  CHECK(resonant.peakY < flat.peakY);
  CHECK(resonant.leftY == flat.leftY);
  CHECK(resonant.rightY == flat.rightY);
}
