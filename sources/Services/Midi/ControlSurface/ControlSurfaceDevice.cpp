/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 */

/*
 * ControlSurfaceDevice coordinates a MIDI control surface with tracker state.
 * It binds to runtime services, translates pad and transport input into song
 * and mute actions, keeps the visible song-window viewport in sync with
 * playback and UI state, and tracks cached LED refresh state for the surface.
 */

#include "ControlSurface/ControlSurfaceDevice.h"
#include "Application/Model/Project.h"
#include "Application/Model/Song.h"
#include "Application/Player/Player.h"
#include "Application/Player/SyncMaster.h"
#include "Application/Views/BaseClasses/View.h"
#include "Services/Midi/MidiInDevice.h"
#include "Services/Midi/MidiMessage.h"
#include "Services/Midi/MidiService.h"
#include <algorithm>

uint8_t ControlSurfaceDevice::midiSendBuffer_
    [ControlSurfaceDevice::kMidiSendBufferSize];

ControlSurfaceDevice::ControlSurfaceDevice(int matrixButtonCount)
    : midiService_(nullptr), player_(nullptr), enabled_(true), started_(false),
      ledsDirty_(true), playerObserverAttached_(false),
      viewDataObserverAttached_(false), soloMuteHeld_(false),
      playbackAutoFollowEnabled_(true), internalViewportSyncDepth_(0),
      songWindowOffset_(0), observedViewData_(nullptr),
      matrixButtonCount_(matrixButtonCount) {
  std::fill(std::begin(muteActionHeld_), std::end(muteActionHeld_), false);
}

bool ControlSurfaceDevice::Init(MidiService &midiService) {
  midiService_ = &midiService;
  player_ = Player::GetInstance();
  midiInputs_.clear();
  enabled_ = true;
  started_ = false;
  ledsDirty_ = true;
  playerObserverAttached_ = false;
  viewDataObserverAttached_ = false;
  songWindowOffset_ = 0;
  observedViewData_ = nullptr;
  resetInteractionState();
  return true;
}

bool ControlSurfaceDevice::Start() {
  if (!enabled_ || started_) {
    return true;
  }

  for (auto *device : midiInputs_) {
    if (device != nullptr) {
      device->AddObserver(*this);
    }
  }

  (void)EnsureRuntimeBindings();
  if (player_ != nullptr) {
    const ViewData *viewData = player_->GetViewData();
    if (viewData != nullptr) {
      const int visibleRowStart = viewData->GetSongOffset() + viewData->songY_;
      if ((visibleRowStart >= 0) && (visibleRowStart < SONG_ROW_COUNT)) {
        const uint32_t pageStart = static_cast<uint32_t>(
            (visibleRowStart / matrixButtonCount_) * matrixButtonCount_);
        setSongWindowOffset(pageStart);
      }
    }
  }

  started_ = true;
  RefreshLeds();
  return true;
}

void ControlSurfaceDevice::Stop() {
  if (!started_) {
    return;
  }

  for (auto *device : midiInputs_) {
    if (device != nullptr) {
      device->RemoveObserver(*this);
    }
  }

  if (playerObserverAttached_ && (player_ != nullptr)) {
    player_->RemoveObserver(*this);
  }
  playerObserverAttached_ = false;
  detachViewDataObserver();
  resetInteractionState();
  OnSurfaceStopped();
  started_ = false;
  ledsDirty_ = true;
}

void ControlSurfaceDevice::SetEnabled(bool enabled) {
  if (enabled_ == enabled) {
    return;
  }

  enabled_ = enabled;
  if (!enabled_) {
    Stop();
    return;
  }

  ledsDirty_ = true;
}

void ControlSurfaceDevice::RegisterMidiInput(MidiInDevice &device) {
  for (auto *registered : midiInputs_) {
    if (registered == &device) {
      return;
    }
  }
  midiInputs_.push_back(&device);
}

void ControlSurfaceDevice::OnPlayerEvent(PlayerEvent &event) {
  if (!enabled_) {
    return;
  }

  switch (event.GetType()) {
  case PET_START:
  case PET_STOP:
  case PET_STATE_CHANGE:
    ledsDirty_ = true;
    RefreshLeds();
    break;
  case PET_UPDATE: {
    SyncMaster *sync = SyncMaster::GetInstance();
    if (sync->MajorSlice()) {
      ledsDirty_ = true;
      RefreshLeds();
    }
    break;
  }
  case PET_NOTE_UPDATE:
    // Direct note activity changes the screen visualizer, not surface LEDs.
    break;
  }
}

