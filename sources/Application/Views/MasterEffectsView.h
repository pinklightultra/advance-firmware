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

#ifndef _MASTER_EFFECTS_VIEW_H_
#define _MASTER_EFFECTS_VIEW_H_

#include "Application/Mixer/OutputEffectConstants.h"
#include "BaseClasses/UIIntVarField.h"
#include "FieldView.h"
#include "ViewData.h"

class MasterEffectsView : public FieldView {
public:
  MasterEffectsView(GUIWindow &w, ViewData *viewData);
  ~MasterEffectsView();

  void Reset();
  virtual void ProcessButtonMask(unsigned short mask, bool pressed);
  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int tick = 0);
  virtual void OnFocus();
  virtual void AnimationUpdate();

private:
  static constexpr int SlotCount = OUTPUT_EFFECT_COUNT;
  static constexpr int EffectParameterRowCount = 4;
  static constexpr int GridRowCount = 1 + EffectParameterRowCount;

  void initFields();
  void updateCursor(int dx, int dy);
  void syncFocusToSelection();
  UIField *GetSelectedField();
  bool SlotUsesReverb(int slot) const;
  void drawParameterColumn(int slot, GUITextProperties props);
  void togglePlay();
  bool shouldDrawPlayTime() const override { return true; }

  etl::vector<UIIntVarField, SlotCount> slotTypeFields_;
  etl::vector<UIIntVarField, SlotCount * EffectParameterRowCount>
      effectParameterFields_;
  UIField *slotFields_[SlotCount];
  UIField *effectParameterGrid_[SlotCount][EffectParameterRowCount];
  int selectedColumn_;
  int selectedRow_;
};

#endif
