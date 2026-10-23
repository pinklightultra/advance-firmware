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

#ifndef _SAMPLE_SLICES_VIEW_H_
#define _SAMPLE_SLICES_VIEW_H_

#include "Application/Instruments/I_Instrument.h"
#include "FieldView.h"
#include "Foundation/Observable.h"
#include "SampleSlicer.h"
#include "ViewData.h"
#include <cstdint>

class SampleSlicesView : public FieldView, public I_Observer {
public:
  SampleSlicesView(GUIWindow &w, ViewData *data);
  ~SampleSlicesView() override;
  void Reset();

  void ProcessButtonMask(unsigned short mask, bool pressed) override;
  void DrawView() override;
  void OnPlayerUpdate(PlayerEventType, unsigned int) override{};
  void OnFocus() override;
  void AnimationUpdate() override;
  void Update(Observable &o, I_ObservableData *d) override;

private:
  static void AutoSliceConfirmCallback(View &v, ModalView &dialog);
  void buildFieldLayout();
  I_Instrument *currentInstrument();

  bool needsFullRedraw_;
  bool modalWasOpen_;
  SampleSlicer slicer_;
};

#endif
