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

#include "MixerView.h"
#include "Application/Mixer/MixerService.h"
#include "Application/Utils/mathutils.h"
#include "Application/Views/SampleEditorView.h"
#include "UIController.h"
#include <Application/AppWindow.h>

namespace {

constexpr int MixerStripX = 5;
constexpr int MixerStripStride = 3;
// VU meters grow upwards from a baseline at (anchorY + height - 1). The strip
// labels sit one row below the baseline and the VOL field one row below that.
#if SHOW_FX_SCREEN
constexpr int MixerVuMeterHeight = 9;
constexpr int MixerStripLabelY = 12;
constexpr int MixerFieldBaseY = 13;
#else
// With the FX send rows gated off the meters are made tall enough to fill the
// space those rows used to occupy above the bottom nav map.
constexpr int MixerVuMeterHeight = 14;
constexpr int MixerStripLabelY = 17;
constexpr int MixerFieldBaseY = 18;
#endif
constexpr int MixerRowLabelX = 1;
constexpr int MixerMasterStripIndex = SONG_CHANNEL_COUNT_STEREO;
constexpr int MixerVolumeRow = 0;
constexpr int MixerFirstEffectRow = 1;
constexpr int MixerEffectRowEnd = MixerFirstEffectRow + OUTPUT_EFFECT_COUNT;

int GetMixerStripX(int strip) { return MixerStripX + strip * MixerStripStride; }

int32_t ScaleVuBarsForHeight(int32_t bars, int meterHeight) {
  const int32_t localMeterMax = meterHeight * 10 - 1;
  return std::min<int32_t>((bars * localMeterMax) / VU_METER_MAX,
                           localMeterMax);
}

constexpr int MixerMasterVuXOffset = MixerMasterStripIndex * MixerStripStride;

FourCC GetChannelVolumeFourCC(int channel) {
  static const FourCC ids[] = {
      FourCC::VarChannel1Volume, FourCC::VarChannel2Volume,
      FourCC::VarChannel3Volume, FourCC::VarChannel4Volume,
      FourCC::VarChannel5Volume, FourCC::VarChannel6Volume,
      FourCC::VarChannel7Volume, FourCC::VarChannel8Volume};
  return ids[channel];
}

#if SHOW_FX_SCREEN
FourCC GetChannelSendFourCC(int channel, int fx) {
  switch (fx) {
  case kReverbEffectIndex: {
    static const FourCC ids[] = {
        FourCC::VarChannel1FX1Send, FourCC::VarChannel2FX1Send,
        FourCC::VarChannel3FX1Send, FourCC::VarChannel4FX1Send,
        FourCC::VarChannel5FX1Send, FourCC::VarChannel6FX1Send,
        FourCC::VarChannel7FX1Send, FourCC::VarChannel8FX1Send};
    return ids[channel];
  }
  case kDelayEffectIndex: {
    static const FourCC ids[] = {
        FourCC::VarChannel1FX2Send, FourCC::VarChannel2FX2Send,
        FourCC::VarChannel3FX2Send, FourCC::VarChannel4FX2Send,
        FourCC::VarChannel5FX2Send, FourCC::VarChannel6FX2Send,
        FourCC::VarChannel7FX2Send, FourCC::VarChannel8FX2Send};
    return ids[channel];
  }
  case kReservedEffectIndex: {
    static const FourCC ids[] = {
        FourCC::VarChannel1FX3Send, FourCC::VarChannel2FX3Send,
        FourCC::VarChannel3FX3Send, FourCC::VarChannel4FX3Send,
        FourCC::VarChannel5FX3Send, FourCC::VarChannel6FX3Send,
        FourCC::VarChannel7FX3Send, FourCC::VarChannel8FX3Send};
    return ids[channel];
  }
  default:
    NAssert(false);
    return FourCC::VarChannel1FX1Send;
  }
}

FourCC GetFXReturnFourCC(int fx) {
  switch (fx) {
  case kReverbEffectIndex:
    return FourCC::VarFX1Volume;
  case kDelayEffectIndex:
    return FourCC::VarFX2Volume;
  case kReservedEffectIndex:
    return FourCC::VarFX3Volume;
  default:
    NAssert(false);
    return FourCC::VarFX1Volume;
  }
}

int GetEffectIndexForMixerRow(int row) {
  if (row >= MixerFirstEffectRow && row < MixerEffectRowEnd) {
    return row - MixerFirstEffectRow;
  }

  NAssert(false);
  return kReverbEffectIndex;
}
#endif

void CopyShortName(const char *source, char *destination) {
  NAssert(destination != nullptr);

  for (int i = 0; i < 3; ++i) {
    destination[i] = (source != nullptr && source[i] != '\0') ? source[i] : ' ';
  }
  destination[3] = '\0';
}

} // namespace

