/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Services/Midi/MidiInDevice.h"

void ResetPlayerMidiInputTestState();
int GetPlayerSongStartButtonCallCount();
int GetPlayerPlayNoteCallCount();

namespace {

class CountingObserver : public I_Observer {
public:
  void Update(Observable &o, I_ObservableData *d) override {
    observed_ = &o;
    payload_ = d;
    notificationCount_++;
  }

  int notificationCount_ = 0;
  Observable *observed_ = nullptr;
  I_ObservableData *payload_ = nullptr;
};

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

TEST_CASE("MidiInDevice does not start playback while stopped") {
  ResetPlayerMidiInputTestState();

  TestMidiInDevice device;
  CountingObserver observer;
  MidiMessage message(MidiMessage::MIDI_START);

  device.AddObserver(observer);
  device.EmitMessage(message);

  CHECK(observer.notificationCount_ == 1);
  CHECK(observer.payload_ == &message);
  CHECK(GetPlayerSongStartButtonCallCount() == 0);

  REQUIRE(device.Start());
  device.EmitMessage(message);
  CHECK(GetPlayerSongStartButtonCallCount() == 1);

  device.Stop();
  device.EmitMessage(message);
  CHECK(observer.notificationCount_ == 4);
  CHECK(GetPlayerSongStartButtonCallCount() == 1);
}

TEST_CASE("MidiInDevice does not play notes while stopped") {
  ResetPlayerMidiInputTestState();

  TestMidiInDevice device;
  MidiMessage noteOn(MidiMessage::MIDI_NOTE_ON, 60, 100);

  device.EmitMessage(noteOn);
  CHECK(GetPlayerPlayNoteCallCount() == 0);

  REQUIRE(device.Start());
  device.EmitMessage(noteOn);
  CHECK(GetPlayerPlayNoteCallCount() == 1);

  device.Stop();
  device.EmitMessage(noteOn);
  CHECK(GetPlayerPlayNoteCallCount() == 1);
}
