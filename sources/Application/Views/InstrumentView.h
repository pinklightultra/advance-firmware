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

#ifndef _INSTRUMENT_VIEW_H_
#define _INSTRUMENT_VIEW_H_

#include "Application/Instruments/I_Instrument.h"
#include "Application/Instruments/InstrumentNameVariable.h"
#include "BaseClasses/UIActionField.h"
#include "BaseClasses/UIBigHexVarField.h"
#include "BaseClasses/UIBitmaskVarField.h"
#include "BaseClasses/UIIntVarField.h"
#include "BaseClasses/UIIntVarOffField.h"
#include "BaseClasses/UINoteVarField.h"
#include "BaseClasses/UIStaticField.h"
#include "BaseClasses/UITextField.h"
#include "Externals/etl/include/etl/string.h"
#include "Externals/etl/include/etl/vector.h"
#include "FieldView.h"
#include "Foundation/Observable.h"
#include "Foundation/Variables/Variable.h"
#include "Foundation/Variables/WatchedVariable.h"
#include "SampleSlicer.h"
#include "ViewData.h"
#include <cstddef>
#include <cstdint>

class InstrumentView : public FieldView, public I_Observer {
public:
  InstrumentView(GUIWindow &w, ViewData *data);
  virtual ~InstrumentView();
  void Reset();

  virtual void ProcessButtonMask(unsigned short mask, bool pressed);
  virtual void DrawView();
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int){};
  virtual void OnFocus();
  virtual void AnimationUpdate() override;
  void onInstrumentTypeChange(bool updateUI = false);
  bool checkInstrumentModified();
  void resetInstrumentToDefaults();
  void applyProposedTypeChangeUI();

protected:
  void warpToNext(int offset);
  void onInstrumentChange();
  void fillSampleParameters();
  void fillSampleSourceParameters();
  void fillMidiParameters();
  void fillNoneParameters();
  I_Instrument *getInstrument();
  void Update(Observable &o, I_ObservableData *d);
  void refreshInstrumentFields();
  void addNameTextField(I_Instrument *instr, GUIPoint &position);
  void handleInstrumentExport();
  void clearEmbeddedSlicerWaveform();
  void setupEmbeddedSlicer();
  void addEmbeddedSlicerFields(GUIPoint &position);
  void drawEmbeddedSlicerWaveform();
  bool focusedEmbeddedSlicerPointSample(uint32_t &sample);
  bool focusedEmbeddedSlicerPreviewField();
  void updateEmbeddedSlicerPreviewRange();
  bool isEmbeddedSlicerActive() const;
  bool startInstrumentPreview(unsigned short mask);
  void stopInstrumentPreview();

private:
  void onConfirmInstrumentTypeChange(View &view, ModalView &dialog);
  void onConfirmResetInstrument(View &view, ModalView &dialog);
  void onConfirmSampleChange(View &view, ModalView &dialog);
  void onConfirmExportOverwrite(View &view, ModalView &dialog);
  static void AutoSliceConfirmCallback(View &view, ModalView &dialog);

  static constexpr size_t SliceCountLabelSize = 20;
  Project *project_;
  FourCC lastFocusID_;
  bool lastFocusWasSliceGraph_;
  WatchedVariable instrumentType_;
  int lastSampleIndex_;
  int8_t pendingSampleSourceLoopModeDirection_;
  bool previewKeyHeld_;
  unsigned short previewChannel_;
  unsigned short previewInstrument_;
  bool suppressSampleChangeWarning_;
  etl::string<SliceCountLabelSize> sliceCountLabel_;

  // Variables for export confirmation dialog
  I_Instrument *pendingPurgeInstrument_ = nullptr;
  I_Instrument *pendingSampleChangeInstrument_ = nullptr;
  int pendingSampleChangeNewIndex_ = -1;
  I_Instrument *exportInstrument_ = nullptr;
  etl::string<MAX_INSTRUMENT_NAME_LENGTH> exportName_;
  InstrumentType pendingInstrumentType_ = IT_NONE;

  SampleSlicer embeddedSlicer_;

  etl::vector<UIIntVarField, 1> typeIntVarField_;
  etl::vector<UIActionField, 2> persistentActionField_;
  etl::vector<UIIntVarField, 40> intVarField_;
  etl::vector<UINoteVarField, 1> noteVarField_;
  etl::vector<UIStaticField, 10> staticField_;
  etl::vector<UIBigHexVarField, 4> bigHexVarField_;
  etl::vector<UIIntVarOffField, 2> intVarOffField_;
  etl::vector<UIActionField, 1> sampleActionField_;
  etl::vector<UIBitmaskVarField, 3> bitmaskVarField_;
  etl::vector<UITextField<MAX_INSTRUMENT_NAME_LENGTH>, 1> nameTextField_;
  etl::vector<InstrumentNameVariable, 1> nameVariables_;
};
#endif
