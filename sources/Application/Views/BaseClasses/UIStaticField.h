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

#ifndef _UI_STATIC_FIELD_H_
#define _UI_STATIC_FIELD_H_

#include "UIField.h"

class UIStaticField : public UIField {

public:
  UIStaticField(const GUIPoint &position, const char *string,
                ColorDefinition color = CD_NORMAL);
  virtual ~UIStaticField(){};
  virtual void Draw(GUIWindow &w, int offset = 0);
  virtual void ProcessArrow(unsigned short mask);
  virtual void OnClick(){};

  virtual bool IsStatic();

protected:
protected:
  const char *string_;
  ColorDefinition color_;
};

#endif