MixerView::MixerView(GUIWindow &w, ViewData *viewData)
    : FieldView(w, viewData), selectedStrip_(0), selectedRow_(0) {
  for (int strip = 0; strip < MixerStripCount; ++strip) {
    for (int row = 0; row < MixerRowCount; ++row) {
      fieldGrid_[strip][row] = nullptr;
    }
  }
  initMixerFields();
}

MixerView::~MixerView() {}

void MixerView::Reset() {
  ClearFocus();
  fieldList_.clear();
  channelVolumeFields_.clear();
  channelSendFields_.clear();
  fxReturnFields_.clear();
  masterVolumeField_.clear();

  for (int strip = 0; strip < MixerStripCount; ++strip) {
    for (int row = 0; row < MixerRowCount; ++row) {
      fieldGrid_[strip][row] = nullptr;
    }
  }

  selectedStrip_ = 0;
  selectedRow_ = 0;
  initMixerFields();
}

void MixerView::OnFocus() {
  setTitle(etl::string<23>("Mixer"));
  syncFocusToSelection();
}

void MixerView::SetFocus(UIField *field) {
  FieldView::SetFocus(field);

  if (!field) {
    return;
  }

  for (int strip = 0; strip < MixerStripCount; ++strip) {
    for (int row = 0; row < MixerRowCount; ++row) {
      if (fieldGrid_[strip][row] == field) {
        selectedStrip_ = strip;
        selectedRow_ = row;
        if (strip < SONG_CHANNEL_COUNT_STEREO) {
          viewData_->songX_ = strip;
        }
        return;
      }
    }
  }
}

UIField *MixerView::GetSelectedField() {
  UIField *field = fieldGrid_[selectedStrip_][selectedRow_];
  if (!field) {
    field = fieldGrid_[selectedStrip_][0];
  }
  if (!field && !fieldList_.empty()) {
    field = *fieldList_.begin();
  }
  return field;
}

void MixerView::syncFocusToSelection() {
  UIField *field = GetSelectedField();
  if (field) {
    SetFocus(field);
  }
}

void MixerView::updateCursor(int dx, int dy) {
  selectedStrip_ += dx;
  if (selectedStrip_ < 0) {
    selectedStrip_ = 0;
  }
  if (selectedStrip_ >= MixerStripCount) {
    selectedStrip_ = MixerStripCount - 1;
  }

  selectedRow_ += dy;
  if (selectedRow_ < 0) {
    selectedRow_ = 0;
  }
  if (selectedRow_ >= MixerRowCount) {
    selectedRow_ = MixerRowCount - 1;
  }

  syncFocusToSelection();
  isDirty_ = true;
}

void MixerView::switchSoloMode() {
  if (selectedStrip_ >= SONG_CHANNEL_COUNT_STEREO) {
    return;
  }

  UIController *controller = UIController::GetInstance();
  controller->SwitchSoloMode(selectedStrip_, selectedStrip_,
                             (viewMode_ == VM_NORMAL));
  viewMode_ = (viewMode_ != VM_SOLOON) ? VM_SOLOON : VM_NORMAL;
  isDirty_ = true;
};

void MixerView::unMuteAll() {
  UIController *controller = UIController::GetInstance();
  controller->UnMuteAll();
  isDirty_ = true;
};

void MixerView::toggleMute() {
  if (selectedStrip_ >= SONG_CHANNEL_COUNT_STEREO) {
    return;
  }

  UIController *controller = UIController::GetInstance();
  controller->ToggleMute(selectedStrip_, selectedStrip_);
  viewMode_ = (viewMode_ != VM_MUTEON) ? VM_MUTEON : VM_NORMAL;
  isDirty_ = true;
};

