/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _PASSTHROUGH_OUTPUT_EFFECT_H_
#define _PASSTHROUGH_OUTPUT_EFFECT_H_

#include "OutputEffectProcessor.h"

class PassthroughOutputEffect : public OutputEffectProcessor {
public:
  virtual ~PassthroughOutputEffect(){};

  virtual void Render(const AudioSample *input, AudioSample *output,
                      int samplecount) override {
    const int count = samplecount * 2;
    for (int i = 0; i < count; ++i) {
      output[i] = input[i];
    }
  }

  virtual const char *GetShortName() const override { return "PAS"; }
};

#endif
