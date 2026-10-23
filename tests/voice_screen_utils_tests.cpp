/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "doctest/doctest.h"

#include "Application/Audio/AudioSample.h"
#include "Application/Instruments/I_Instrument.h"
#include "Application/Views/VoiceScreenUtils.h"

namespace {

class TestInstrumentBase : public I_Instrument {
public:
  TestInstrumentBase() : I_Instrument(&variables_) {}

  bool Init() override { return true; }

  bool Start(int channel, unsigned char note, bool retrigger = true) override {
    (void)channel;
    (void)note;
    (void)retrigger;
    return true;
  }

  void Stop(int channel) override { (void)channel; }

  void OnStart() override {}

  bool IsInitialized() override { return true; }

  bool IsEmpty() override { return false; }

  InstrumentType GetType() override { return IT_SAMPLE_SOURCE; }

  void ProcessCommand(int channel, TrackerCommand cc, ushort value) override {
    (void)channel;
    (void)cc;
    (void)value;
  }

  bool Render(int channel, AudioSample *buffer, int size,
              bool updateTick) override {
    (void)channel;
    (void)buffer;
    (void)size;
    (void)updateTick;
    return false;
  }

  int GetTable() override { return 0; }

  bool GetTableAutomation() override { return false; }

  void GetTableState(TableSaveState &state) override { (void)state; }

  void SetTableState(TableSaveState &state) override { (void)state; }

  etl::ivector<Variable *> *Variables() override { return &variables_; }

private:
  etl::vector<Variable *, 1> variables_;
};

class DefaultVoiceScreenInstrument : public TestInstrumentBase {};

class AudioVoiceScreenInstrument : public TestInstrumentBase {
public:
  bool SupportsAudioVoiceScreen() const override { return true; }
};

} // namespace

TEST_CASE("Audio voice screen support is opt-in per instrument type") {
  DefaultVoiceScreenInstrument legacyInstrument;
  AudioVoiceScreenInstrument audioVoiceInstrument;

  CHECK_FALSE(legacyInstrument.SupportsAudioVoiceScreen());
  CHECK(audioVoiceInstrument.SupportsAudioVoiceScreen());
}

TEST_CASE("Audio voice map marker is only shown on eligible screens") {
  DefaultVoiceScreenInstrument legacyInstrument;
  AudioVoiceScreenInstrument audioVoiceInstrument;

  CHECK_FALSE(ShouldShowAudioVoiceScreenOnMap(VT_INSTRUMENT, nullptr));
  CHECK_FALSE(
      ShouldShowAudioVoiceScreenOnMap(VT_INSTRUMENT, &legacyInstrument));
  CHECK_FALSE(ShouldShowAudioVoiceScreenOnMap(VT_VOICE, &legacyInstrument));
  CHECK_FALSE(
      ShouldShowAudioVoiceScreenOnMap(VT_PHRASE, &audioVoiceInstrument));
  CHECK(ShouldShowAudioVoiceScreenOnMap(VT_INSTRUMENT, &audioVoiceInstrument));
  CHECK(ShouldShowAudioVoiceScreenOnMap(VT_VOICE, &audioVoiceInstrument));
}
