/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#include "SampleSlicesView.h"

#include "Application/AppWindow.h"
#include "Application/Instruments/InstrumentBank.h"
#include "Application/Instruments/SampleInstrument.h"
#include "Application/Instruments/SampleSourceInstrument.h"
#include "Application/Model/Project.h"
#include "Application/Utils/char.h"
#include "Application/Views/ModalDialogs/MessageBox.h"

namespace {
constexpr int32_t SliceXOffset = SampleSlicer::GraphXOffset;
constexpr int32_t SliceYOffset = 2 * CHAR_HEIGHT;
} // namespace

SampleSlicesView::SampleSlicesView(GUIWindow &w, ViewData *data)
    : FieldView(w, data), needsFullRedraw_(true), modalWasOpen_(false),
      slicer_({GUIPoint(SliceXOffset, SliceYOffset), GUIPoint(1, 8), true, true,
               true, true}) {
  slicer_.AddObserver(*this);
}

SampleSlicesView::~SampleSlicesView() { slicer_.Reset(); }

void SampleSlicesView::Reset() {
  slicer_.Reset();
  modalWasOpen_ = false;
  needsFullRedraw_ = true;
}

void SampleSlicesView::OnFocus() {
  slicer_.Attach(currentInstrument(),
                 static_cast<int32_t>(viewData_->currentInstrumentID_));
  modalWasOpen_ = false;
  needsFullRedraw_ = true;
  buildFieldLayout();
  isDirty_ = true;
}

void SampleSlicesView::ProcessButtonMask(unsigned short mask, bool pressed) {
  if (!pressed) {
    if (slicer_.ProcessButtonMask(*this, *viewData_, mask, pressed)) {
      return;
    }
    FieldView::ProcessButtonMask(mask, pressed);
    return;
  }

  if (mask & EPBM_NAV) {
    if (mask & EPBM_LEFT) {
      slicer_.Reset();
      ViewType vt = VT_INSTRUMENT;
      ViewEvent ve(VET_SWITCH_VIEW, &vt);
      SetChanged();
      NotifyObservers(&ve);
      return;
    }
    FieldView::ProcessButtonMask(mask, pressed);
    return;
  }

  if (slicer_.ProcessButtonMask(*this, *viewData_, mask, pressed)) {
    return;
  }

  FieldView::ProcessButtonMask(mask, pressed);
}

void SampleSlicesView::DrawView() {
  if (needsFullRedraw_) {
    Clear();
  }

  GUITextProperties props;

  // Draw title
  setTitle(etl::string<23>("Sample Slices"));

  bool hasModal = HasModalView();
  if (!hasModal) {
    slicer_.Draw(*this);
    ClearTextRect(0, 9, SCREEN_WIDTH, 3);
  }

  if (hasModal) {
    if (GetFocus() == 0 && !fieldList_.empty()) {
      SetFocus(*fieldList_.begin());
    }
    for (auto it = fieldList_.begin(); it != fieldList_.end(); ++it) {
      if (*it == slicer_.GraphFieldControl()) {
        continue;
      }
      (*it)->Draw(w_);
    }
  } else {
    FieldView::Redraw();
  }
  needsFullRedraw_ = false;
}

void SampleSlicesView::AnimationUpdate() {
  GUITextProperties props;
  drawTitlebar(props);
  drawPowerButtonUI(props);

  bool hasModal = HasModalView();
  if (modalWasOpen_ && !hasModal) {
    slicer_.Graph().RequestFullRedraw();
    isDirty_ = true;
    ((AppWindow &)w_).SetDirty();
  }
  modalWasOpen_ = hasModal;
  slicer_.AnimationUpdate(*this);
  if (isDirty_) {
    ((AppWindow &)w_).SetDirty();
  }
}

void SampleSlicesView::Update(Observable &, I_ObservableData *data) {
  if (!hasFocus_) {
    return;
  }

  uintptr_t fourcc = reinterpret_cast<uintptr_t>(data);
  if (fourcc == FourCC::ActionAutoSlice) {
    if (slicer_.HasSlicesForPlayback()) {
      MessageBox *mb = MessageBox::Create(*this, "Replace current slices?",
                                          MBBF_YES | MBBF_NO);
      slicer_.MarkModalRecovery(0);
      slicer_.ClearWaveformRegion(*this);
      ((AppWindow &)w_).InvalidateTextCache();
      DoModal(mb, ModalViewCallback::create<
                      &SampleSlicesView::AutoSliceConfirmCallback>());
    } else {
      slicer_.AutoSliceEvenly(*this);
    }
    ((AppWindow &)w_).SetDirty();
    return;
  }

  if (slicer_.HandleVariableUpdate(*this, fourcc)) {
    ((AppWindow &)w_).SetDirty();
  }
}

void SampleSlicesView::buildFieldLayout() {
  fieldList_.clear();
  slicer_.AddFields(fieldList_, *this);
  SetFocus(slicer_.GraphFieldControl());
}

void SampleSlicesView::AutoSliceConfirmCallback(View &view, ModalView &dialog) {
  auto &self = static_cast<SampleSlicesView &>(view);
  self.slicer_.MarkModalRecovery(2);
  self.isDirty_ = true;
  ((AppWindow &)self.w_).SetDirty();
  if (dialog.GetReturnCode() == MBL_YES) {
    self.slicer_.AutoSliceEvenly(self);
  }
}

I_Instrument *SampleSlicesView::currentInstrument() {
  if (!viewData_ || !viewData_->project_) {
    return nullptr;
  }

  InstrumentBank *bank = viewData_->project_->GetInstrumentBank();
  if (!bank) {
    return nullptr;
  }

  I_Instrument *instr = bank->GetInstrument(viewData_->currentInstrumentID_);
  if (!instr ||
      (instr->GetType() != IT_SAMPLE && instr->GetType() != IT_SAMPLE_SOURCE)) {
    return nullptr;
  }
  return instr;
}
