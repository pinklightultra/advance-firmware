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

#include "ScreenView.h"
#include <Application/AppWindow.h>
#include <nanoprintf.h>

ScreenView::ScreenView(GUIWindow &w, ViewData *viewData) : View(w, viewData) {}

ScreenView::~ScreenView() {}

void ScreenView::OnPlayerUpdate(PlayerEventType, unsigned int tick) {
  (void)tick;
  if (shouldDrawPlayTime()) {
    needsPlayTimeUpdate_ = true;
  }
}

/// Updates the animation by redrawing the battery gauge and power button UI on
/// every clock tick
void ScreenView::AnimationUpdate() {
  GUITextProperties props;
  updateDeferredPlayButtonHold();
  drawTitlebar(props);
  if (shouldDrawPlayTime() && needsPlayTimeUpdate_) {
    Player *player = Player::GetInstance();
    if (player != nullptr) {
      char strbuffer[10];
      SetColor(CD_NORMAL);
      props.invert_ = false;
      if (!player->IsRunning()) {
        DrawString(27, 1, "     ", props);
      } else {
        int time = int(player->GetPlayTime());
        int mi = time / 60;
        int se = time - mi * 60;
        npf_snprintf(strbuffer, sizeof(strbuffer), "%2.2d:%2.2d", mi, se);
        DrawString(27, 1, strbuffer, props);
      }
    }
    needsPlayTimeUpdate_ = false;
  }
  drawPowerButtonUI(props);
};
