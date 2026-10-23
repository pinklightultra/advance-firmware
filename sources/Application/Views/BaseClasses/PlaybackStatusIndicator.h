/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef APPLICATION_VIEWS_BASECLASSES_PLAYBACK_STATUS_INDICATOR_H
#define APPLICATION_VIEWS_BASECLASSES_PLAYBACK_STATUS_INDICATOR_H

#include "Application/Views/ViewData.h"
#include <cstdint>

enum class PlaybackStatusIndicator : uint8_t {
  None,
  Normal,
  Phrase,
  Chain,
  Audition,
  MidiInput,
  SamplePreview,
  Tail,
};

constexpr PlaybackStatusIndicator
ResolvePlaybackStatusIndicator(bool isRunning, bool isAuditioning,
                               bool isMidiInputPlaying, bool isSamplePreviewing,
                               bool isTailPlaying, PlayMode playMode) {
  if (isAuditioning) {
    return PlaybackStatusIndicator::Audition;
  }
  if (isMidiInputPlaying) {
    return PlaybackStatusIndicator::MidiInput;
  }
  if (isSamplePreviewing) {
    return PlaybackStatusIndicator::SamplePreview;
  }
  if (isRunning) {
    if (playMode == PM_PHRASE) {
      return PlaybackStatusIndicator::Phrase;
    }
    if (playMode == PM_CHAIN) {
      return PlaybackStatusIndicator::Chain;
    }
    return PlaybackStatusIndicator::Normal;
  }
  if (isTailPlaying) {
    return PlaybackStatusIndicator::Tail;
  }
  return PlaybackStatusIndicator::None;
}

constexpr bool ShowsPlaybackIcon(PlaybackStatusIndicator indicator) {
  return indicator != PlaybackStatusIndicator::None;
}

constexpr bool ShowsChannelPlaybackVisualizer(bool isRunning,
                                              bool isSequencerAudition,
                                              bool isMidiInputPlaying) {
  return (isRunning && !isSequencerAudition) || isMidiInputPlaying;
}

constexpr const char *
GetPlaybackStatusSuffix(PlaybackStatusIndicator indicator) {
  switch (indicator) {
  case PlaybackStatusIndicator::Phrase:
    return "P";
  case PlaybackStatusIndicator::Chain:
    return "C";
  case PlaybackStatusIndicator::Audition:
    return "A";
  case PlaybackStatusIndicator::MidiInput:
    return "M";
  case PlaybackStatusIndicator::SamplePreview:
    return "S";
  case PlaybackStatusIndicator::Tail:
    return "T";
  case PlaybackStatusIndicator::None:
  case PlaybackStatusIndicator::Normal:
    return " ";
  }
  return " ";
}

#endif
