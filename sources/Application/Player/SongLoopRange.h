/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef APPLICATION_PLAYER_SONG_LOOP_RANGE_H
#define APPLICATION_PLAYER_SONG_LOOP_RANGE_H

#include <cstdint>

struct SongLoopRange {
  bool active = false;
  uint8_t startRow = 0;
  uint8_t endRow = 0;
  uint8_t startChannel = 0;
  uint8_t endChannel = 0;
};

inline bool IsSongLoopRangeValid(const SongLoopRange &range) {
  return range.active && range.startRow <= range.endRow;
}

inline bool ShouldWrapSongLoop(const SongLoopRange &range, int32_t nextRow,
                               bool naturalLoopBack) {
  return IsSongLoopRangeValid(range) &&
         (naturalLoopBack || nextRow > static_cast<int32_t>(range.endRow));
}

inline bool ShouldPlaySongLoopChannel(const SongLoopRange &range,
                                      int32_t channel) {
  return !IsSongLoopRangeValid(range) ||
         (channel >= static_cast<int32_t>(range.startChannel) &&
          channel <= static_cast<int32_t>(range.endChannel));
}

#endif
