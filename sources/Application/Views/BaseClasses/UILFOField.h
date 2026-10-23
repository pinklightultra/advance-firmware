/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _UI_LFO_FIELD_H_
#define _UI_LFO_FIELD_H_

#include "Application/Instruments/VoiceParameters.h"
#include "UIField.h"
#include <cstdint>

enum class LFOFieldKind : uint8_t {
  Target = 0,
  Shape,
  Trigger,
  RateMode,
  Rate,
  Depth,
};

class UILFOField : public UIField {
public:
  UILFOField(const GUIPoint &position, VoiceParameters &voiceParameters,
             size_t lfoIndex, LFOFieldKind kind);

  void Draw(GUIWindow &w, int offset = 0) override;
  void ProcessArrow(unsigned short mask) override;
  void OnClick() override {}
  void ProcessClear() override;

private:
  void cycleTarget(int direction);
  void cycleShape(int direction);
  void cycleTrigger(int direction);
  void cycleRateMode(int direction);
  void adjustRate(int delta);
  void adjustDepth(int delta);
  int depthMax() const;

  VoiceParameters &voiceParameters_;
  size_t lfoIndex_;
  LFOFieldKind kind_;
};

#endif
