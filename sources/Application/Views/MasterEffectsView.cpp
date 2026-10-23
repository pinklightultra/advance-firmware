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

#include "MasterEffectsView.h"
#include "Application/Mixer/OutputEffectDefinitions.h"
#include "Application/Player/Player.h"
#include <cstdint>

namespace {

constexpr int kSlotColumnX[OUTPUT_EFFECT_COUNT] = {0, 10, 20};
constexpr int kSlotRowY = 2;
constexpr int kParameterBaseY = 4;
constexpr int kParameterRowCount = 4;

struct ReverbParameterLayout {
  FourCC id;
  const char *format;
  const char *placeholder;
};

constexpr ReverbParameterLayout kReverbParameters[kParameterRowCount] = {
    {FourCC::VarReverbInputGain, "IN:[%2.2d]", "---:[--]"},
    {FourCC::VarReverbTime, "TIM:[%2.2d]", "---:[--]"},
    {FourCC::VarReverbDiffusion, "DIF:[%2.2d]", "---:[--]"},
    {FourCC::VarReverbLowPass, "LPF:[%2.2d]", "---:[--]"},
};

FourCC GetFXTypeFourCC(int slot) {
  switch (slot) {
  case 0:
    return FourCC::VarFX1Type;
  case 1:
    return FourCC::VarFX2Type;
  case 2:
    return FourCC::VarFX3Type;
  default:
    NAssert(false);
    return FourCC::VarFX1Type;
  }
}

const char *GetSlotFormat(int slot) {
  switch (slot) {
  case 0:
    return "S1:[%s]";
  case 1:
    return "S2:[%s]";
  case 2:
    return "S3:[%s]";
  default:
    NAssert(false);
    return "S1:[%s]";
  }
}

} // namespace

MasterEffectsView::MasterEffectsView(GUIWindow &w, ViewData *viewData)
    : FieldView(w, viewData), selectedColumn_(0), selectedRow_(0) {
  for (int slot = 0; slot < SlotCount; ++slot) {
    slotFields_[slot] = nullptr;
    for (int parameter = 0; parameter < EffectParameterRowCount; ++parameter) {
      effectParameterGrid_[slot][parameter] = nullptr;
    }
  }

  initFields();
}

MasterEffectsView::~MasterEffectsView() {}

void MasterEffectsView::Reset() {
  ClearFocus();
  fieldList_.clear();
  slotTypeFields_.clear();
  effectParameterFields_.clear();

  for (int slot = 0; slot < SlotCount; ++slot) {
    slotFields_[slot] = nullptr;
    for (int parameter = 0; parameter < EffectParameterRowCount; ++parameter) {
      effectParameterGrid_[slot][parameter] = nullptr;
    }
  }

  selectedColumn_ = 0;
  selectedRow_ = 0;
  initFields();
}

void MasterEffectsView::ProcessButtonMask(unsigned short mask, bool pressed) {
  if (!pressed) {
    SetDirty(true);
    return;
  }

  if ((mask & EPBM_EDIT) && (mask & EPBM_PLAY)) {
    if (!Player::GetInstance()->IsRunning()) {
      switchToRecordView();
    }
    return;
  }

  if (mask & EPBM_PLAY) {
    togglePlay();
    return;
  }

  if ((mask & EPBM_NAV) && (mask & EPBM_UP)) {
    ViewType vt = VT_MIXER;
    ViewEvent ve(VET_SWITCH_VIEW, &vt);
    SetChanged();
    NotifyObservers(&ve);
    return;
  }

  if (mask & (EPBM_ENTER | EPBM_EDIT)) {
    if (GetSelectedField()) {
      FieldView::ProcessButtonMask(mask, pressed);
      syncFocusToSelection();
    }
    SetDirty(true);
    return;
  }

  if (mask & EPBM_NAV) {
    return;
  }

  if (mask & EPBM_LEFT) {
    updateCursor(-1, 0);
  }
  if (mask & EPBM_RIGHT) {
    updateCursor(1, 0);
  }
  if (mask & EPBM_UP) {
    updateCursor(0, -1);
  }
  if (mask & EPBM_DOWN) {
    updateCursor(0, 1);
  }
}

void MasterEffectsView::DrawView() {
  Clear();

  GUITextProperties props;

  // Draw title
  setTitle(etl::string<23>("Master FX"));

  props.invert_ = false;

  for (int slot = 0; slot < SlotCount; ++slot) {
    if (slotFields_[slot]) {
      slotFields_[slot]->Draw(w_);
    }
    drawParameterColumn(slot, props);
  }

  drawMap();
}

