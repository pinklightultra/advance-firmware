/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#ifndef _VIEW_DATA_
#define _VIEW_DATA_

#include "Application/Model/Project.h"
#include "Externals/etl/include/etl/string.h"
#include "Externals/etl/include/etl/vector.h"
#include "Foundation/Observable.h"
#include "System/Console/Trace.h"
#include "System/FileSystem/FileSystem.h"
#include <cstdint>

enum PlayMode { PM_SONG, PM_CHAIN, PM_PHRASE, PM_LIVE, PM_AUDITION };

struct SongViewportEvent : public I_ObservableData {
  uint32_t previousOffset_;
  uint32_t offset_;
};

class ViewData : public Observable {

public:
  ViewData(Project *project);
  ~ViewData();
  void Load(Project *project);

  unsigned char UpdateSongChain(int offset);
  uint32_t GetSongOffset() const;
  void SetSongOffset(uint32_t offset);
  void UpdateSongOffset(int32_t offset);
  void UpdateSongCursor(int dx, int dy);
  void RefreshSongViewport();
  void SetSongChain(unsigned char);
  unsigned char *GetCurrentSongPointer();

  void UpdateChainCursor(int dx, int dy);
  unsigned char UpdateChainCursorValue(int offset, int dx, int dy);
  void SetChainPhrase(unsigned char value);
  unsigned char *GetCurrentChainPointer();

protected:
  void checkSongBoundaries();
  void NotifySongOffsetChanged(uint32_t previousOffset);

  inline void updateData(unsigned char *c, int offset, unsigned char limit,
                         bool wrap) {
    int v = *c;
    if ((v == 0xFF) && (limit != 0xFF)) {
      v = 0;
    }
    v += offset;
    if (v < 0)
      v = (wrap ? (limit + 1 + v) : 0);
    if (v > limit)
      v = (wrap ? v - (limit + 1) : limit);
    *c = v;
  }

public:
  // Data

  Project *project_;
  Song *song_;
  etl::vector<I_Observer *, 8> observers_;

  // Editor settings

  int songX_;      // .Current song screen position in the editor
  int songY_;      //
  int songOffset_; // .Current song offset (top screen row) in editor

  int chainRow_; // .Current row in chain editor
  int chainCol_; // .Current column in chain editor

  int currentChain_;  // .Current edited chain
  int currentPhrase_; // .Current edited phrase

  int currentInstrumentID_; // .Current edited instrument

  int currentTable_; // .Current edited table

  int currentGroove_; // .Current edited groove

  // Player Settings

  PlayMode playMode_;
  int songPlayPos_[SONG_CHANNEL_COUNT]; // .Play position of each channel
  unsigned char currentPlayChain_[SONG_CHANNEL_COUNT]; // .Current played chain
                                                       // for each channel
  int chainPlayPos_[SONG_CHANNEL_COUNT]; // .Play position in chain for each
                                         // channel
  unsigned char
      currentPlayPhrase_[SONG_CHANNEL_COUNT]; // .Current played phrase for each
                                              // channel
  int phrasePlayPos_[SONG_CHANNEL_COUNT]; // .Play position in phrase for each
                                          // channel
  int phraseCurPos_;                      // current UI cursor row position

  // Sample Editor
  etl::string<PFILENAME_SIZE> sampleEditorFilename;
  bool isShowingSampleEditorProjectPool = false;

  // Import File browser
  etl::string<PFILENAME_SIZE> importViewStartDir;
};
#endif
