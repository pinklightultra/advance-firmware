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

#include "RenderProgressModal.h"
#include "Application/AppWindow.h"
#include "Application/Player/Player.h"
#include "Application/Player/SelectionResampleSession.h"
#include "Application/Views/BaseClasses/View.h"
#include "Application/Views/ModalDialogs/MessageBox.h"
#include "UIFramework/BasicDatas/GUIPoint.h"
#include <cstdint>
#include <new>
#include <stdio.h>

bool RenderProgressModal::inUse_ = false;
alignas(RenderProgressModal) static unsigned char RenderProgressModalStorage
    [sizeof(RenderProgressModal)];
void *RenderProgressModal::storage_ = RenderProgressModalStorage;

RenderProgressModal *
RenderProgressModal::Create(View &view, const char *title, const char *message,
                            ProgressDisplayMode progressDisplayMode) {
  if (inUse_) {
    auto *existing = reinterpret_cast<RenderProgressModal *>(storage_);
    existing->~RenderProgressModal();
    inUse_ = false;
  }
  inUse_ = true;
  return new (storage_)
      RenderProgressModal(view, title, message, progressDisplayMode);
}

RenderProgressModal::RenderProgressModal(
    View &view, const char *title, const char *message,
    ProgressDisplayMode progressDisplayMode)
    : ModalView(view), title_(title), message_(message), totalSamples_(0.0f),
      progressDisplayMode_(progressDisplayMode) {
  dialogWidth_ = title_.size();
  if (message_.size() > dialogWidth_) {
    dialogWidth_ = message_.size();
  }
  if (dialogWidth_ < 16u) {
    dialogWidth_ = 16u;
  }
}

RenderProgressModal::~RenderProgressModal() {}

void RenderProgressModal::Destroy() {
  this->~RenderProgressModal();
  inUse_ = false;
}

void RenderProgressModal::DrawView() {
  // Calculate window size
  const uint32_t width = getDialogWidth();
  SetWindow(width, 5); // Height of 5 for title, message, time/progress, and
                       // button with blank line sep

  // Draw title
  int32_t y = 0;
  int32_t x = (width - title_.size()) / 2;
  GUITextProperties props;
  SetColor(CD_WARN);
  DrawString(x, y, title_.c_str(), props);

  // Draw message
  y++;
  x = (width - message_.size()) / 2;
  DrawString(x, y, message_.c_str(), props);

  // Draw render progress
  y++;
  GUIPoint progressPos(width / 2 - 2, y); // Center the progress display
  drawRenderProgress(progressPos, props);

  // Draw action button
  SetColor(CD_NORMAL);
  y += 2;
  props.invert_ = true;
  // Use a fixed-width label area to avoid stale characters when label shrinks.
  x = width / 2 - 3;
  DrawString(x, y, renderComplete_ ? "  OK  " : "Cancel", props);
}

void RenderProgressModal::OnPlayerUpdate(PlayerEventType eventType,
                                         unsigned int currentTick) {
  (void)eventType;
  (void)currentTick;
}

void RenderProgressModal::OnFocus() {}

void RenderProgressModal::ProcessButtonMask(unsigned short mask, bool pressed) {
  if (mask & EPBM_ENTER && pressed) {
    Player *player = Player::GetInstance();
    if (player && player->IsRunning()) {
      if (SelectionResampleSession::IsActive()) {
        SelectionResampleSession::MarkCanceled();
      }
      player->Stop();
      return;
    }

    EndModal(MBL_OK);
    return;
  }

  isDirty_ = true;
}

