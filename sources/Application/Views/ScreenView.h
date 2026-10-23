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

#ifndef _SCREEN_VIEW_H_
#define _SCREEN_VIEW_H_

#include "BaseClasses/View.h"
#include "ViewData.h"

class ScreenView : public View {
public:
  ScreenView(GUIWindow &w, ViewData *viewData);
  ~ScreenView();
  virtual void ProcessButtonMask(unsigned short mask, bool pressed){};
  virtual void DrawView(){};
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick = 0) override;
  virtual void OnFocus(){};
  virtual void AnimationUpdate() override;

protected:
  virtual bool shouldDrawPlayTime() const { return false; }

private:
  bool needsPlayTimeUpdate_ = false;
};
#endif
