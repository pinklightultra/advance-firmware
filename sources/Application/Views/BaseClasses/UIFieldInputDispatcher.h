/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _UI_FIELD_INPUT_DISPATCHER_H_
#define _UI_FIELD_INPUT_DISPATCHER_H_

#include "UIField.h"

class UIFieldInputDispatcher {
public:
  static bool ProcessButtonMask(UIField &field, unsigned short mask);
};

#endif
