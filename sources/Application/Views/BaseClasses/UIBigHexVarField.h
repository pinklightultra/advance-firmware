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

#ifndef _UI_BIG_HEX_VAR_FIELD_H_
#define _UI_BIG_HEX_VAR_FIELD_H_

#include "Foundation/Observable.h"
#include "UIIntVarField.h"
#include <cstdint>

enum class UIBigHexEditMode : uint8_t { Carry, IndependentDigits };

namespace UIBigHexVarFieldDetail {

inline int AdjustIndependentHexDigit(int value, uint8_t position,
                                     int direction) {
  const uint8_t shift = position * 4;
  const int digit = (value >> shift) & 0xF;
  if (direction > 0 && digit < 0xF) {
    return value + (1 << shift);
  }
  if (direction < 0 && digit > 0) {
    return value - (1 << shift);
  }
  return value;
}

} // namespace UIBigHexVarFieldDetail

class UIBigHexVarField : public UIIntVarField {

public:
  UIBigHexVarField(const GUIPoint &position, Variable &v, int precision,
                   const char *format, int min, int max, int power,
                   bool wrap = false,
                   UIBigHexEditMode editMode = UIBigHexEditMode::Carry);
  virtual ~UIBigHexVarField(){};
  virtual void Draw(GUIWindow &w, int offset = 0);
  virtual void ProcessArrow(unsigned short mask);
  void ResetDigitPosition();
  // ApplyOffset is used for selection editing where callers need to directly
  // change the value without moving the focused digit.
  // ProcessArrow is not suitable because it treats LEFT/RIGHT as digit-
  // position navigation (changing which hex digit UP/DOWN modifies) rather
  // than value change, and when position_ is 0 UP/DOWN only changes by 1.
  void ApplyOffset(int offset);

private:
  unsigned int precision_;
  unsigned int power_;
  unsigned int position_;
  bool wrap_;
  UIBigHexEditMode editMode_;
};
#endif
