/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "Adapters/adv/Core/Inc/firmware_info.h"
#include "Application/Audio/AudioFileStreamer.h"
#include "Application/Audio/RecordStreamer.h"
#include "Application/Player/Player.h"
#include "Application/Player/PlayerMixer.h"
#include "Services/Midi/ControlSurface/ApcMiniMk2Surface.h"
#include "Services/Midi/ControlSurface/ControlSurfaceDevice.h"
#include "Services/Midi/ControlSurface/LaunchpadMiniMk3Surface.h"
#include "Services/Midi/MidiService.h"
#include <cstddef>

extern "C" const firmware_info_t firmware_info = {
    .magic = 0xC0D31355,
    .length = 0,
    .version = 0x72020301,
    .crc32 = 0,
    .type = 0x00006677,
};

namespace {
constexpr size_t kQueuedMidiMessageCapacity = 64;
MidiMessage gQueuedMidiMessages[kQueuedMidiMessageCapacity];
size_t gQueuedMidiMessageCount = 0;
int gPlayerSongStartButtonCallCount = 0;
int gPlayerStopCallCount = 0;
int gPlayerPlayNoteCallCount = 0;
int gPlayerStopNoteCallCount = 0;
bool gPlayerRunning = false;
} // namespace

void ClearQueuedMidiMessagesForTest() { gQueuedMidiMessageCount = 0; }

size_t GetQueuedMidiMessageCountForTest() { return gQueuedMidiMessageCount; }

const MidiMessage &GetQueuedMidiMessageForTest(size_t index) {
  return gQueuedMidiMessages[index];
}

void ResetPlayerMidiInputTestState() {
  gPlayerSongStartButtonCallCount = 0;
  gPlayerStopCallCount = 0;
  gPlayerPlayNoteCallCount = 0;
  gPlayerStopNoteCallCount = 0;
  gPlayerRunning = false;
}

int GetPlayerSongStartButtonCallCount() {
  return gPlayerSongStartButtonCallCount;
}

int GetPlayerStopCallCount() { return gPlayerStopCallCount; }

int GetPlayerPlayNoteCallCount() { return gPlayerPlayNoteCallCount; }

int GetPlayerStopNoteCallCount() { return gPlayerStopNoteCallCount; }

MidiService::MidiService() {}

MidiService::~MidiService() {}

void MidiService::Update(Observable &o, I_ObservableData *d) {
  (void)o;
  (void)d;
}

void MidiService::QueueMessage(MidiMessage &message) {
  if (gQueuedMidiMessageCount < kQueuedMidiMessageCapacity) {
    gQueuedMidiMessages[gQueuedMidiMessageCount++] = message;
  }
}

void MidiService::RegisterActiveChannel(uint8_t channel) { (void)channel; }

void MidiService::registerControlSurfaceInputs(IControlSurface &surface) {
  (void)surface;
}

AudioFileStreamer::AudioFileStreamer() : project_(nullptr) {}

AudioFileStreamer::~AudioFileStreamer() {}

bool AudioFileStreamer::Render(AudioSample *buffer, int samplecount) {
  (void)buffer;
  (void)samplecount;
  return false;
}

RecordStreamer::RecordStreamer()
    : srcBuffer_(nullptr), bufferSize_(0), stereo_(false), mode_(RSM_STOPPED),
      srcBufferPos_(0) {}

RecordStreamer::~RecordStreamer() {}

bool RecordStreamer::Render(AudioSample *buffer, int samplecount) {
  (void)buffer;
  (void)samplecount;
  return false;
}

PlayerMixer::PlayerMixer() {}

void PlayerMixer::Update(Observable &o, I_ObservableData *d) {
  (void)o;
  (void)d;
}

bool PlayerMixer::IsStreamingActive() { return false; }

void PlayerMixer::Lock() {}

void PlayerMixer::Unlock() {}

Player::Player() {}

