/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _UI_FM6_ALGO_FIELD_H_
#define _UI_FM6_ALGO_FIELD_H_

#include "Application/Instruments/FM6Instrument.h"
#include "UIFM6ParamField.h"
#include "UIField.h"

// Single-row DX7 algorithm field: renders the algorithm as
// modulator->carrier chains ("2>1 6*>5>4>3", '>' drawn as an arrow glyph,
// '+' sums sibling modulators, '*' marks the feedback operator), followed
// by a 'G' stop for the global block.
//
// While this field has focus, ENTER+UP/DOWN steps through the 32
// algorithms and ENTER+LEFT/RIGHT moves the cursor over the operators and
// then onto 'G', which selects what the parameter panel below shows. That
// is the standard field gesture, routed by UIFieldInputDispatcher, so it
// only applies when this row is focused; plain arrows keep doing ordinary
// field-to-field navigation.
//
// The cursor position is held in the view rather than here so it survives
// the panel rebuild that an operator/global switch triggers (see
// InstrumentView::ProcessButtonMask).
class UIFM6AlgoField : public UIField, public Observable {
public:
  UIFM6AlgoField(const GUIPoint &position, FM6Instrument &instrument,
                 uint8_t &selection);
  virtual ~UIFM6AlgoField(){};

  virtual void Draw(GUIWindow &w, int offset = 0);
  virtual void ProcessArrow(unsigned short mask);
  virtual void OnClick(){};

private:
  FM6Instrument &instrument_;
  uint8_t &selection_;
};

#endif
