/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _OUTPUT_EFFECT_PROCESSOR_H_
#define _OUTPUT_EFFECT_PROCESSOR_H_

#include "Application/Audio/AudioSample.h"

class OutputEffectProcessor {
public:
  virtual ~OutputEffectProcessor(){};
  virtual void Reset() {}
  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) = 0;
  virtual const char *GetShortName() const = 0;
};

#endif
