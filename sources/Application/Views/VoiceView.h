/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _VOICE_VIEW_H_
#define _VOICE_VIEW_H_

#include "Application/Player/InsertEffectDefinitions.h"
#include "BaseClasses/UIIntVarField.h"
#include "BaseClasses/UILFOField.h"
#include "BaseClasses/UIStaticField.h"
#include "FieldView.h"
#include "Foundation/Types/Types.h"
#include "ViewData.h"
#include <cstddef>

class I_Instrument;
class InstrumentBank;

class VoiceView : public FieldView {
public:
  VoiceView(GUIWindow &w, ViewData *data);
  virtual ~VoiceView();

  void Reset();

  virtual void ProcessButtonMask(unsigned short mask, bool pressed) override;
  virtual void DrawView() override;
  virtual void OnPlayerUpdate(PlayerEventType, unsigned int){};
  virtual void OnFocus() override;
  enum class GraphKind : uint8_t { Voice = 0, Filter, Amp, LFO1, LFO2, Fx };
  enum class GraphSection : uint8_t {
    None = 0,
    All,
    FilterCutoff,
    FilterType,
    FilterResonance,
    Attack,
    Decay,
    Sustain,
    Release,
    VoiceBase,
    VoiceWidth,
    TriggerLength,
    Pan,
    FxType1,
    FxAmount1,
    FxPosition1,
    FxType2,
    FxAmount2,
    FxPosition2,
    LFOShape,
    LFORate,
    LFODepth,
  };

private:
  void refreshVoiceFields();
  void drawEnvelopeGraphs();
  void drawSectionHeaders();
  void drawLFOTargetMarkers();
  void updateGraphSelectionFromFocus();
  bool moveFocus(int32_t xDirection, int32_t yDirection);
  bool processNavigationButtonMask(unsigned short mask);
  GraphKind graphKindForField(UIField *field);
  GraphSection graphSectionForField(UIField *field);
  I_Instrument *getInstrument();
  bool startVoicePreview(unsigned short mask);
  void stopVoicePreview();
  void warpToNext(int offset);

  static constexpr size_t VoiceFieldCount = 23;

  int lastFocusIndex_;
  etl::vector<UIIntVarField, VoiceFieldCount> intVarField_;
  etl::vector<UIStaticField, 20> staticField_;
  etl::vector<UILFOField, 12> lfoFields_;
  GraphKind graphKind_;
  GraphSection graphSection_;
  bool previewKeyHeld_;
  unsigned short previewChannel_;
  unsigned short previewInstrument_;
};

#endif