void RenderProgressModal::AnimationUpdate() {
  // This runs on core0 (UI thread). Keep updating progress every UI tick
  // while rendering so the dialog stays responsive even if PET_UPDATE events
  // are sparse during stems.
  Player *player = Player::GetInstance();
  const bool isRunning = player && player->IsRunning();

  if (isRunning) {
    renderStarted_ = true;
    if (progressDisplayMode_ == ProgressDisplayMode::ElapsedTime) {
      totalSamples_ = player->GetPlayTime() * SAMPLE_RATE;
    } else {
      SelectionResampleRequest request;
      if (!selectionBoundsCaptured_ &&
          SelectionResampleSession::GetActiveRequest(request) &&
          request.sourceKind == SelectionResampleSourceKind::Song) {
        startSongRow_ = static_cast<int>(request.startSongRow);
        endSongRow_ = static_cast<int>(request.endSongRow);
        selectionBoundsCaptured_ = true;
      }
    }
    isDirty_ = true;
  } else if (renderStarted_ && !renderComplete_) {
    const SelectionResampleFinalizeStatus finalizeStatus =
        SelectionResampleSession::FinalizeIfStopped();
    renderComplete_ = true;
    switch (finalizeStatus) {
    case SelectionResampleFinalizeStatus::Completed:
      message_ = "Render Complete!";
      break;
    case SelectionResampleFinalizeStatus::Cancelled:
      message_ = "Render Cancelled";
      break;
    case SelectionResampleFinalizeStatus::Failed:
      message_ = "Render Failed";
      break;
    case SelectionResampleFinalizeStatus::None:
      message_ = "Render Complete!";
      break;
    }

    // Importing the rendered WAV into the sample pool uses the global Status
    // renderer, which draws directly into the screen buffer. Force a full view
    // redraw on the next UI tick so the modal is rebuilt cleanly afterwards.
    static_cast<AppWindow &>(w_).SetDirty();
    isDirty_ = true;
  }

  if (!isDirty_) {
    return;
  }
  isDirty_ = false;

  const uint32_t width = getDialogWidth();
  int32_t y = 2;
  GUITextProperties props;

  if (renderComplete_) {
    ClearTextRect(0, y - 1, width, 1);
    SetColor(CD_INFO);
    int32_t x = (width - message_.size()) / 2;
    DrawString(x, y - 1, message_.c_str(), props);
  }

  GUIPoint progressPos(width / 2 - 2, y);
  SetColor(CD_NORMAL);
  drawRenderProgress(progressPos, props);

  // Keep button label in sync with render state.
  ClearTextRect(0, y + 1, width, 1);
  props.invert_ = true;
  DrawString(width / 2 - 3, y + 2, renderComplete_ ? "  OK  " : "Cancel",
             props);
}

void RenderProgressModal::drawRenderProgress(GUIPoint &pos,
                                             GUITextProperties &props) {
  const char *spinnerchars = "|/-\\";
  char spinner = spinnerchars[spinner_++ % 4];

  char buffer[12];
  if (progressDisplayMode_ == ProgressDisplayMode::SongPercent) {
    uint8_t percent = calculateSongRenderPercent();
    sprintf(buffer, "%3d%% %c", percent, spinner);
  } else {
    // Calculate time in seconds from total samples
    uint8_t seconds = static_cast<uint8_t>(totalSamples_ / SAMPLE_RATE);
    uint8_t minutes = seconds / 60;
    seconds %= 60;
    sprintf(buffer, "%02d:%02d %c", minutes, seconds, spinner);
  }

  DrawString(pos._x, pos._y, buffer, props);
}

uint32_t RenderProgressModal::getDialogWidth() const { return dialogWidth_; }

int RenderProgressModal::getCurrentRenderedSongRow(bool *hasActive) const {
  if (hasActive != nullptr) {
    *hasActive = false;
  }
  if (viewData_ == nullptr) {
    return 0;
  }

  int currentRow = 0;
  bool foundActive = false;
  const int channelCount = viewData_->song_->GetChannelCount();
  for (int channel = 0; channel < channelCount; channel++) {
    if (viewData_->currentPlayPhrase_[channel] == EMPTY_SONG_VALUE) {
      continue;
    }
    const int row = viewData_->songPlayPos_[channel];
    if (!foundActive || row > currentRow) {
      currentRow = row;
      foundActive = true;
    }
  }

  if (hasActive != nullptr) {
    *hasActive = foundActive;
  }

  if (currentRow < 0) {
    currentRow = 0;
  } else if (currentRow >= SONG_ROW_COUNT) {
    currentRow = SONG_ROW_COUNT - 1;
  }
  return currentRow;
}

int RenderProgressModal::calculateSongRenderPercent() const {
  if (renderComplete_) {
    return 100;
  }
  if (!renderStarted_ || !selectionBoundsCaptured_) {
    return 0;
  }

  Player *player = Player::GetInstance();
  int currentRow =
      player != nullptr ? player->GetSelectionRenderSongRow() : startSongRow_;

  int totalRows = endSongRow_ - startSongRow_ + 1;
  if (totalRows <= 0) {
    totalRows = 1;
  }

  int renderedRows = currentRow - startSongRow_ + 1;
  if (renderedRows < 0) {
    renderedRows = 0;
  } else if (renderedRows > totalRows) {
    renderedRows = totalRows;
  }

  int percent = (renderedRows * 100) / totalRows;
  if (percent > 99) {
    percent = 99;
  }
  return percent;
}