bool ControlSurfaceDevice::HasSongWindow() const {
  return started_ && enabled_ && (observedViewData_ != nullptr);
}

uint32_t ControlSurfaceDevice::GetSongWindowOffset() const {
  return songWindowOffset_;
}

uint32_t ControlSurfaceDevice::GetSongWindowRowCount() const {
  return static_cast<uint32_t>(matrixButtonCount_);
}

void ControlSurfaceDevice::Update(Observable &o, I_ObservableData *d) {
  if (!started_ || !enabled_) {
    return;
  }

  if ((observedViewData_ != nullptr) && (&o == observedViewData_)) {
    syncSongWindowToAdvanceViewport();
    updatePlaybackFollowStateFromViewportVisibility();
    OnViewDataChanged(d);
    ledsDirty_ = true;
    RefreshLeds();
    return;
  }

  if ((player_ != nullptr) && (&o == player_)) {
    if (d != nullptr) {
      OnPlayerEvent(*static_cast<PlayerEvent *>(d));
    }
    return;
  }

  if (d == nullptr) {
    return;
  }

  for (auto *device : midiInputs_) {
    if (&o == device) {
      (void)OnMidiMessage(*static_cast<MidiMessage *>(d));
      return;
    }
  }
}

bool ControlSurfaceDevice::EnsureRuntimeBindings() {
  player_ = Player::GetInstance();
  if ((player_ != nullptr) && !playerObserverAttached_) {
    player_->AddObserver(*this);
    playerObserverAttached_ = true;
  }
  syncViewDataObserver();
  return player_ != nullptr;
}

void ControlSurfaceDevice::MarkLedsDirty() { ledsDirty_ = true; }

void ControlSurfaceDevice::ClearLedsDirty() { ledsDirty_ = false; }

bool ControlSurfaceDevice::appendMidiMessage(uint16_t &pos, uint8_t status,
                                             uint8_t data1, uint8_t data2) {
  if (static_cast<size_t>(pos + 3) > kMidiSendBufferSize) {
    return false;
  }

  midiSendBuffer_[pos++] = status;
  midiSendBuffer_[pos++] = data1;
  midiSendBuffer_[pos++] = data2;
  return true;
}

void ControlSurfaceDevice::sendBufferedMidiMessages(uint16_t len) {
  if ((midiService_ == nullptr) || (len == 0)) {
    return;
  }

  midiService_->SendControlSurfaceRawImmediate(midiSendBuffer_,
                                               static_cast<uint8_t>(len));
}

bool ControlSurfaceDevice::IsLiveModeActive() const {
  return (player_ != nullptr) && (player_->GetSequencerMode() == SM_LIVE);
}

bool ControlSurfaceDevice::isPress(const MidiMessage &message) const {
  switch (message.status_ & 0xF0) {
  case MidiMessage::MIDI_NOTE_ON:
  case MidiMessage::MIDI_CONTROL_CHANGE:
    return message.data2_ > 0;
  default:
    return false;
  }
}

bool ControlSurfaceDevice::isRelease(const MidiMessage &message) const {
  switch (message.status_ & 0xF0) {
  case MidiMessage::MIDI_NOTE_OFF:
    return true;
  case MidiMessage::MIDI_NOTE_ON:
  case MidiMessage::MIDI_CONTROL_CHANGE:
    return message.data2_ == 0;
  default:
    return false;
  }
}

unsigned char ControlSurfaceDevice::GetSongCellValue(int channel,
                                                     int row) const {
  if ((player_ == nullptr) || (player_->GetProject() == nullptr)) {
    return EMPTY_SONG_VALUE;
  }

  const Song &song = player_->GetProject()->song_;
  const int songRow = SongRowForMatrixRow(row);
  if ((channel < 0) || (channel >= SONG_CHANNEL_COUNT) || (songRow < 0)) {
    return EMPTY_SONG_VALUE;
  }

  return song.data_[SONG_CHANNEL_COUNT * songRow + channel];
}

bool ControlSurfaceDevice::IsSongCellPlayable(int channel, int row) const {
  return GetSongCellValue(channel, row) != EMPTY_SONG_VALUE;
}

