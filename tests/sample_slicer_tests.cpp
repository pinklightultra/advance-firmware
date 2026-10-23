/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Views/SampleSlicer.h"

TEST_CASE("shared waveform graph geometry is centered and 660 pixels wide") {
  CHECK(GraphField::DefaultXOffset == 30);
  CHECK(GraphField::BitmapWidth == 660);
  CHECK(SampleSlicer::GraphXOffset == GraphField::DefaultXOffset);
  CHECK(SampleSlicer::GraphWidth == GraphField::BitmapWidth);
  CHECK(SampleSlicer::GraphHeight == GraphField::BitmapHeight);
  CHECK(GraphField::MaxMarkers >= SampleSlicer::MaxSlices + 5);
}

TEST_CASE("slicer preview is handled for graph and slicer-owned focus") {
  CHECK(SampleSlicer::ShouldHandlePreview(true, false, false));
  CHECK(SampleSlicer::ShouldHandlePreview(false, true, false));
  CHECK(SampleSlicer::ShouldHandlePreview(false, false, true));
  CHECK_FALSE(SampleSlicer::ShouldHandlePreview(false, false, false));
}

TEST_CASE("slicer previews projected sample points and loop type focus") {
  CHECK(SampleSlicer::ShouldPreviewFromProjectedFocus(true, false));
  CHECK(SampleSlicer::ShouldPreviewFromProjectedFocus(false, true));
  CHECK_FALSE(SampleSlicer::ShouldPreviewFromProjectedFocus(false, false));
}

TEST_CASE("slicer preview range is only used when slices are disabled") {
  CHECK(SampleSlicer::ShouldUsePreviewRange(false, true));
  CHECK_FALSE(SampleSlicer::ShouldUsePreviewRange(true, true));
  CHECK_FALSE(SampleSlicer::ShouldUsePreviewRange(false, false));
}

TEST_CASE("slicer preview loop start is only shown for loop modes") {
  CHECK(SampleSlicer::ShouldShowPreviewLoopStart(true, true));
  CHECK_FALSE(SampleSlicer::ShouldShowPreviewLoopStart(true, false));
  CHECK_FALSE(SampleSlicer::ShouldShowPreviewLoopStart(false, true));
}

TEST_CASE("embedded slicer automatic playhead is sample source only") {
  CHECK(SampleSlicer::ShouldTrackAutomaticPlayhead(IT_SAMPLE_SOURCE));
  CHECK_FALSE(SampleSlicer::ShouldTrackAutomaticPlayhead(IT_SAMPLE));
  CHECK_FALSE(SampleSlicer::ShouldTrackAutomaticPlayhead(IT_MIDI));
  CHECK_FALSE(SampleSlicer::ShouldTrackAutomaticPlayhead(IT_NONE));
}

TEST_CASE("slicer preview note follows slices before root note") {
  CHECK(SampleSlicer::PreviewNoteForState(true, 48, 2, 67) == 50);
  CHECK(SampleSlicer::PreviewNoteForState(false, 48, 2, 67) == 67);
  CHECK(SampleSlicer::PreviewNoteForState(true, 48, -4, 67) == 48);
  CHECK(SampleSlicer::PreviewNoteForState(false, 48, 2, 200) == 127);
}
