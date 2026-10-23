/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _SELECTION_RESAMPLE_REQUEST_H_
#define _SELECTION_RESAMPLE_REQUEST_H_

#include "Application/Views/ViewData.h"
#include "Externals/etl/include/etl/string.h"
#include "System/FileSystem/FileSystem.h"
#include <stdint.h>

enum class SelectionResampleSourceKind : uint8_t {
  Song,
  Chain,
  Phrase,
};

enum class SelectionOutputIntent : uint8_t {
  ProjectSample,
  Mixdown,
  Stems,
};

struct SelectionResampleRequest {
  SelectionResampleSourceKind sourceKind = SelectionResampleSourceKind::Song;
  SelectionOutputIntent outputIntent = SelectionOutputIntent::ProjectSample;
  etl::string<MAX_PROJECT_SAMPLE_PATH_LENGTH> outputPath;
  uint8_t startSongRow = 0;
  uint8_t endSongRow = 0;
  uint8_t startChainRow = 0;
  uint8_t endChainRow = 0;
  uint8_t startPhraseRow = 0;
  uint8_t endPhraseRow = 0;
  uint8_t channelStart = 0;
  uint8_t channelEnd = 0;
  uint8_t trackerChannel = 0;
  uint8_t currentChain = 0xFF;
  uint8_t currentPhrase = 0xFF;

  PlayMode GetPlayMode() const {
    switch (sourceKind) {
    case SelectionResampleSourceKind::Song:
      return PM_SONG;
    case SelectionResampleSourceKind::Chain:
      return PM_CHAIN;
    case SelectionResampleSourceKind::Phrase:
      return PM_PHRASE;
    }

    return PM_SONG;
  }

  uint16_t GetSongChannelMask() const {
    uint16_t mask = 0;
    for (uint8_t channel = channelStart;
         channel <= channelEnd && channel < SONG_CHANNEL_COUNT; ++channel) {
      mask |= static_cast<uint16_t>(1U << channel);
    }
    return mask;
  }
};

#endif
