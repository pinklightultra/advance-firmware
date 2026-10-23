/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _PASSTHROUGH_INSERT_EFFECT_H_
#define _PASSTHROUGH_INSERT_EFFECT_H_

#include "InsertEffectProcessor.h"

class PassthroughInsertEffect : public InsertEffectProcessor {
public:
  virtual ~PassthroughInsertEffect(){};

  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) override {
    (void)buffer;
    (void)samplecount;
    (void)amount;
    (void)layout;
  }

  virtual const char *GetShortName() const override { return "PAS"; }
};

#endif
