/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _INSERT_EFFECT_SLOT_H_
#define _INSERT_EFFECT_SLOT_H_

#include "InsertEffectProcessor.h"

class InsertEffectSlot {
public:
  InsertEffectSlot();
  virtual ~InsertEffectSlot(){};

  void ResetState();
  void SetEffect(InsertEffectProcessor *effect);
  InsertEffectProcessor *GetEffect() const;
  void Render(AudioSample *buffer, int samplecount, int amount,
              AudioBufferLayout layout);

private:
  InsertEffectProcessor *effect_ = nullptr;
};

#endif
