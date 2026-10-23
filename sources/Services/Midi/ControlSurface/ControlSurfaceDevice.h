/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#pragma once

#include "ControlSurfaceState.h"
#include "Externals/etl/include/etl/vector.h"
#include "Foundation/Observable.h"
#include "IControlSurface.h"
#include <stdint.h>

class MidiInDevice;
class MidiService;
class Player;
class ViewData;
class PlayerEvent;
struct MidiMessage;

class ControlSurfaceDevice : public IControlSurface, public I_Observer {
public:
  struct RowState {
    bool playing = false;
    bool queued = false;
    bool playable = false;
  };

  explicit ControlSurfaceDevice(int matrixButtonCount);
  ~ControlSurfaceDevice() override = default;

  bool Init(MidiService &midiService) override;
  bool Start() override;
  void Stop() override;
  void SetEnabled(bool enabled) override;
  void RegisterMidiInput(MidiInDevice &device) override;
  void OnPlayerEvent(PlayerEvent &event) override;
  bool HasSongWindow() const override;
  uint32_t GetSongWindowOffset() const override;
  uint32_t GetSongWindowRowCount() const override;

  void Update(Observable &o, I_ObservableData *d) override;

  inline bool IsEnabled() const { return enabled_; }
  inline bool IsStarted() const { return started_; }
  inline bool AreLedsDirty() const { return ledsDirty_; }
  bool IsLiveModeActive() const;
  inline bool IsSoloMuteHeld() const { return soloMuteHeld_; }
  inline bool IsMuteActionHeld(int channel) const {
    if ((channel < 0) || (channel >= matrixButtonCount_) ||
        (channel >= static_cast<int>(sizeof(muteActionHeld_) /
                                     sizeof(muteActionHeld_[0])))) {
      return false;
    }
    return muteActionHeld_[channel];
  }
  inline int GetMatrixButtonCount() const { return matrixButtonCount_; }
  inline int SongRowForMatrixRow(int row) const {
    if ((row < 0) || (row >= matrixButtonCount_)) {
      return -1;
    }

    const uint32_t songRow = songWindowOffset_ + static_cast<uint32_t>(row);
    if (songRow >= 256) { // SONG_ROW_COUNT is likely 256
      return -1;
    }
    return static_cast<int>(songRow);
  }

  inline Player *GetPlayer() const { return player_; }
  inline MidiService *GetMidiService() const { return midiService_; }

protected:
  bool EnsureRuntimeBindings();
  inline void ResetSongWindowControlState() {
    playbackAutoFollowEnabled_ = true;
  }
  void MarkLedsDirty();
  void ClearLedsDirty();
  bool appendMidiMessage(uint16_t &pos, uint8_t status, uint8_t data1,
                         uint8_t data2);
  void sendBufferedMidiMessages(uint16_t len);

  unsigned char GetSongCellValue(int channel, int row) const;
  bool IsSongCellPlayable(int channel, int row) const;
  bool IsRowPlayable(int row) const;

  void HandleGridPadPress(int channel, int row);
  void HandleSceneLaunchPress(int row);
  void HandleMuteModifierPress();
  void HandleMuteModifierRelease();
  void HandleMuteActionPress(int channel);
  void HandleMuteActionRelease(int channel);
  void HandleSongWindowFineUpPress();
  void HandleSongWindowFineDownPress();
  void HandleSongWindowPageUpPress();
  void HandleSongWindowPageDownPress();
  void HandleTransportPress();

  static RowState GetRowState(const ControlSurfaceState &state,
                              int channelCount, int row);

  bool isPress(const MidiMessage &message) const;
  bool isRelease(const MidiMessage &message) const;

  virtual void OnViewDataChanged(I_ObservableData *data);
  virtual void OnSurfaceStopped();

private:
  void syncViewDataObserver();
  void detachViewDataObserver();
  void setSongWindowOffset(uint32_t offset);
  void updateSongWindowOffset(int32_t delta);
  bool getActiveSongRowRange(int32_t &minRow, int32_t &maxRow) const;
  void updatePlaybackFollowStateFromViewportVisibility();
  void syncSongWindowToAdvanceViewport();
  void syncAdvanceViewportToSongWindow();
  void toggleMuteChannel(int channel);
  void resetInteractionState();
  bool updateSongWindowForPlayback();
  uint32_t getMaxSongWindowOffset() const;

  static constexpr uint16_t kMidiSendBufferSize = 256;
  // buffer shared by all control surface subclasses because we only support one
  // at a time
  static uint8_t midiSendBuffer_[kMidiSendBufferSize];

private:
  MidiService *midiService_;
  Player *player_;
  etl::vector<MidiInDevice *, 2> midiInputs_;
  bool enabled_;
  bool started_;
  bool ledsDirty_;
  bool playerObserverAttached_;
  bool viewDataObserverAttached_;
  bool soloMuteHeld_;
  bool playbackAutoFollowEnabled_;
  uint8_t internalViewportSyncDepth_;
  uint32_t songWindowOffset_;
  ViewData *observedViewData_;
  bool muteActionHeld_[8];
  int matrixButtonCount_;
};
