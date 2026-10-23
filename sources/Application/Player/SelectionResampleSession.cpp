/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "SelectionResampleSession.h"
#include "Application/Instruments/SamplePool.h"
#include "Application/Mixer/MixerService.h"
#include "Player.h"
#include "System/FileSystem/FileSystem.h"

namespace {

struct SelectionResampleSessionState {
  bool active = false;
  bool canceled = false;
  bool finalized = false;
  bool muteSnapshotActive = false;
  bool originalMuteState[SONG_CHANNEL_COUNT] = {};
  SelectionResampleRequest request;
  etl::string<64> lastError;
};

SelectionResampleSessionState &GetSessionState() {
  static SelectionResampleSessionState state;
  return state;
}

void setLastError(const char *message) {
  auto &state = GetSessionState();
  state.lastError = message ? message : "";
}

void clearLastError() { GetSessionState().lastError.clear(); }

void restoreMutedChannels(Player *player) {
  auto &state = GetSessionState();
  if (!state.muteSnapshotActive || player == nullptr) {
    state.muteSnapshotActive = false;
    return;
  }

  for (int channel = 0; channel < SONG_CHANNEL_COUNT; ++channel) {
    player->SetChannelMute(channel, state.originalMuteState[channel]);
  }
  state.muteSnapshotActive = false;
}

void cleanupOutputFile() {
  auto &state = GetSessionState();
  if (state.request.outputPath.empty()) {
    return;
  }

  FileSystem *fs = FileSystem::GetInstance();
  if (fs && fs->exists(state.request.outputPath.c_str())) {
    fs->DeleteFile(state.request.outputPath.c_str());
  }
}

void clearSession(Player *player) {
  restoreMutedChannels(player);
  if (player != nullptr) {
    player->ClearSelectionRender();
  }

  auto &state = GetSessionState();
  state.active = false;
  state.canceled = false;
  state.finalized = false;
  state.request = SelectionResampleRequest();
}

bool registerRenderedSample() {
  auto &state = GetSessionState();

  SamplePool *samplePool = SamplePool::GetInstance();
  if (samplePool->LoadProjectSample(state.request.outputPath.c_str()) >= 0) {
    return true;
  }

  setLastError("Sample registration failed");
  cleanupOutputFile();
  return false;
}

} // namespace

namespace SelectionResampleSession {

bool Begin(const SelectionResampleRequest &request) {
  auto &state = GetSessionState();
  Player *player = Player::GetInstance();
  MixerService *mixerService = MixerService::GetInstance();
  if (state.active || player == nullptr || mixerService == nullptr) {
    return false;
  }
  if (player->IsRunning()) {
    setLastError("Player already running");
    return false;
  }
  if (request.outputIntent != SelectionOutputIntent::Stems &&
      (request.outputPath.empty() || request.outputPath.is_truncated())) {
    setLastError("Invalid output path");
    return false;
  }
  if (request.outputIntent != SelectionOutputIntent::Stems &&
      !mixerService->SetRenderTargetOverride(request.outputPath.c_str())) {
    setLastError("Render path setup failed");
    return false;
  }

  clearLastError();
  state.active = true;
  state.canceled = false;
  state.finalized = false;
  state.request = request;

  player->ConfigureSelectionRender(request);

  if (request.sourceKind == SelectionResampleSourceKind::Song) {
    state.muteSnapshotActive = true;
    const uint16_t mask = request.GetSongChannelMask();
    for (int channel = 0; channel < SONG_CHANNEL_COUNT; ++channel) {
      state.originalMuteState[channel] = player->IsChannelMuted(channel);
      const bool shouldMute = (mask & (1U << channel)) == 0;
      player->SetChannelMute(channel, shouldMute);
    }
  }

  return true;
}

void MarkCanceled() {
  auto &state = GetSessionState();
  if (state.active) {
    state.canceled = true;
  }
}

SelectionResampleFinalizeStatus FinalizeIfStopped() {
  auto &state = GetSessionState();
  Player *player = Player::GetInstance();
  if (!state.active || state.finalized) {
    return SelectionResampleFinalizeStatus::None;
  }
  if (player != nullptr && player->IsRunning()) {
    return SelectionResampleFinalizeStatus::None;
  }

  state.finalized = true;
  restoreMutedChannels(player);

  if (state.canceled) {
    cleanupOutputFile();
    clearSession(player);
    return SelectionResampleFinalizeStatus::Cancelled;
  }

  if (state.request.outputIntent == SelectionOutputIntent::Stems) {
    clearSession(player);
    return SelectionResampleFinalizeStatus::Completed;
  }

  FileSystem *fs = FileSystem::GetInstance();
  const bool outputExists =
      fs != nullptr && fs->exists(state.request.outputPath.c_str());

  if (!outputExists) {
    setLastError("Render output missing");
    clearSession(player);
    return SelectionResampleFinalizeStatus::Failed;
  }

  if (state.request.outputIntent == SelectionOutputIntent::Mixdown) {
    clearSession(player);
    return SelectionResampleFinalizeStatus::Completed;
  }

  if (registerRenderedSample()) {
    clearSession(player);
    return SelectionResampleFinalizeStatus::Completed;
  }

  clearSession(player);
  return SelectionResampleFinalizeStatus::Failed;
}

bool GetActiveRequest(SelectionResampleRequest &request) {
  auto &state = GetSessionState();
  if (!state.active) {
    return false;
  }

  request = state.request;
  return true;
}

bool IsActive() { return GetSessionState().active; }

bool IsCanceled() {
  return GetSessionState().active && GetSessionState().canceled;
}

const char *GetLastError() {
  return GetSessionState().lastError.empty()
             ? ""
             : GetSessionState().lastError.c_str();
}

} // namespace SelectionResampleSession