void MixerView::ProcessButtonMask(unsigned short mask, bool pressed) {
  if (!pressed) {
    if (viewMode_ == VM_MUTEON && (mask & EPBM_NAV)) {
      toggleMute();
    }
    if (viewMode_ == VM_SOLOON && (mask & EPBM_NAV)) {
      switchSoloMode();
    }
    SetDirty(true);
    return;
  }

  if (mask == EPBM_PLAY) {
    togglePlay();
    return;
  }

  if ((mask & EPBM_NAV) && (mask & EPBM_UP)) {
    ViewType vt = VT_SONG;
    ViewEvent ve(VET_SWITCH_VIEW, &vt);
    SetChanged();
    NotifyObservers(&ve);
    return;
  }

  if ((mask & EPBM_NAV) && (mask & EPBM_DOWN)) {
#if SHOW_FX_SCREEN
    ViewType vt = VT_MASTER_EFFECTS;
    ViewEvent ve(VET_SWITCH_VIEW, &vt);
    SetChanged();
    NotifyObservers(&ve);
#endif
    return;
  }

  if ((mask & EPBM_NAV) && (mask & EPBM_ALT)) {
    unMuteAll();
    return;
  }

  if ((mask & EPBM_EDIT) && (mask & EPBM_NAV)) {
    toggleMute();
    return;
  }

  if ((mask & EPBM_ENTER) && (mask & EPBM_NAV)) {
    switchSoloMode();
    return;
  }

  if ((mask & EPBM_EDIT) && (mask & EPBM_PLAY)) {
    if (!Player::GetInstance()->IsRunning()) {
      switchToRecordView();
    }
    return;
  }

  if (mask & (EPBM_ENTER | EPBM_EDIT)) {
    FieldView::ProcessButtonMask(mask, pressed);
    SetDirty(true);
    return;
  }

  viewMode_ = VM_NORMAL;
  SetDirty(true);
  processNormalButtonMask(mask);
}

