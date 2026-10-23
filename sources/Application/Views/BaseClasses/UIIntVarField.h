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

#ifndef _UI_INT_VAR_FIELD_H_
#define _UI_INT_VAR_FIELD_H_

#include "Foundation/Variables/Variable.h"
#include "UIField.h"

enum class UIIntVarFieldDisplayMode {
  Default,
  SignedHex,
};

class UIIntVarField : public UIField, public Observable {

public:
  UIIntVarField(
      const GUIPoint &position, Variable &v, const char *format, int min,
      int max, int xOffset, int yOffset, int displayOffset = 0,
      UIIntVarFieldDisplayMode displayMode = UIIntVarFieldDisplayMode::Default);

  virtual ~UIIntVarField(){};
  virtual void Draw(GUIWindow &w, int offset = 0);
  virtual void ProcessArrow(unsigned short mask);
  virtual void OnClick(){};

  void ProcessClear();
  FourCC GetVariableID();
  Variable &GetVariable();
  void SetMaximum(int max) { max_ = max; }

  virtual bool HasVariable() const override { return true; };

protected:
  Variable &src_;
  const char *format_;
  int min_;
  int max_;
  int xOffset_;
  int yOffset_;
  int displayOffset_;
  UIIntVarFieldDisplayMode displayMode_;
};

#endif
