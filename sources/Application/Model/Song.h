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

#ifndef _SONG_H_
#define _SONG_H_

#include "Application/Persistency/Persistent.h"
#include "Chain.h"
#include "Phrase.h"

#define SONG_CHANNEL_COUNT_STEREO 8
#define SONG_CHANNEL_COUNT_MONO 12
#define SONG_CHANNEL_COUNT SONG_CHANNEL_COUNT_MONO
#define SONG_ROW_COUNT 256

#define MAX_SAMPLEINSTRUMENT_COUNT 0x40
#define MAX_SIDINSTRUMENT_COUNT 0x00
#define MAX_MIDIINSTRUMENT_COUNT 0x40
#define MAX_OPALINSTRUMENT_COUNT 0x00
#define MAX_MACROINSTRUMENT_COUNT 0x00
#define MAX_INSTRUMENT_COUNT 0x40

#define HIGHEST_NOTE 119
#define NOTE_OFF 0xFE
#define NO_NOTE 0xFF
#define NOTE_C3 60
#define EMPTY_SONG_VALUE 0xFF

class Song : Persistent {
public:
  Song();
  ~Song();
  void Reset();
  int GetChannelCount() const { return channelCount_; }
  void SetChannelCount(int channelCount);

  virtual void SaveContent(tinyxml2::XMLPrinter *printer);
  virtual void RestoreContent(PersistencyDocument *doc);

  unsigned char data_[SONG_CHANNEL_COUNT * SONG_ROW_COUNT];
  Chain chain_;
  Phrase phrase_;

private:
  int channelCount_;
};

#endif