void MixerView::processNormalButtonMask(unsigned int mask) {
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

void MixerView::processSelectionButtonMask(unsigned int mask) { (void)mask; }

void MixerView::initMixerFields() {
  Project *project = viewData_ ? viewData_->project_ : nullptr;
  if (!project) {
    return;
  }

  channelVolumeFields_.clear();
  channelSendFields_.clear();
  fxReturnFields_.clear();
  masterVolumeField_.clear();

  for (int channel = 0; channel < SONG_CHANNEL_COUNT_STEREO; ++channel) {
    GUIPoint volumePos(GetMixerStripX(channel), MixerFieldBaseY);
    Variable *volume = project->FindVariable(GetChannelVolumeFourCC(channel));
    if (volume) {
      channelVolumeFields_.emplace_back(volumePos, *volume, "%2.2d", 0, 99, 1,
                                        5);
      UIField *field = &(*channelVolumeFields_.rbegin());
      fieldGrid_[channel][0] = field;
      fieldList_.insert(fieldList_.end(), field);
    }

#if SHOW_FX_SCREEN
    // the columns for the fx to master send levels
    for (int row = MixerFirstEffectRow; row < MixerEffectRowEnd; ++row) {
      int effectIndex = GetEffectIndexForMixerRow(row);
      GUIPoint sendPos(GetMixerStripX(channel), MixerFieldBaseY + row);
      Variable *send =
          project->FindVariable(GetChannelSendFourCC(channel, effectIndex));
      if (send) {
        channelSendFields_.emplace_back(sendPos, *send, "%2.2d", 0, 99, 1, 5);
        UIField *field = &(*channelSendFields_.rbegin());
        fieldGrid_[channel][row] = field;
        fieldList_.insert(fieldList_.end(), field);
      }
    }
#endif
  }

#if SHOW_FX_SCREEN
  // the column for the fx to master return level
  for (int row = MixerFirstEffectRow; row < MixerEffectRowEnd; ++row) {
    int effectIndex = GetEffectIndexForMixerRow(row);
    GUIPoint returnPos(GetMixerStripX(MixerMasterStripIndex),
                       MixerFieldBaseY + row);
    Variable *returnVolume =
        project->FindVariable(GetFXReturnFourCC(effectIndex));
    if (returnVolume) {
      fxReturnFields_.emplace_back(returnPos, *returnVolume, "%2.2d", 0, 99, 1,
                                   5);
      UIField *field = &(*fxReturnFields_.rbegin());
      fieldGrid_[MixerMasterStripIndex][row] = field;
      fieldList_.insert(fieldList_.end(), field);
    }
  }
#endif

  GUIPoint masterPos(GetMixerStripX(MixerMasterStripIndex),
                     MixerFieldBaseY + MixerVolumeRow);
  Variable *masterVolume = project->FindVariable(FourCC::VarMasterVolume);
  if (masterVolume) {
    masterVolumeField_.emplace_back(masterPos, *masterVolume, "%2.2d", 0, 100,
                                    1, 5);
    UIField *field = &(*masterVolumeField_.begin());
    fieldGrid_[MixerMasterStripIndex][MixerVolumeRow] = field;
    fieldList_.insert(fieldList_.end(), field);
  }

  syncFocusToSelection();
}

void MixerView::drawMixerLabels(Player *player, GUITextProperties props) {
  for (int row = 0; row < MixerRowCount; ++row) {
    char rowLabel[4];
    if (row == MixerVolumeRow) {
      CopyShortName("VOL", rowLabel);
#if SHOW_FX_SCREEN
    } else {
      OutputEffectProcessor *effect =
          MixerService::GetInstance()->GetOutputEffect(
              GetEffectIndexForMixerRow(row));
      CopyShortName(effect ? effect->GetShortName() : "---", rowLabel);
#endif
    }

    if (row == selectedRow_) {
      SetColor(CD_HILITE2);
      props.invert_ = true;
    } else {
      SetColor(CD_NORMAL);
      props.invert_ = false;
    }
    DrawString(MixerRowLabelX, MixerFieldBaseY + row, rowLabel, props);
  }

  for (int strip = 0; strip < MixerStripCount; ++strip) {
    char label[3] = {' ', ' ', '\0'};
    if (strip < SONG_CHANNEL_COUNT_STEREO) {
      if (player->IsChannelMuted(strip)) {
        label[0] = 'M';
        label[1] = static_cast<char>('1' + strip);
      } else {
        label[0] = '0';
        label[1] = static_cast<char>('1' + strip);
      }
    } else {
      label[0] = 'M';
      label[1] = 'B';
    }

    if (strip == selectedStrip_) {
      SetColor(CD_HILITE2);
      props.invert_ = true;
    } else {
      SetColor(CD_NORMAL);
      props.invert_ = false;
    }

    DrawString(GetMixerStripX(strip), MixerStripLabelY, label, props);
  }

  SetColor(CD_NORMAL);
  props.invert_ = false;
}

void MixerView::DrawView() {
  Clear();

  GUITextProperties props;

  Player *player = Player::GetInstance();

  props.invert_ = false;
  drawMixerLabels(player, props);
  FieldView::Redraw();
  drawMasterVuMeter(player, props, false, MixerMasterVuXOffset,
                    MixerVuMeterHeight);
  drawMap();
  drawNotes();

  if (player->IsRunning()) {
    OnPlayerUpdate(PET_UPDATE);
  }
}

void MixerView::OnPlayerUpdate(PlayerEventType eventType, unsigned int tick) {
  ScreenView::OnPlayerUpdate(eventType, tick);
}

void MixerView::AnimationUpdate() {
  ScreenView::AnimationUpdate();
  GUITextProperties props;

  Player *player = Player::GetInstance();
  if (!viewData_ || !player) {
    w_.Flush();
    return;
  }

  etl::array<stereosample, SONG_CHANNEL_COUNT> *levels =
      player->GetMixerLevels();
  if (levels) {
    drawChannelVUMeters(levels, player, props);
    drawMasterVuMeter(player, props, false, MixerMasterVuXOffset,
                      MixerVuMeterHeight);
  }

  drawNotes();

  w_.Flush();
}

void MixerView::drawChannelVUMeters(
    etl::array<stereosample, SONG_CHANNEL_COUNT> *levels, Player *player,
    GUITextProperties props, bool forceRedraw) {
  const int displayChannelCount =
      std::min(viewData_->song_->GetChannelCount(), SONG_CHANNEL_COUNT_STEREO);
  if (!forceRedraw) {
    bool anyChanges = false;
    for (int i = 0; i < displayChannelCount; i++) {
      int32_t leftBars = 0;
      int32_t rightBars = 0;
      if (!player->IsChannelMuted(i)) {
        amplitudeToBars(levels->at(i), &leftBars, &rightBars);
        leftBars = ScaleVuBarsForHeight(leftBars, MixerVuMeterHeight);
        rightBars = ScaleVuBarsForHeight(rightBars, MixerVuMeterHeight);
      }
      if (leftBars != prevLeftVU_[i + 1] || rightBars != prevRightVU_[i + 1]) {
        anyChanges = true;
        break;
      }
    }
    if (!anyChanges) {
      return;
    }
  }

  GUIPoint pos(MixerStripX, GetAnchor()._y + MixerVuMeterHeight - 1);
  for (int i = 0; i < displayChannelCount; i++) {
    int32_t leftBars = 0;
    int32_t rightBars = 0;
    if (!player->IsChannelMuted(i)) {
      amplitudeToBars(levels->at(i), &leftBars, &rightBars);
    }

    drawVUMeter(leftBars, rightBars, pos, props, i + 1, forceRedraw,
                MixerVuMeterHeight);
    pos._x += MixerStripStride;
  }
}

void MixerView::togglePlay() {
  if (!viewData_) {
    return;
  }

  Player *player = Player::GetInstance();
  int startChannel = selectedStrip_;
  if (startChannel >= SONG_CHANNEL_COUNT_STEREO) {
    startChannel = viewData_->songX_;
  }
  if (startChannel < 0 || startChannel >= SONG_CHANNEL_COUNT_STEREO) {
    startChannel = 0;
  }

  player->OnStartButton(PM_CHAIN, static_cast<unsigned int>(startChannel), true,
                        viewData_->chainRow_);
}
