/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#include "SelectionResampleSupport.h"
#include "Application/Mixer/MixerService.h"
#include "Application/Model/Chain.h"
#include "Application/Model/Project.h"
#include "Application/Model/Song.h"
#include "Application/Player/Player.h"
#include "Application/Player/SelectionResampleSession.h"
#include "Application/Views/BaseClasses/BrowserPathUtils.h"
#include "Application/Views/BaseClasses/View.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "Application/Views/ModalDialogs/RenderProgressModal.h"
#include "System/FileSystem/PathUtils.h"

namespace SelectionResampleSupport {

namespace {

etl::string<64> projectSamplePathError;

void SetProjectSamplePathError(const char *message) {
  projectSamplePathError = message != nullptr ? message : "";
}

bool SelectionHasPlayableSongContent(const GUIRect &selectionRect,
                                     const ViewData &viewData) {
  if (viewData.song_ == nullptr) {
    return false;
  }

  for (int row = selectionRect.Top();
       row <= selectionRect.Bottom() && row < SONG_ROW_COUNT; ++row) {
    for (int channel = selectionRect.Left();
         channel <= selectionRect.Right() && channel < SONG_CHANNEL_COUNT;
         ++channel) {
      const unsigned char chain =
          viewData.song_->data_[row * SONG_CHANNEL_COUNT + channel];
      if (chain == EMPTY_SONG_VALUE) {
        continue;
      }
      if (viewData.song_->chain_.data_[chain * PHRASES_PER_CHAIN] !=
          EMPTY_SONG_VALUE) {
        return true;
      }
    }
  }

  return false;
}

} // namespace

void ClearProjectSamplePathError() { projectSamplePathError.clear(); }

const char *GetProjectSamplePathError() {
  return projectSamplePathError.empty() ? "" : projectSamplePathError.c_str();
}

bool BuildNextProjectSamplePath(Project *project,
                                SelectionResampleRequest &request) {
  ClearProjectSamplePathError();

  if (project == nullptr) {
    SetProjectSamplePathError("Resample unavailable");
    return false;
  }

  char projectName[MAX_PROJECT_NAME_LENGTH + 1];
  project->GetProjectName(projectName);

  FileSystemPath::PathString samplesDirectory;
  if (!BrowserPathUtils::BuildProjectSamplesDirectory(projectName,
                                                      samplesDirectory)) {
    SetProjectSamplePathError("Sample path unavailable");
    return false;
  }

  FileSystemPath::PathString outputPath;
  const auto pathStatus = BrowserPathUtils::BuildNextIndexedFilePath(
      samplesDirectory.c_str(), "resample", ".wav", 32, outputPath);
  if (pathStatus == BrowserPathUtils::NextIndexedFilePathStatus::Exhausted) {
    SetProjectSamplePathError("Resample slots full");
    return false;
  }
  if (pathStatus != BrowserPathUtils::NextIndexedFilePathStatus::Found) {
    SetProjectSamplePathError("Sample path unavailable");
    return false;
  }

  request.outputPath = outputPath.c_str();
  if (request.outputPath.is_truncated()) {
    SetProjectSamplePathError("Sample path too long");
    return false;
  }

  return true;
}

bool BuildNextMixdownPath(SelectionResampleRequest &request) {
  ClearProjectSamplePathError();

  FileSystemPath::PathString outputPath;
  const auto pathStatus = BrowserPathUtils::BuildNextIndexedFilePath(
      RENDERS_DIR, "mixdown", ".wav", 99, outputPath);
  if (pathStatus == BrowserPathUtils::NextIndexedFilePathStatus::Exhausted) {
    SetProjectSamplePathError("Mixdown slots full");
    return false;
  }
  if (pathStatus != BrowserPathUtils::NextIndexedFilePathStatus::Found) {
    SetProjectSamplePathError("Render path unavailable");
    return false;
  }

  request.outputPath = outputPath.c_str();
  if (request.outputPath.is_truncated()) {
    SetProjectSamplePathError("Render path too long");
    return false;
  }

  return true;
}

bool BuildSelectionRequest(const GUIRect &selectionRect,
                           const ViewData &viewData,
                           SelectionResampleRequest &request,
                           SelectionOutputIntent outputIntent) {
  ClearProjectSamplePathError();

  if (viewData.song_ == nullptr || viewData.project_ == nullptr) {
    SetProjectSamplePathError("Resample unavailable");
    return false;
  }
  if (!SelectionHasPlayableSongContent(selectionRect, viewData)) {
    SetProjectSamplePathError("Selection is not playable");
    return false;
  }

  request = SelectionResampleRequest();
  request.sourceKind = SelectionResampleSourceKind::Song;
  request.outputIntent = outputIntent;
  request.startSongRow = static_cast<uint8_t>(selectionRect.Top());
  request.endSongRow = static_cast<uint8_t>(selectionRect.Bottom());
  request.channelStart = static_cast<uint8_t>(selectionRect.Left());
  request.channelEnd = static_cast<uint8_t>(selectionRect.Right());
  request.trackerChannel = static_cast<uint8_t>(viewData.songX_);

  switch (outputIntent) {
  case SelectionOutputIntent::ProjectSample:
    return BuildNextProjectSamplePath(viewData.project_, request);
  case SelectionOutputIntent::Mixdown:
    return BuildNextMixdownPath(request);
  case SelectionOutputIntent::Stems:
    return true;
  }

  SetProjectSamplePathError("Resample unavailable");
  return false;
}

MixerServiceMode GetMixerServiceMode(SelectionOutputIntent outputIntent) {
  switch (outputIntent) {
  case SelectionOutputIntent::Stems:
    return MSM_FILESPLIT;
  case SelectionOutputIntent::ProjectSample:
  case SelectionOutputIntent::Mixdown:
    return MSM_FILE;
  }

  return MSM_FILE;
}

void ShowResampleError(View &view, const char *message) {
  MessageBox *messageBox = MessageBox::Create(
      view, message != nullptr ? message : "Resample failed", MBBF_OK);
  view.DoModal(messageBox);
}

bool StartSelectionResample(View &view,
                            const SelectionResampleRequest &request) {
  Player *player = Player::GetInstance();
  if (player == nullptr) {
    ShowResampleError(view, "Resample unavailable");
    return false;
  }
  if (player->IsRunning()) {
    ShowResampleError(view, "Stop playback first");
    return false;
  }

  if (!SelectionResampleSession::Begin(request)) {
    const char *error = SelectionResampleSession::GetLastError();
    ShowResampleError(view, error[0] != '\0' ? error : "Resample failed");
    return false;
  }

  const MixerServiceMode mixerServiceMode =
      GetMixerServiceMode(request.outputIntent);
  const auto progressMode =
      request.sourceKind == SelectionResampleSourceKind::Song
          ? RenderProgressModal::ProgressDisplayMode::SongPercent
          : RenderProgressModal::ProgressDisplayMode::ElapsedTime;
  RenderProgressModal *renderDialog =
      RenderProgressModal::Create(view, "Rendering", "", progressMode);
  view.DoModal(renderDialog);
  player->Start(request.GetPlayMode(), false, mixerServiceMode, false);
  return true;
}

} // namespace SelectionResampleSupport
