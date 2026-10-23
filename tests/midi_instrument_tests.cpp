/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#include "doctest/doctest.h"

#include "Application/Audio/AudioSample.h"
#include "Application/Instruments/MidiInstrument.h"
#include "Services/Midi/MidiService.h"
#include <array>
#include <cstddef>

void ClearQueuedMidiMessagesForTest();
size_t GetQueuedMidiMessageCountForTest();
const MidiMessage &GetQueuedMidiMessageForTest(size_t index);

namespace {
struct MidiServiceScope {
  MidiService service;

  MidiServiceScope() {
    MidiService::Install(&service);
    ClearQueuedMidiMessagesForTest();
  }

  ~MidiServiceScope() {
    ClearQueuedMidiMessagesForTest();
    MidiService::Install(nullptr);
  }
};
} // namespace

TEST_CASE("MidiInstrument continue sends note on before previous note off") {
  MidiServiceScope midiScope;
  MidiInstrument instrument;
  std::array<AudioSample, 2> buffer = {};

  REQUIRE(instrument.Init());
  REQUIRE(instrument.Start(0, 60, true));
  REQUIRE_FALSE(instrument.Render(0, buffer.data(), 1, false));

  ClearQueuedMidiMessagesForTest();

  REQUIRE(instrument.Start(0, 62, false));
  REQUIRE_FALSE(instrument.Render(0, buffer.data(), 1, false));

  REQUIRE(GetQueuedMidiMessageCountForTest() == 2);

  const MidiMessage &noteOn = GetQueuedMidiMessageForTest(0);
  CHECK(noteOn.status_ == MidiMessage::MIDI_NOTE_ON);
  CHECK(noteOn.data1_ == 62);
  CHECK(noteOn.data2_ == INITIAL_NOTE_VELOCITY);

  const MidiMessage &noteOff = GetQueuedMidiMessageForTest(1);
  CHECK(noteOff.status_ == MidiMessage::MIDI_NOTE_OFF);
  CHECK(noteOff.data1_ == 60);
  CHECK(noteOff.data2_ == 0x00);
}

TEST_CASE("MidiInstrument continue on same note is a no-op") {
  MidiServiceScope midiScope;
  MidiInstrument instrument;
  std::array<AudioSample, 2> buffer = {};

  REQUIRE(instrument.Init());
  REQUIRE(instrument.Start(0, 60, true));
  REQUIRE_FALSE(instrument.Render(0, buffer.data(), 1, false));

  ClearQueuedMidiMessagesForTest();

  REQUIRE(instrument.Start(0, 60, false));
  REQUIRE_FALSE(instrument.Render(0, buffer.data(), 1, false));

  CHECK(GetQueuedMidiMessageCountForTest() == 0);
}

TEST_CASE("MidiInstrument TLN command updates remaining ticks") {
  MidiServiceScope midiScope;
  MidiInstrument instrument;
  std::array<AudioSample, 2> buffer = {};

  REQUIRE(instrument.Init());
  REQUIRE(instrument.Start(0, 60, true));
  REQUIRE_FALSE(instrument.Render(0, buffer.data(), 1, false));

  instrument.ProcessCommand(0, TrackerCommand::InstrumentCommandGateLength, 6);

  ClearQueuedMidiMessagesForTest();

  // Tick 1 to 6 - should still be playing
  for (int i = 1; i <= 6; ++i) {
    REQUIRE_FALSE(instrument.Render(0, buffer.data(), 1, false)); // Should not advance
    CHECK(GetQueuedMidiMessageCountForTest() == 0);
    
    REQUIRE_FALSE(instrument.Render(0, buffer.data(), 1, true));
    CHECK(GetQueuedMidiMessageCountForTest() == 0);
  }

  // Tick 7 - should stop
  REQUIRE_FALSE(instrument.Render(0, buffer.data(), 1, false)); // Should not advance
  CHECK(GetQueuedMidiMessageCountForTest() == 0);
  
  REQUIRE_FALSE(instrument.Render(0, buffer.data(), 1, true));
  CHECK(GetQueuedMidiMessageCountForTest() == 1);

  const MidiMessage &noteOff = GetQueuedMidiMessageForTest(0);
  CHECK(noteOff.status_ == MidiMessage::MIDI_NOTE_OFF);
  CHECK(noteOff.data1_ == 60);
}
