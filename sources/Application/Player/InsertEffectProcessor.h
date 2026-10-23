/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _INSERT_EFFECT_PROCESSOR_H_
#define _INSERT_EFFECT_PROCESSOR_H_

#include "Application/Audio/AudioBufferLayout.h"
#include "Application/Audio/AudioSample.h"

class InsertEffectProcessor {
public:
  virtual ~InsertEffectProcessor(){};
  virtual void Reset() {}
  virtual void Render(AudioSample *buffer, int samplecount, int amount,
                      AudioBufferLayout layout) = 0;
  virtual const char *GetShortName() const = 0;
};

#endif
