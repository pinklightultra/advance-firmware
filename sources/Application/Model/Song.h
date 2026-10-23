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
#include "Externals/etl/include/etl/array.h"
#include "Phrase.h"

#define SONG_CHANNEL_COUNT 8
#define SONG_ROW_COUNT 256

#define MAX_SAMPLEINSTRUMENT_COUNT 0x40
#define MAX_SIDINSTRUMENT_COUNT 0x00
#define MAX_MIDIINSTRUMENT_COUNT 0x40
#define MAX_OPALINSTRUMENT_COUNT 0x00
#define MAX_MACROINSTRUMENT_COUNT 0x00
#define MAX_INSTRUMENT_COUNT 0x40

#define LOWEST_NOTE 0
#define HIGHEST_NOTE 119
#define NOTE_OFF 0xFE
#define NO_NOTE 0xFF
#define NOTE_C3 60
#define EMPTY_SONG_VALUE 0xFF

#define MIDI_INPUT_CHANNEL_NONE 0
#define MIDI_INPUT_CHANNEL_COUNT 16
#define MIDI_INPUT_MIN_PROGRAM 0x00
#define MIDI_INPUT_DEFAULT_PROGRAM 0x00
#define MIDI_INPUT_MAX_PROGRAM (MAX_INSTRUMENT_COUNT - 1)

class Song : Persistent {
public:
  using InstrumentUsageMap = etl::array<bool, MAX_INSTRUMENT_COUNT>;

  Song();
  ~Song();
  void Reset();

  virtual void SaveContent(tinyxml2::XMLPrinter *printer);
  virtual void RestoreContent(PersistencyDocument *doc);

  void SetMidiInputChannel(int songChannel, uint8_t midiChannel);
  uint8_t GetMidiInputChannel(int songChannel) const;
  bool IsMidiInputChannelAssigned(int songChannel) const;
  int FindAvailableMidiInputSongChannel(uint8_t midiChannel,
                                        uint16_t unavailableChannelMask) const;
  void SetMidiDefaultProgram(int midiChannel, uint8_t program);
  uint8_t GetMidiDefaultProgram(int midiChannel) const;
  uint8_t GetMidiDefaultInstrumentIndex(int midiChannel) const;
  InstrumentUsageMap GetInstrumentUsage() const;

  unsigned char data_[SONG_CHANNEL_COUNT * SONG_ROW_COUNT];
  uint8_t midiInputChannels_[SONG_CHANNEL_COUNT];
  uint8_t midiDefaultPrograms_[MIDI_INPUT_CHANNEL_COUNT];
  Chain chain_;
  Phrase phrase_;
};

#endif
