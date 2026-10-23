/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Player/Player.h"
#include "Services/Midi/ControlSurface/LaunchpadMiniMk3Surface.h"
#include "Services/Midi/MidiInDevice.h"
#include "Services/Midi/MidiService.h"

void ResetLaunchpadControlSurfaceTestState();
void SetPlayerSequencerModeForTest(SequencerMode mode);
int GetPlayerSongStartButtonCallCount();
int GetPlayerPlayNoteCallCount();
int GetPlayerStopNoteCallCount();

namespace {

class TestMidiInDevice : public MidiInDevice {
public:
  TestMidiInDevice() : MidiInDevice("test") {}

  bool Start() override { return MidiInDevice::Start(); }
  void Stop() override { MidiInDevice::Stop(); }
  void poll() override {}

  void EmitMessage(MidiMessage &message) { onDriverMessage(message); }

protected:
  bool initDriver() override { return true; }
  void closeDriver() override {}
  bool startDriver() override { return true; }
  void stopDriver() override {}
};

} // namespace

TEST_CASE("Launchpad USB grid notes pass through outside Live mode") {
  ResetLaunchpadControlSurfaceTestState();
  SetPlayerSequencerModeForTest(SM_SONG);

  MidiService midiService;
  LaunchpadMiniMk3Surface surface;
  TestMidiInDevice device;

  REQUIRE(surface.Init(midiService));
  surface.RegisterMidiInput(device);
  REQUIRE(surface.Start());
  REQUIRE(device.Start());

  MidiMessage noteOn(MidiMessage::MIDI_NOTE_ON, 11, 100);
  device.EmitMessage(noteOn);

  CHECK_FALSE(noteOn.IsConsumed());
  CHECK(GetPlayerSongStartButtonCallCount() == 0);
  CHECK(GetPlayerPlayNoteCallCount() == 1);

  MidiMessage noteOff(MidiMessage::MIDI_NOTE_OFF, 11, 0);
  device.EmitMessage(noteOff);

  CHECK_FALSE(noteOff.IsConsumed());
  CHECK(GetPlayerStopNoteCallCount() == 1);
}

TEST_CASE("Launchpad USB grid presses are consumed in Live mode") {
  ResetLaunchpadControlSurfaceTestState();
  SetPlayerSequencerModeForTest(SM_LIVE);

  MidiService midiService;
  LaunchpadMiniMk3Surface surface;
  TestMidiInDevice device;

  REQUIRE(surface.Init(midiService));
  surface.RegisterMidiInput(device);
  REQUIRE(surface.Start());
  REQUIRE(device.Start());

  MidiMessage noteOn(MidiMessage::MIDI_NOTE_ON, 11, 100);
  device.EmitMessage(noteOn);

  CHECK(noteOn.IsConsumed());
  CHECK(GetPlayerSongStartButtonCallCount() == 1);
  CHECK(GetPlayerPlayNoteCallCount() == 0);
}
