/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Views/VoiceEnvelopeGraph.h"

TEST_CASE("voice envelope graph uses positive depth from bottom axis") {
  const VoiceEnvelopeShapeValues values{
      .attack = 0xFF, .decay = 0xFF, .sustain = 0x80, .release = 0xFE};
  const VoiceEnvelopeGraphGeometry geometry =
      CalculateVoiceEnvelopeGraphGeometry(100, 81, values, 0xFF);

  CHECK(geometry.axisY == 80);
  CHECK(geometry.peakY == 0);
  CHECK(geometry.attackX == 25);
  CHECK(geometry.decayX == 50);
  CHECK(geometry.sustainX == 75);
  CHECK(geometry.sustainY == 40);
  CHECK(geometry.releaseX == 100);
  CHECK(geometry.releaseY == 80);
  CHECK(geometry.releaseHold == false);
}

TEST_CASE("voice envelope graph inverts negative depth by shifting the axis") {
  const VoiceEnvelopeShapeValues values{
      .attack = 0x80, .decay = 0x40, .sustain = 0x80, .release = 0x20};
  const VoiceEnvelopeGraphGeometry geometry =
      CalculateVoiceEnvelopeGraphGeometry(100, 81, values, -0x80);

  CHECK(geometry.axisY == 40);
  CHECK(geometry.peakY == 80);
  CHECK(geometry.sustainY == 60);
  CHECK(geometry.attackX == 12);
  CHECK(geometry.decayX == 18);
  CHECK(geometry.releaseY == 40);
  CHECK(geometry.releaseHold == false);
}

TEST_CASE("voice envelope graph keeps sustain level for infinite release") {
  const VoiceEnvelopeShapeValues values{
      .attack = 0x10, .decay = 0x20, .sustain = 0x60, .release = 0xFF};
  const VoiceEnvelopeGraphGeometry geometry =
      CalculateVoiceEnvelopeGraphGeometry(120, 61, values, 0xC0);

  CHECK(geometry.sustainX == 90);
  CHECK(geometry.releaseX == 120);
  CHECK(geometry.releaseY == geometry.sustainY);
  CHECK(geometry.releaseHold == true);
}