void MasterEffectsView::OnPlayerUpdate(PlayerEventType eventType,
                                       unsigned int tick) {
  ScreenView::OnPlayerUpdate(eventType, tick);
}

void MasterEffectsView::OnFocus() { syncFocusToSelection(); }

void MasterEffectsView::AnimationUpdate() {
  ScreenView::AnimationUpdate();
  w_.Flush();
}

void MasterEffectsView::initFields() {
  Project *project = viewData_ ? viewData_->project_ : nullptr;
  if (!project) {
    return;
  }

  slotTypeFields_.clear();
  effectParameterFields_.clear();

  for (int slot = 0; slot < SlotCount; ++slot) {
    Variable *slotType = project->FindVariable(GetFXTypeFourCC(slot));
    if (slotType) {
      slotTypeFields_.emplace_back(
          GUIPoint(kSlotColumnX[slot], kSlotRowY), *slotType,
          GetSlotFormat(slot), kOutputEffectNone, kOutputEffectReverb, 1, 1);
      slotFields_[slot] = &(*slotTypeFields_.rbegin());
    }

    for (int parameter = 0; parameter < EffectParameterRowCount; ++parameter) {
      Variable *value = project->FindVariable(kReverbParameters[parameter].id);
      if (!value) {
        continue;
      }

      effectParameterFields_.emplace_back(
          GUIPoint(kSlotColumnX[slot], kParameterBaseY + parameter), *value,
          kReverbParameters[parameter].format, 0, 99, 1, 5);
      effectParameterGrid_[slot][parameter] =
          &(*effectParameterFields_.rbegin());
    }
  }

  syncFocusToSelection();
}

void MasterEffectsView::updateCursor(int dx, int dy) {
  selectedColumn_ += dx;
  if (selectedColumn_ < 0) {
    selectedColumn_ = 0;
  }
  if (selectedColumn_ >= SlotCount) {
    selectedColumn_ = SlotCount - 1;
  }

  selectedRow_ += dy;
  if (selectedRow_ < 0) {
    selectedRow_ = 0;
  }
  if (selectedRow_ >= GridRowCount) {
    selectedRow_ = GridRowCount - 1;
  }

  syncFocusToSelection();
  isDirty_ = true;
}

void MasterEffectsView::syncFocusToSelection() { SetFocus(GetSelectedField()); }

UIField *MasterEffectsView::GetSelectedField() {
  if (selectedRow_ == 0) {
    return slotFields_[selectedColumn_];
  }

  if (!SlotUsesReverb(selectedColumn_)) {
    return nullptr;
  }

  return effectParameterGrid_[selectedColumn_][selectedRow_ - 1];
}

bool MasterEffectsView::SlotUsesReverb(int slot) const {
  if (slot < 0 || slot >= SlotCount || viewData_ == nullptr ||
      viewData_->project_ == nullptr) {
    return false;
  }

  return ClampOutputEffectTypeIndex(viewData_->project_->GetFXSlotType(slot)) ==
         kOutputEffectReverb;
}

void MasterEffectsView::drawParameterColumn(int slot, GUITextProperties props) {
  const bool slotUsesReverb = SlotUsesReverb(slot);

  for (int parameter = 0; parameter < EffectParameterRowCount; ++parameter) {
    if (slotUsesReverb && effectParameterGrid_[slot][parameter]) {
      effectParameterGrid_[slot][parameter]->Draw(w_);
      continue;
    }

    const bool selected =
        (selectedColumn_ == slot) && (selectedRow_ == parameter + 1);
    if (selected) {
      SetColor(CD_HILITE2);
      props.invert_ = true;
    } else {
      SetColor(CD_NORMAL);
      props.invert_ = false;
    }

    DrawString(kSlotColumnX[slot], kParameterBaseY + parameter,
               kReverbParameters[parameter].placeholder, props);
  }
}

void MasterEffectsView::togglePlay() {
  if (!viewData_ || !viewData_->song_) {
    return;
  }

  const int32_t songChannelCount = viewData_->song_->GetChannelCount();
  if (songChannelCount <= 0) {
    return;
  }

  Player *player = Player::GetInstance();
  player->OnSongStartButton(0, static_cast<uint32_t>(songChannelCount - 1),
                            false, false);
}