void Player::Update(Observable &o, I_ObservableData *d) {
  (void)o;
  (void)d;
}

void Player::OnSongStartButton(unsigned int from, unsigned int to,
                               bool requestStop, bool forceImmediate,
                               MixerServiceMode msmMode, bool stopAtEnd,
                               int useStartRow) {
  (void)from;
  (void)to;
  (void)requestStop;
  (void)forceImmediate;
  (void)msmMode;
  (void)stopAtEnd;
  (void)useStartRow;
  gPlayerSongStartButtonCallCount++;
  gPlayerRunning = true;
}

bool Player::IsRunning() { return gPlayerRunning; }

void Player::Stop(PlayerStopMode stopMode) {
  (void)stopMode;
  gPlayerStopCallCount++;
  gPlayerRunning = false;
}

void Player::PlayNote(unsigned short instrumentIndex, unsigned short channel,
                      unsigned char note, unsigned char velocity) {
  (void)instrumentIndex;
  (void)channel;
  (void)note;
  (void)velocity;
  gPlayerPlayNoteCallCount++;
}

void Player::StopNote(unsigned short instrumentIndex, unsigned short channel) {
  (void)instrumentIndex;
  (void)channel;
  gPlayerStopNoteCallCount++;
}
bool Player::IsStreamingActive() { return false; }

ControlSurfaceDevice::ControlSurfaceDevice(int matrixButtonCount)
    : matrixButtonCount_(matrixButtonCount) {}

bool ControlSurfaceDevice::Init(MidiService &midiService) {
  (void)midiService;
  return true;
}

bool ControlSurfaceDevice::Start() { return true; }

void ControlSurfaceDevice::Stop() {}

void ControlSurfaceDevice::SetEnabled(bool enabled) { (void)enabled; }

void ControlSurfaceDevice::RegisterMidiInput(MidiInDevice &device) {
  (void)device;
}

void ControlSurfaceDevice::OnPlayerEvent(PlayerEvent &event) { (void)event; }

void ControlSurfaceDevice::OnSurfaceStopped() {}

void ControlSurfaceDevice::OnViewDataChanged(I_ObservableData *data) {
  (void)data;
}

bool ControlSurfaceDevice::HasSongWindow() const { return false; }

uint32_t ControlSurfaceDevice::GetSongWindowOffset() const { return 0; }

uint32_t ControlSurfaceDevice::GetSongWindowRowCount() const { return 0; }

void ControlSurfaceDevice::Update(Observable &o, I_ObservableData *d) {
  (void)o;
  (void)d;
}

LaunchpadMiniMk3Surface::LaunchpadMiniMk3Surface() : ControlSurfaceDevice(8) {}

LaunchpadMiniMk3Surface::~LaunchpadMiniMk3Surface() {}

bool LaunchpadMiniMk3Surface::Init(MidiService &midiService) {
  (void)midiService;
  return true;
}

bool LaunchpadMiniMk3Surface::OnMidiMessage(MidiMessage &message) {
  (void)message;
  return false;
}

void LaunchpadMiniMk3Surface::RefreshLeds() {}

void LaunchpadMiniMk3Surface::OnSurfaceStopped() {}

void LaunchpadMiniMk3Surface::OnViewDataChanged(I_ObservableData *data) {
  (void)data;
}

ApcMiniMk2Surface::ApcMiniMk2Surface() : ControlSurfaceDevice(8) {}

ApcMiniMk2Surface::~ApcMiniMk2Surface() {}

bool ApcMiniMk2Surface::Init(MidiService &midiService) {
  (void)midiService;
  return true;
}

bool ApcMiniMk2Surface::OnMidiMessage(MidiMessage &message) {
  (void)message;
  return false;
}

void ApcMiniMk2Surface::RefreshLeds() {}

void ApcMiniMk2Surface::OnSurfaceStopped() {}

void ApcMiniMk2Surface::OnViewDataChanged(I_ObservableData *data) {
  (void)data;
}
