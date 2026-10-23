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

#include "UIController.h"

#include "Application/Player/Player.h"

UIController::UIController(){};

void UIController::Init(Project *project, ViewData *viewData) {
  viewData_ = viewData;
  project_ = project;
}

void UIController::Reset() {
  viewData_ = 0;
  project_ = 0;
};

void UIController::UnMuteAll() {

  Player *player = Player::GetInstance();
  const int channelCount = viewData_->song_->GetChannelCount();
  for (int i = 0; i < channelCount; i++) {
    player->SetChannelMute(i, false);
  };
};

void UIController::ToggleMute(int from, int to) {

  Player *player = Player::GetInstance();
  for (int i = from; i < to + 1; i++) {
    bool muted = player->IsChannelMuted(i);
    player->SetChannelMute(i, !muted);
  };
};

void UIController::SwitchSoloMode(int from, int to, bool soloing) {

  Player *player = Player::GetInstance();

  // If not in solo mode, we solo current channel or selection

  if (soloing) {

    const int channelCount = viewData_->song_->GetChannelCount();
    for (int i = 0; i < channelCount; i++) {
      soloMask_[i] = player->IsChannelMuted(i);
      player->SetChannelMute(i, (i < from) || (i > to));
    };
  } else {
    const int channelCount = viewData_->song_->GetChannelCount();
    for (int i = 0; i < channelCount; i++) {
      player->SetChannelMute(i, soloMask_[i]);
    };
  }
};
