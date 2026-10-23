/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _UI_INSERT_AMOUNT_FIELD_H_
#define _UI_INSERT_AMOUNT_FIELD_H_

#include "UIIntVarField.h"

class UIInsertAmountField : public UIIntVarField {
public:
  UIInsertAmountField(const GUIPoint &position, Variable &amount,
                      Variable &placement, const char *format, int min, int max,
                      int yOffset);

  virtual void Draw(GUIWindow &w, int offset = 0) override;
  virtual void ProcessArrow(unsigned short mask) override;
  virtual void ProcessEditArrow(unsigned short mask) override;

private:
  void changeAmount(int delta);
  void changePlacement(int delta);

  Variable &placement_;
};

#endif