bool ControlSurfaceDevice::IsRowPlayable(int row) const {
  for (int channel = 0; channel < matrixButtonCount_; ++channel) {
    if (IsSongCellPlayable(channel, row)) {
      return true;
    }
  }
  return false;
}

void ControlSurfaceDevice::HandleGridPadPress(int channel, int row) {
  if ((player_ == nullptr) || (channel < 0) ||
      (channel >= matrixButtonCount_) || (row < 0) ||
      (row >= matrixButtonCount_)) {
    return;
  }

  const int songRow = SongRowForMatrixRow(row);
  if (songRow < 0) {
    return;
  }

  if (soloMuteHeld_) {
    player_->OnSongStartButton(static_cast<unsigned int>(channel),
                               static_cast<unsigned int>(channel), true, false,
                               MSM_AUDIO, false, songRow);
  } else {
    player_->OnSongStartButton(static_cast<unsigned int>(channel),
                               static_cast<unsigned int>(channel), false, false,
                               MSM_AUDIO, false, songRow);
  }

  ledsDirty_ = true;
  RefreshLeds();
}

void ControlSurfaceDevice::HandleSceneLaunchPress(int row) {
  if ((player_ == nullptr) || (row < 0) || (row >= matrixButtonCount_)) {
    return;
  }

  const int songRow = SongRowForMatrixRow(row);
  if (songRow < 0) {
    return;
  }

  player_->OnSongStartButton(0,
                             static_cast<unsigned int>(matrixButtonCount_ - 1),
                             false, false, MSM_AUDIO, false, songRow);

  ledsDirty_ = true;
  RefreshLeds();
}

void ControlSurfaceDevice::HandleMuteModifierPress() {
  soloMuteHeld_ = true;
  ledsDirty_ = true;
  RefreshLeds();
}

void ControlSurfaceDevice::HandleMuteModifierRelease() {
  if (!soloMuteHeld_) {
    return;
  }

  soloMuteHeld_ = false;
  std::fill(std::begin(muteActionHeld_), std::end(muteActionHeld_), false);
  ledsDirty_ = true;
  RefreshLeds();
}

void ControlSurfaceDevice::HandleMuteActionPress(int channel) {
  if ((player_ == nullptr) || !soloMuteHeld_ || (channel < 0) ||
      (channel >= matrixButtonCount_) ||
      (channel >= static_cast<int>(std::size(muteActionHeld_)))) {
    return;
  }

  toggleMuteChannel(channel);
  muteActionHeld_[channel] = true;
  ledsDirty_ = true;
  RefreshLeds();
}

void ControlSurfaceDevice::HandleMuteActionRelease(int channel) {
  if ((channel < 0) || (channel >= matrixButtonCount_) ||
      (channel >= static_cast<int>(std::size(muteActionHeld_))) ||
      !muteActionHeld_[channel]) {
    return;
  }

  muteActionHeld_[channel] = false;
  if (soloMuteHeld_) {
    toggleMuteChannel(channel);
    ledsDirty_ = true;
    RefreshLeds();
  }
}

void ControlSurfaceDevice::HandleSongWindowFineUpPress() {
  updateSongWindowOffset(-1);
  ledsDirty_ = true;
  RefreshLeds();
}

void ControlSurfaceDevice::HandleSongWindowFineDownPress() {
  updateSongWindowOffset(1);
  ledsDirty_ = true;
  RefreshLeds();
}

void ControlSurfaceDevice::HandleSongWindowPageUpPress() {
  updateSongWindowOffset(-matrixButtonCount_);
  ledsDirty_ = true;
  RefreshLeds();
}

void ControlSurfaceDevice::HandleSongWindowPageDownPress() {
  updateSongWindowOffset(matrixButtonCount_);
  ledsDirty_ = true;
  RefreshLeds();
}

ControlSurfaceDevice::RowState
ControlSurfaceDevice::GetRowState(const ControlSurfaceState &state,
                                  int channelCount, int row) {
  RowState rowState;

  for (int channel = 0; channel < channelCount; ++channel) {
    const auto cell = state.cells[channel][row];
    if (cell == ControlSurfaceState::Cell::Playing)
      rowState.playing = true;
    else if (cell == ControlSurfaceState::Cell::Queued)
      rowState.queued = true;
    else if (cell != ControlSurfaceState::Cell::Empty)
      rowState.playable = true;
  }

  return rowState;
}

void ControlSurfaceDevice::HandleTransportPress() {
  if (player_ == nullptr) {
    return;
  }

  (void)player_->ToggleLiveStartStop();
  ledsDirty_ = true;
  RefreshLeds();
}

void ControlSurfaceDevice::OnViewDataChanged(I_ObservableData *data) {
  (void)data;
}

void ControlSurfaceDevice::OnSurfaceStopped() {}

void ControlSurfaceDevice::syncViewDataObserver() {
  ViewData *viewData = nullptr;
  if (player_ != nullptr) {
    viewData = player_->GetViewData();
  }

  if ((viewData == observedViewData_) &&
      (viewDataObserverAttached_ || (viewData == nullptr))) {
    return;
  }

  detachViewDataObserver();
  observedViewData_ = viewData;

  if (observedViewData_ != nullptr) {
    observedViewData_->AddObserver(*this);
    viewDataObserverAttached_ = true;
  }
}

void ControlSurfaceDevice::detachViewDataObserver() {
  if (viewDataObserverAttached_ && (observedViewData_ != nullptr)) {
    observedViewData_->RemoveObserver(*this);
  }
  viewDataObserverAttached_ = false;
  observedViewData_ = nullptr;
}

void ControlSurfaceDevice::setSongWindowOffset(uint32_t offset) {
  const uint32_t clampedOffset = std::min(offset, getMaxSongWindowOffset());
  if (clampedOffset == songWindowOffset_) {
    return;
  }

  songWindowOffset_ = clampedOffset;
  const int32_t previousAdvanceOffset =
      (observedViewData_ != nullptr)
          ? static_cast<int32_t>(observedViewData_->GetSongOffset())
          : -1;
  ++internalViewportSyncDepth_;
  syncAdvanceViewportToSongWindow();
  if ((observedViewData_ != nullptr) &&
      (static_cast<int32_t>(observedViewData_->GetSongOffset()) ==
       previousAdvanceOffset)) {
    observedViewData_->RefreshSongViewport();
  }
  if (internalViewportSyncDepth_ > 0) {
    --internalViewportSyncDepth_;
  }
}

void ControlSurfaceDevice::updateSongWindowOffset(int32_t delta) {
  int64_t nextOffset = static_cast<int64_t>(songWindowOffset_) + delta;
  if (nextOffset < 0) {
    nextOffset = 0;
  }

  const int64_t maxOffset = static_cast<int64_t>(getMaxSongWindowOffset());
  if (nextOffset > maxOffset) {
    nextOffset = maxOffset;
  }

  setSongWindowOffset(static_cast<uint32_t>(nextOffset));
}

bool ControlSurfaceDevice::getActiveSongRowRange(int32_t &minRow,
                                                 int32_t &maxRow) const {
  if (player_ == nullptr) {
    return false;
  }

  minRow = SONG_ROW_COUNT;
  maxRow = -1;

  for (int channel = 0; channel < matrixButtonCount_; ++channel) {
    if (player_->IsChannelPlaying(channel)) {
      const int32_t row =
          static_cast<int32_t>(player_->GetChannelSongPlayPosition(channel));
      if ((row >= 0) && (row < SONG_ROW_COUNT)) {
        minRow = std::min(minRow, row);
        maxRow = std::max(maxRow, row);
      }
    }
  }

  if (maxRow >= 0) {
    return true;
  }

  for (int channel = 0; channel < matrixButtonCount_; ++channel) {
    if (player_->GetQueueingMode(channel) != QM_NONE) {
      const int32_t row =
          static_cast<int32_t>(player_->GetQueuePosition(channel));
      if ((row >= 0) && (row < SONG_ROW_COUNT)) {
        minRow = std::min(minRow, row);
        maxRow = std::max(maxRow, row);
      }
    }
  }

  return maxRow >= 0;
}

void ControlSurfaceDevice::updatePlaybackFollowStateFromViewportVisibility() {
  if ((player_ == nullptr) || !player_->IsRunning() || !IsLiveModeActive()) {
    playbackAutoFollowEnabled_ = true;
    return;
  }

  if (observedViewData_ == nullptr) {
    playbackAutoFollowEnabled_ = true;
    return;
  }

  int32_t minActiveRow = 0;
  int32_t maxActiveRow = -1;
  if (!getActiveSongRowRange(minActiveRow, maxActiveRow)) {
    playbackAutoFollowEnabled_ = true;
    return;
  }

  const int advanceStart = observedViewData_->GetSongOffset();
  const int advanceEnd = advanceStart + View::songRowCount_ - 1;
  playbackAutoFollowEnabled_ =
      (maxActiveRow >= advanceStart) && (maxActiveRow <= advanceEnd);
}

bool ControlSurfaceDevice::updateSongWindowForPlayback() {
  if ((player_ == nullptr) || !playbackAutoFollowEnabled_) {
    return false;
  }

  int32_t minActiveRow = 0;
  int32_t maxActiveRow = -1;
  if (!getActiveSongRowRange(minActiveRow, maxActiveRow)) {
    return false;
  }

  bool updated = false;

  if (observedViewData_ != nullptr) {
    const int advanceStart = observedViewData_->GetSongOffset();
    const int advanceEnd = advanceStart + View::songRowCount_ - 1;

    if (maxActiveRow > advanceEnd) {
      observedViewData_->SetSongOffset(
          static_cast<uint32_t>(maxActiveRow - View::songRowCount_ + 1));
      updated = true;
    } else if (minActiveRow < advanceStart) {
      observedViewData_->SetSongOffset(static_cast<uint32_t>(minActiveRow));
      updated = true;
    }
  }

  const int32_t windowStart = static_cast<int32_t>(songWindowOffset_);
  const int32_t windowEnd = windowStart + matrixButtonCount_ - 1;

  if (maxActiveRow > windowEnd) {
    setSongWindowOffset(static_cast<uint32_t>(
        std::max(int32_t{0}, maxActiveRow - matrixButtonCount_ + 1)));
    updated = true;
  } else if (minActiveRow < windowStart) {
    setSongWindowOffset(
        static_cast<uint32_t>(std::max(int32_t{0}, minActiveRow)));
    updated = true;
  }

  return updated;
}

void ControlSurfaceDevice::syncSongWindowToAdvanceViewport() {
  if (observedViewData_ == nullptr) {
    return;
  }

  const int advanceStart = observedViewData_->GetSongOffset();
  const int advanceEnd = advanceStart + View::songRowCount_ - 1;
  const int songWindowStart = static_cast<int>(songWindowOffset_);
  const int songWindowEnd = songWindowStart + matrixButtonCount_ - 1;

  if ((songWindowStart >= advanceStart) && (songWindowEnd <= advanceEnd)) {
    return;
  }

  if (songWindowStart < advanceStart) {
    setSongWindowOffset(static_cast<uint32_t>(advanceStart));
    return;
  }

  setSongWindowOffset(
      static_cast<uint32_t>(std::max(0, advanceEnd - matrixButtonCount_ + 1)));
}

void ControlSurfaceDevice::syncAdvanceViewportToSongWindow() {
  if (observedViewData_ == nullptr) {
    return;
  }

  const int advanceStart = observedViewData_->GetSongOffset();
  const int advanceEnd = advanceStart + View::songRowCount_ - 1;
  const int songWindowStart = static_cast<int>(songWindowOffset_);
  const int songWindowEnd = songWindowStart + matrixButtonCount_ - 1;

  if ((songWindowStart >= advanceStart) && (songWindowEnd <= advanceEnd)) {
    return;
  }

  if (songWindowStart < advanceStart) {
    observedViewData_->SetSongOffset(static_cast<uint32_t>(songWindowStart));
    return;
  }

  observedViewData_->SetSongOffset(static_cast<uint32_t>(
      std::max(0, songWindowEnd - View::songRowCount_ + 1)));
}

void ControlSurfaceDevice::toggleMuteChannel(int channel) {
  if ((player_ == nullptr) || (channel < 0) ||
      (channel >= matrixButtonCount_)) {
    return;
  }
  player_->SetChannelMute(channel, !player_->IsChannelMuted(channel));
}

void ControlSurfaceDevice::resetInteractionState() {
  soloMuteHeld_ = false;
  ResetSongWindowControlState();
  internalViewportSyncDepth_ = 0;
  std::fill(std::begin(muteActionHeld_), std::end(muteActionHeld_), false);
}

uint32_t ControlSurfaceDevice::getMaxSongWindowOffset() const {
  const int32_t maxOffset = SONG_ROW_COUNT - matrixButtonCount_;
  return static_cast<uint32_t>(maxOffset > 0 ? maxOffset : 0);
}
